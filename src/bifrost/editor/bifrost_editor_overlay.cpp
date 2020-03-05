#include "bifrost/editor/bifrost_editor_overlay.hpp"

#include "bifrost/asset_io/bifrost_assets.hpp"
#include "bifrost/bifrost.hpp"
#include "bifrost/core/bifrost_engine.hpp"
#include "bifrost/data_structures/bifrost_intrusive_list.hpp"

#include <imgui/imgui.h>          /* ImGUI::* */
#include <nativefiledialog/nfd.h> /* nfd**    */

// TODO(Shareef): This is useful for the engine aswell.
template<std::size_t size>
class FixedLinearAllocator
{
 private:
  char            m_MemoryBacking[size];
  LinearAllocator m_LinearAllocator;
  NoFreeAllocator m_NoFreeAdapter;

 public:
  FixedLinearAllocator() :
    m_MemoryBacking{},
    m_LinearAllocator{m_MemoryBacking, size},
    m_NoFreeAdapter{m_LinearAllocator}
  {
  }

  LinearAllocator& linear() { return m_LinearAllocator; }
  IMemoryManager&  memory() { return m_NoFreeAdapter; }

  operator IMemoryManager&()
  {
    return memory();
  }
};

// clang-format off
template<std::size_t Size>
class BlockAllocator final : public IMemoryManager, private bfNonCopyMoveable<BlockAllocator<Size>>
// clang-format on
{
  struct MemoryBlock final
  {
    char            memory_backing[Size];
    IMemoryManager& allocator;
    MemoryBlock*    next;

    explicit MemoryBlock(MemoryBlock* prev) :
      memory_backing{},
      allocator{memory_backing, Size},
      next{nullptr}
    {
      if (prev)
      {
        prev->next = this;
      }
    }
  };

 private:
  IMemoryManager& m_BlockAllocator;
  MemoryBlock     m_SmallBacking;
  MemoryBlock*    m_Tail;

 public:
  explicit BlockAllocator(IMemoryManager& block_allocator) :
    m_BlockAllocator{block_allocator},
    m_SmallBacking{nullptr},
    m_Tail{&m_SmallBacking}
  {
  }

  void* allocate(std::size_t size) override
  {
    // TODO(Shareef): Handle the case of - If Size <= size.
    void* ptr = m_Tail->allocator.allocate(size);

    if (!ptr)
    {
      MemoryBlock* new_block = m_BlockAllocator.allocateT<MemoryBlock>(m_Tail);

      if (!new_block)
      {
        return nullptr;
      }

      m_Tail = new_block;

      ptr = new_block->allocator.allocate(size);
    }

    return ptr;
  }

  void deallocate(void* ptr) override
  {
    MemoryBlock* cursor = &m_SmallBacking;

    while (cursor)
    {
      if (cursor->memory_backing >= ptr && ptr < (cursor->memory_backing + Size))
      {
        cursor->allocator.deallocate(ptr);
        break;
      }

      cursor = cursor->next;
    }
  }

  virtual ~BlockAllocator()
  {
    MemoryBlock* cursor = &m_SmallBacking;

    while (cursor)
    {
      MemoryBlock* const next = cursor->next;
      m_BlockAllocator.deallocateT(cursor);
      cursor = next;
    }
  }
};

namespace bifrost::editor
{
  using namespace intrusive;

  static char              s_EditorMemoryBacking[4096];
  static FreeListAllocator s_EditorMemory{s_EditorMemoryBacking, sizeof(s_EditorMemoryBacking)};
  static ui::MainMenu      s_MainMenuBar("Main Menu", s_EditorMemory);

  IMemoryManager& allocator()
  {
    return s_EditorMemory;
  }

  bool ActionContext::actionButton(const char* name) const
  {
    auto* const action = editor->findAction(name);

    if (action && action->isActive(*this) && ImGui::Button(name))
    {
      action->execute(*this);
      return true;
    }

    return false;
  }

  template<typename T, typename... Args>
  T* make(Args&&... args)
  {
    return allocator().allocateT<T>(std::forward<Args&&>(args)...);
  }

  namespace ui
  {
    MenuDropdown* makeDropdown(const char* name)
    {
      return s_EditorMemory.allocateT<MenuDropdown>(name, s_EditorMemory);
    }

    MenuAction* makeAction(const char* name, Action* action = nullptr)
    {
      return s_EditorMemory.allocateT<MenuAction>(name, action);
    }
  }  // namespace ui

  template<typename T>
  class ShowDialogAction final : public Action
  {
   public:
    explicit ShowDialogAction() = default;

    void execute(const ActionContext& ctx) override
    {
      ctx.editor->enqueueDialog(make<T>());
    }

    ~ShowDialogAction() override = default;
  };

  template<typename F>
  class LambdaAction final : public Action
  {
   private:
    F m_Fn;

   public:
    LambdaAction(F&& f) :
      m_Fn{f}
    {
    }

    void execute(const ActionContext& ctx) override
    {
      m_Fn(ctx);
    }

    ~LambdaAction() override = default;
  };

  template<typename TRet = void>
  class MemberAction : public Action
  {
   private:
    TRet (EditorOverlay::*m_Fn)();

   public:
    MemberAction(TRet (EditorOverlay::*f)()) :
      m_Fn{f}
    {
    }

    void execute(const ActionContext& ctx) override
    {
      (ctx.editor->*m_Fn)();
    }

    ~MemberAction() override = default;
  };

  class WelcomeDialog : public ui::Dialog
  {
   public:
    explicit WelcomeDialog() :
      Dialog("Welcome")
    {
    }

    void show(const ActionContext& ctx) override
    {
      if (ctx.actionButton("New Project"))
      {
        close();
      }

      if (ImGui::Button("Open Project") && ctx.editor->openProjectDialog())
      {
        close();
      }
    }
  };

  class NewProjectDialog : public ui::Dialog
  {
   private:
    char m_ProjectName[256];
    char m_ProjectPath[256];

   public:
    explicit NewProjectDialog() :
      Dialog("New Project"),
      m_ProjectName{"New Bifrost Project"},
      m_ProjectPath{'\0'}
    {
    }

    void show(const ActionContext& ctx) override
    {
      ImGui::InputText("Name", m_ProjectName, sizeof(m_ProjectName));
      ImGui::InputText("Path", m_ProjectPath, sizeof(m_ProjectPath), ImGuiInputTextFlags_CharsNoBlank);

      ImGui::SameLine();

      if (ImGui::Button("Select..."))
      {
        nfdchar_t*        out_path = nullptr;
        const nfdresult_t result   = NFD_PickFolder(nullptr, &out_path);

        if (result == NFD_OKAY)
        {
          std::strcpy(m_ProjectPath, out_path);
          free(out_path);

          file::canonicalizePath(m_ProjectPath);
        }
        else if (result == NFD_CANCEL)
        {
        }
        else
        {
          // printf("Error: %s\n", NFD_GetError());
        }
      }

      ImGui::Separator();

      if (path::doesExist(m_ProjectPath))
      {
        if (ImGui::Button("Create"))
        {
          String full_path{m_ProjectPath};

          full_path.append("/");
          full_path.append(m_ProjectName);

          if (path::createDirectory(full_path.cstr()))
          {
            const String meta_path = full_path + "/_meta";

            if (!path::createDirectory(meta_path.cstr()))
            {
              bfLogError("Failed to create '%s' directory", meta_path.cstr());
            }

            const String project_file_path = full_path + "/Project.project.json";

            const JsonValue json = JsonValue{
             std::pair{std::string("Name"), JsonValue{m_ProjectName}},
             std::pair{std::string("Path"), JsonValue{m_ProjectPath}},
            };

            std::string json_str;
            json.toString(json_str, true, 4);

            File project_file{project_file_path, file::FILE_MODE_WRITE};
            project_file.writeBytes(json_str.c_str(), json_str.length());
            project_file.close();

            ctx.editor->openProject(project_file_path);
            close();
          }
        }
      }
      else
      {
        ImGui::Button("Please Select a Valid Path");
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel"))
      {
        close();
      }
    }
  };

  bool ui::MenuDropdown::beginItem(const ActionContext& ctx)
  {
    return ImGui::BeginMenu(name());
  }

  void ui::MenuDropdown::doAction(const ActionContext& ctx)
  {
    for (const auto& item : m_SubItems)
    {
      if (item->beginItem(ctx))
      {
        item->doAction(ctx);
        item->endItem();
      }
    }
  }

  void ui::MenuDropdown::endItem()
  {
    ImGui::EndMenu();
  }

  ui::MenuDropdown* ui::MenuDropdown::addItem(BaseMenuItem* item)
  {
    m_SubItems.push(item);
    return this;
  }

  bool ui::MainMenu::beginItem(const ActionContext& ctx)
  {
    return ImGui::BeginMainMenuBar();
  }

  void ui::MainMenu::endItem()
  {
    ImGui::EndMainMenuBar();
  }

  bool ui::MenuAction::beginItem(const ActionContext& ctx)
  {
    return ImGui::MenuItem(name(), nullptr, false, m_Action && m_Action->isActive(ctx));
  }

  void ui::MenuAction::doAction(const ActionContext& ctx)
  {
    if (m_Action)
    {
      m_Action->execute(ctx);
    }
  }

  void ui::MenuAction::endItem()
  {
  }

  class ACloseProject : public Action
  {
   public:
    void execute(const ActionContext& ctx) override
    {
      ctx.editor->closeProject();
    }

    bool isActive(const ActionContext& ctx) const override
    {
      return ctx.editor->currentlyOpenProject() != nullptr;
    }
  };

  class ARefreshAsset final : public MemberAction<void>
  {
   public:
    explicit ARefreshAsset() :
      MemberAction<void>{&EditorOverlay::assetRefresh}
    {
    }

    bool isActive(const ActionContext& ctx) const override
    {
      return ctx.editor->currentlyOpenProject() != nullptr;
    }
  };

  template<std::size_t N>
  void addMainMenuItem(const char* name, ui::BaseMenuItem* (&menu_items)[N])
  {
    auto* const menu = ui::makeDropdown(name);

    for (auto* const item : menu_items)
    {
      menu->addItem(item);
    }

    s_MainMenuBar.addItem(menu);
  }

  void EditorOverlay::onCreate(Engine& engine)
  {
    m_Engine = &engine;

    ImGuiStyle&   style  = ImGui::GetStyle();
    ImVec4* const colors = style.Colors;

    style.FrameRounding    = 2.0f;
    style.FramePadding     = ImVec2(4.0f, 2.0f);
    style.FrameBorderSize  = 1.0f;
    style.WindowBorderSize = 1.0f;
    style.WindowPadding    = ImVec2(5.0f, 5.0f);
    style.WindowRounding   = 3.0f;
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ChildRounding    = 2.0f;
    style.GrabMinSize      = 6.0f;
    style.GrabRounding     = 2.0f;

    colors[ImGuiCol_Text]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_WindowBg]          = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBg]           = ImVec4(0.06f, 0.06f, 0.07f, 0.54f);
    colors[ImGuiCol_TitleBgActive]     = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Border]            = ImVec4(0.09f, 0.05f, 0.11f, 0.38f);
    colors[ImGuiCol_TitleBg]           = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]  = ImVec4(0.00f, 0.00f, 0.00f, 0.66f);
    colors[ImGuiCol_CheckMark]         = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ResizeGrip]        = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.28f, 0.33f, 0.81f);
    colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.16f, 0.15f, 0.20f, 0.95f);
    colors[ImGuiCol_Tab]               = ImVec4(0.12f, 0.09f, 0.16f, 0.86f);
    colors[ImGuiCol_TabActive]         = ImVec4(0.41f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_Header]            = ImVec4(0.08f, 0.08f, 0.09f, 0.31f);
    colors[ImGuiCol_HeaderHovered]     = ImVec4(0.26f, 0.26f, 0.27f, 0.80f);
    colors[ImGuiCol_HeaderActive]      = ImVec4(0.14f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]    = ImVec4(0.63f, 0.65f, 0.68f, 0.44f);
    colors[ImGuiCol_FrameBgActive]     = ImVec4(0.37f, 0.38f, 0.40f, 0.89f);
    colors[ImGuiCol_SliderGrab]        = ImVec4(0.75f, 0.75f, 0.77f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]  = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_Button]            = ImVec4(0.51f, 0.53f, 0.71f, 0.40f);
    colors[ImGuiCol_ButtonHovered]     = ImVec4(0.45f, 0.43f, 0.52f, 0.86f);
    colors[ImGuiCol_ButtonActive]      = ImVec4(0.26f, 0.24f, 0.30f, 0.82f);

    m_Actions.emplace("New Project", ActionPtr(make<ShowDialogAction<NewProjectDialog>>()));
    m_Actions.emplace("Open Project", ActionPtr(make<MemberAction<bool>>(&EditorOverlay::openProjectDialog)));
    m_Actions.emplace("Close Project", ActionPtr(make<ACloseProject>()));
    m_Actions.emplace("Asset Refresh", ActionPtr(make<ARefreshAsset>()));

    ui::BaseMenuItem* file_menu_items[] =
     {
      ui::makeDropdown("New")->addItem(ui::makeAction("Project", findAction("New Project"))),
      ui::makeDropdown("Open")->addItem(ui::makeAction("Project", findAction("Open Project"))),
      ui::makeDropdown("Save")->addItem(ui::makeAction("Project")),
      ui::makeAction("Close Project", findAction("Close Project")),
      ui::makeAction("Exit"),
     };

    ui::BaseMenuItem* asset_menu_items[] =
     {
      ui::makeAction("Refresh", findAction("Asset Refresh")),
     };

    ui::BaseMenuItem* edit_menu_items[] =
     {
      ui::makeAction("Copy"),
     };

    ui::BaseMenuItem* view_menu_items[] =
     {
      ui::makeAction("DUMMY_ITEM"),
     };

    ui::BaseMenuItem* entity_menu_items[] =
     {
      ui::makeAction("DUMMY_ITEM"),
     };

    ui::BaseMenuItem* component_menu_items[] =
     {
      ui::makeAction("DUMMY_ITEM"),
     };

    ui::BaseMenuItem* help_menu_items[] =
     {
      ui::makeAction("DUMMY_ITEM"),
     };

    addMainMenuItem("File", file_menu_items);
    addMainMenuItem("Asset", asset_menu_items);
    addMainMenuItem("Edit", edit_menu_items);
    addMainMenuItem("View / Window", view_menu_items);
    addMainMenuItem("Entity", entity_menu_items);
    addMainMenuItem("Component", component_menu_items);
    addMainMenuItem("Help", help_menu_items);

    // enqueueDialog(make<WelcomeDialog>());
  }

  void EditorOverlay::onLoad(Engine& engine)
  {
  }

  void EditorOverlay::onEvent(Engine& engine, Event& event)
  {
    event.accept();
  }

  void EditorOverlay::onUpdate(Engine& engine, float delta_time)
  {
    static const float window_width  = 350.0f;
    static const float window_margin = 5.0f;

    const ActionContext action_ctx{this};

    // ImGui::ShowDemoWindow();

    float menu_bar_height = 0.0f;

    if (s_MainMenuBar.beginItem(action_ctx))
    {
      menu_bar_height = ImGui::GetWindowSize().y;

      s_MainMenuBar.doAction(action_ctx);

      m_FpsTimer -= delta_time;

      if (m_FpsTimer <= 0.0f)
      {
        m_CurrentFps = int(1.0f / delta_time);
        m_FpsTimer   = 1.0f;
      }

      ImGui::Text("| %ifps |", m_CurrentFps);
      s_MainMenuBar.endItem();
    }

    ImGuiIO& io = ImGui::GetIO();

    if (m_OpenProject)
    {
      const auto height = io.DisplaySize.y - (window_margin * 2.0f) - menu_bar_height;

      ImGui::SetNextWindowPos(ImVec2(window_margin, menu_bar_height + window_margin));
      ImGui::SetNextWindowSize(ImVec2(window_width, height), ImGuiCond_Appearing);
      ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f, height), ImVec2(600.0f, height));

      if (ImGui::Begin("Project View"))
      {
        ImGui::Text("%s", m_OpenProject->name().cstr());
        ImGui::Separator();

        if (ImGui::CollapsingHeader("Assets"))
        {
          for (const auto& asset : engine.assets().assetMap())
          {
            BaseAssetInfo* const asset_handle = asset.value();

            if (ImGui::TreeNode(asset_handle->path().cstr()))
            {
              const auto type = asset_handle->type();

              ImGui::Text("UUID    : %s", asset_handle->uuid().as_string);
              ImGui::Text("RefCount: %i", asset_handle->refCount());

              if (type)
              {
                ImGui::Text("Type Name: '%s'", type->name().data());
              }
              else
              {
                ImGui::Text("Unregistered Type <nullptr>");
              }

              ImGui::TreePop();
            }
          }
        }
      }

      ImGui::End();
    }

    if (ImGui::Begin("RTTI"))
    {
      auto& memory = meta::gRttiMemoryBacking();
      ImGui::Text("Memory Usage %i / %i", int(memory.usedMemory()), int(memory.size()));

      for (auto& type : meta::gRegistry())
      {
        const auto& name = type.key();

        if (ImGui::TreeNode(type.value(), "%.*s", name.length(), name.data()))
        {
          auto* const type_info = type.value();

          ImGui::Text("Size: %i, Alignment %i", int(type_info->size()), int(type_info->alignment()));

          for (auto& field : type_info->members())
          {
            ImGui::Text("member: %.*s", field->name().length(), field->name().data());
          }

          for (auto& prop : type_info->properties())
          {
            ImGui::Text("prop: %.*s", prop->name().length(), prop->name().data());
          }

          for (auto& method : type_info->methods())
          {
            ImGui::Text("method: %.*s", method->name().length(), method->name().data());
          }

          ImGui::Separator();
          ImGui::TreePop();
        }
      }
    }
    ImGui::End();

    if (m_OpenNewDialog)
    {
      if (m_CurrentDialog)
      {
        ImGui::OpenPopup(m_CurrentDialog->name());
      }

      m_OpenNewDialog = false;
    }

    if (m_CurrentDialog)
    {
      if (ImGui::BeginPopupModal(m_CurrentDialog->name(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
      {
        m_CurrentDialog->show(action_ctx);

        if (m_CurrentDialog->wantsToClose())
        {
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }
    }
  }

  void EditorOverlay::onUnload(Engine& engine)
  {
  }

  void EditorOverlay::onDestroy(Engine& engine)
  {
    enqueueDialog(nullptr);
  }

  Action* EditorOverlay::findAction(const char* name) const
  {
    ActionPtr* action_ptr = m_Actions.at(name);

    return action_ptr ? action_ptr->get() : nullptr;
  }

  void EditorOverlay::enqueueDialog(ui::Dialog* dlog)
  {
    deallocateT(m_CurrentDialog);
    m_CurrentDialog = dlog;
    m_OpenNewDialog = true;
  }

  bool EditorOverlay::openProjectDialog()
  {
    const nfdchar_t* filter = "project.json";

    nfdchar_t*        out_path = nullptr;
    const nfdresult_t result   = NFD_OpenDialog(filter, nullptr, &out_path);
    bool              did_open = false;

    if (result == NFD_OKAY)
    {
      const std::size_t length = file::canonicalizePath(out_path);

      did_open = openProject({out_path, out_path + length});
    }
    else if (result == NFD_CANCEL)
    {
    }
    else
    {
      // printf("Error: %s\n", NFD_GetError());
    }

    return did_open;
  }

  bool EditorOverlay::openProject(const StringRange path)
  {
    const String path_str = {path};
    File         project_file{path_str, file::FILE_MODE_READ};

    if (project_file)
    {
      const StringRange project_dir       = file::directoryOfFile(path);
      String            project_meta_path = project_dir;

      project_meta_path.append("/");
      project_meta_path.append(Assets::META_PATH_NAME);

      String project_json_str{};
      project_file.readAll(project_json_str);

      bfLogPush("Open This Project:");
      bfLogPrint("Directory(%.*s)", int(project_dir.length()), project_dir.bgn);
      bfLogPrint("%s", project_json_str.cstr());
      bfLogPop();

      const AssetError err = m_Engine->assets().setRootPath(std::string_view{project_dir.bgn, project_dir.length()});

      if (!path::doesExist(project_meta_path.cstr()) && !path::createDirectory(project_meta_path.cstr()))
      {
        bfLogWarn("Project does not have meta asset files. (%s)", project_meta_path.cstr());
      }
      else
      {
        FixedLinearAllocator<1024> allocator;
        path::DirectoryEntry*      dir = path::openDirectory(allocator.memory(), project_meta_path);

        if (dir)
        {
          do
          {
            const char* name = entryFilename(dir);

            if (isFile(dir))
            {
              m_Engine->assets().loadMeta(bfMakeStringRangeC(name));
            }
          } while (readNextEntry(dir));

          closeDirectory(dir);
        }
      }

      if (err == AssetError::NONE)
      {
        const JsonValue project_json = JsonParser::parse(project_json_str);

        assert(project_json.isObject());

        const JsonValue* const project_name = project_json.at("Name");

        if (project_name && project_name->isString())
        {
          const auto& project_name_str = project_name->as<string_t>();

          m_OpenProject.reset(make<Project>(String(project_name_str.c_str()), project_dir, std::move(project_meta_path)));

          return true;
        }
      }
    }

    return false;
  }

  void EditorOverlay::closeProject()
  {
    m_OpenProject.reset(nullptr);
  }

  // Asset Management

  struct FileExtensionHandler final
  {
    using Callback = void (*)(Assets& engine, StringRange relative_path, StringRange meta_name);

    StringRange ext;
    Callback    handler;

    FileExtensionHandler(StringRange extension, Callback callback) :
      ext{extension},
      handler{callback}
    {
    }
  };

  struct MetaAssetPath final
  {
    char*       file_name{nullptr};
    StringRange meta_name;
  };

  template<typename T>
  static void fileExtensionHandlerImpl(Assets& assets, StringRange relative_path, StringRange meta_name)
  {
    assets.indexAsset<T>(relative_path, meta_name);
  }

  static const FileExtensionHandler s_AssetHandlers[] =
   {
    {".png", &fileExtensionHandlerImpl<AssetTextureInfo>},
    {".jpg", &fileExtensionHandlerImpl<AssetTextureInfo>},
    {".jpeg", &fileExtensionHandlerImpl<AssetTextureInfo>},
    {".ppm", &fileExtensionHandlerImpl<AssetTextureInfo>},
    {".pgm", &fileExtensionHandlerImpl<AssetTextureInfo>},
    {".bmp", &fileExtensionHandlerImpl<AssetTextureInfo>},
    {".tga", &fileExtensionHandlerImpl<AssetTextureInfo>},
    {".psd", &fileExtensionHandlerImpl<AssetTextureInfo>},

    {".glsl", &fileExtensionHandlerImpl<AssetShaderModuleInfo>},
    {".frag", &fileExtensionHandlerImpl<AssetShaderModuleInfo>},
    {".vert", &fileExtensionHandlerImpl<AssetShaderModuleInfo>},

    {".shader", &fileExtensionHandlerImpl<AssetShadeProgramInfo>},

    // {".obj", &fileExtensionHandlerImpl<AssetShadeProgramHandle>},
    // {".gltf", &fileExtensionHandlerImpl<AssetShadeProgramHandle>},
  };

  static void                        assetFindAssets(List<MetaAssetPath>& metas, const String& path, const String& current_string);
  static const FileExtensionHandler* assetFindHandler(StringRange relative_path);

  void EditorOverlay::assetRefresh()
  {
    const auto& path = m_OpenProject->path();

    if (path::doesExist(path.cstr()))
    {
      FixedLinearAllocator<1024> allocator;
      List<MetaAssetPath>        metas_to_check{allocator};

      assetFindAssets(metas_to_check, path, "");

      for (const auto& meta : metas_to_check)
      {
        const char* const                 relative_path_bgn = meta.file_name + path.length() + 1;
        const StringRange                 relative_path     = {relative_path_bgn, relative_path_bgn + std::strlen(relative_path_bgn)};
        const FileExtensionHandler* const handler           = assetFindHandler(relative_path);

        if (handler)
        {
          bfLogPush("(%s)", meta.file_name);
          {
            const StringRange file_name = file::fileNameOfPath(relative_path);

            bfLogPrint("Meta-Name    : (%s)", meta.meta_name.bgn);
            bfLogPrint("Relative-Path: (%s)", relative_path.bgn);
            bfLogPrint("File-Name    : (%.*s)", (unsigned)file_name.length(), file_name.bgn);

            handler->handler(m_Engine->assets(), relative_path, meta.meta_name);
          }
          bfLogPop();
        }
        else
        {
          bfLogWarn("Unknown file type (%s)", meta.file_name);
        }

        string_utils::free_fmt(allocator, meta.file_name);
        string_utils::free_fmt(allocator, const_cast<char*>(meta.meta_name.bgn));
      }
    }
  }

  static void assetFindAssets(List<MetaAssetPath>& metas, const String& path, const String& current_string)
  {
    FixedLinearAllocator<512> allocator;
    path::DirectoryEntry*     dir = path::openDirectory(allocator.memory(), path);

    if (dir)
    {
      do
      {
        const char* name = entryFilename(dir);

        if (name[0] != '.' && name[0] != '_')
        {
          if (isDirectory(dir))
          {
            assetFindAssets(metas, path + "/" + name, current_string + name + ".");
          }
          else
          {
            auto& m = metas.emplaceBack();

            m.file_name = string_utils::alloc_fmt(metas.memory(), nullptr, "%s/%s", path.cstr(), name);

            std::size_t meta_name_length;
            char* const meta_name = string_utils::alloc_fmt(metas.memory(), &meta_name_length, "%s%s.meta", current_string.cstr(), name);

            m.meta_name = {meta_name, meta_name + meta_name_length};
          }
        }
      } while (readNextEntry(dir));

      closeDirectory(dir);
    }
    else
    {
      bfLogError("Could not open directory (%s)!", path.cstr());
    }
  }

  static const FileExtensionHandler* assetFindHandler(StringRange relative_path)
  {
    const StringRange file_ext = file::extensionOfFile(relative_path);

    for (const auto& handler : s_AssetHandlers)
    {
      if (handler.ext == file_ext)
      {
        return &handler;
      }
    }

    return nullptr;
  }
}  // namespace bifrost::editor
