#include "bifrost/editor/bifrost_editor_overlay.hpp"

#include "bifrost/asset_io/bifrost_assets.hpp"
#include "bifrost/bifrost.hpp"
#include "bifrost/core/bifrost_engine.hpp"
#include "bifrost/data_structures/bifrost_intrusive_list.hpp"

#include <imgui/imgui.h> /* ImGUI::* */

#include "imgui/imgui_internal.h"
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
template<std::size_t Size, typename TAllocator = FreeListAllocator>
class BlockAllocator final : public IMemoryManager, private bfNonCopyMoveable<BlockAllocator<Size>>
// clang-format on
{
  struct MemoryBlock final
  {
    char         memory_backing[Size];
    TAllocator   allocator;
    MemoryBlock* next;

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
    assert(size <= Size);

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

  static char              s_EditorMemoryBacking[4096 * 2];
  static FreeListAllocator s_EditorMemory{s_EditorMemoryBacking, sizeof(s_EditorMemoryBacking)};
  static ui::MainMenu      s_MainMenuBar("Main Menu", s_EditorMemory);

  IMemoryManager& allocator()
  {
    return s_EditorMemory;
  }

  template<typename T, typename... Args>
  T* make(Args&&... args)
  {
    return allocator().allocateT<T>(std::forward<Args&&>(args)...);
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

  class NewFolderDialog : public ui::Dialog
  {
   private:
    String m_BasePath;
    char   m_FolderName[120];  // 120 is the max folder name length on windows.

   public:
    explicit NewFolderDialog(const String& base_path) :
      Dialog("New Folder"),
      m_BasePath{base_path},
      m_FolderName{"FolderName"}
    {
    }

    void show(const ActionContext& ctx) override
    {
      ImGui::InputText("Name", m_FolderName, sizeof(m_FolderName));

      ImGui::Separator();

      if (file::isValidName(m_FolderName))
      {
        if (ImGui::Button("Create"))
        {
          String full_path{m_BasePath};

          full_path.append("/");
          full_path.append(m_FolderName);

          if (path::createDirectory(full_path.cstr()))
          {
            ctx.editor->assetRefresh();
          }
          else
          {
            bfLogError("Failed to create Folder: %s", full_path.cstr());
          }

          close();
        }
      }
      else
      {
        ImGui::Button("Please Use a Valid Name");
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel"))
      {
        close();
      }
    }
  };

  template<typename TAsset>
  class BaseNewAssetDialog : public ui::Dialog
  {
   protected:
    String m_BasePath;
    char   m_AssetName[120];  // 120 is the max folder name length on windows.

   public:
    explicit BaseNewAssetDialog(const String& base_path) :
      Dialog("New Asset"),
      m_BasePath{base_path},
      m_AssetName{"NewAsset"}
    {
    }

    virtual bool createAsset(String& full_path) = 0;

    void show(const ActionContext& ctx) override
    {
      ImGui::InputText("Name", m_AssetName, sizeof(m_AssetName));

      ImGui::Separator();

      if (file::isValidName(m_AssetName))
      {
        if (ImGui::Button("Create"))
        {
          String full_path{m_BasePath};

          full_path.append("/");
          full_path.append(m_AssetName);

          if (createAsset(full_path))
          {
            ctx.editor->assetRefresh();
          }
          else
          {
            bfLogError("Failed to create Asset: %s", full_path.cstr());
          }

          close();
        }
      }
      else
      {
        ImGui::Button("Please Use a Valid Name");
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

    m_Actions.emplace("File.New.Project", ActionPtr(make<ShowDialogAction<NewProjectDialog>>()));
    m_Actions.emplace("File.Open.Project", ActionPtr(make<MemberAction<bool>>(&EditorOverlay::openProjectDialog)));
    m_Actions.emplace("Close Project", ActionPtr(make<ACloseProject>()));
    m_Actions.emplace("Asset Refresh", ActionPtr(make<ARefreshAsset>()));

    ui::BaseMenuItem* file_menu_items[] =
     {
      ui::makeDropdown("New")->addItem(ui::makeAction("Project", findAction("File.New.Project"))),
      ui::makeDropdown("Open")->addItem(ui::makeAction("Project", findAction("File.Open.Project"))),
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
    addMainMenuItem("View", view_menu_items);
    addMainMenuItem("Entity", entity_menu_items);
    addMainMenuItem("Component", component_menu_items);
    addMainMenuItem("Help", help_menu_items);

    // enqueueDialog(make<WelcomeDialog>());
  }

  void EditorOverlay::onLoad(Engine& engine)
  {
  }

  static const float invalid_mouse_postion = -1.0f;
  static const float mouse_speed           = 0.01f;
  float              oldx                  = invalid_mouse_postion;
  float              oldy                  = invalid_mouse_postion;

  void EditorOverlay::onEvent(Engine& engine, Event& event)
  {
    auto& mouse_evt = event.mouse;

    if (event.type == EventType::ON_MOUSE_DOWN || event.type == EventType::ON_MOUSE_UP)
    {
      oldx = invalid_mouse_postion;
      oldy = invalid_mouse_postion;
    }
    else if (event.type == EventType::ON_MOUSE_MOVE)
    {
      if (mouse_evt.button_state & MouseEvent::BUTTON_LEFT)
      {
        const float newx = float(mouse_evt.x);
        const float newy = float(mouse_evt.y);

        if (oldx == invalid_mouse_postion)
        {
          oldx = newx;
        }

        if (oldy == invalid_mouse_postion)
        {
          oldy = newy;
        }

        Camera_mouse(&engine.renderer().camera(), (newx - oldx) * mouse_speed, (newy - oldy) * mouse_speed);

        oldx = newx;
        oldy = newy;
      }
    }

    {
      ImGuiIO& io = ImGui::GetIO();

      if (io.WantCaptureKeyboard || io.WantCaptureMouse)
      {
        event.accept();
      }
    }

    event.accept();
  }

  void EditorOverlay::onUpdate(Engine& engine, float delta_time)
  {
    static const float window_width  = 350.0f;
    static const float window_margin = 5.0f;

    const ActionContext action_ctx{this};

    m_Inspector.setAssets(&engine.assets());

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

      ImGui::Text("| %ifps | Memory (%i / %i) |", m_CurrentFps, s_EditorMemory.usedMemory(), s_EditorMemory.size());
      s_MainMenuBar.endItem();
    }

    ImGuiIO& io = ImGui::GetIO();

    // Dock Space
    {
      static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

      ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
      ImGuiViewport*   viewport     = ImGui::GetMainViewport();

      ImGui::SetNextWindowPos(viewport->GetWorkPos());
      ImGui::SetNextWindowSize(viewport->GetWorkSize());
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

      window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
      window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

      if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

      ImGui::Begin("DockSpace Demo", nullptr, window_flags);

      const ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

      ImGui::End();

      ImGui::PopStyleVar(3);

      ImGui::DockBuilderDockWindow("Scene View", dockspace_id);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    if (ImGui::Begin("Scene View", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar))
    {
      const auto content_area = ImGui::GetContentRegionAvail();

      if (m_OpenProject)
      {
        ImGui::Image(engine.renderer().colorBuffer(), content_area);
      }
      else
      {
        static const char* s_StrNoProjectOpen = "No Project Open";
        static const char* s_StrNewProject    = "  New Project  ";
        static const char* s_StrOpenProject   = "  Open Project ";

        const auto text_size  = ImGui::CalcTextSize(s_StrNoProjectOpen);
        const auto mid_screen = (ImGui::GetWindowSize() - text_size) * 0.5f;

        ImGui::SetCursorPos(
         (ImGui::GetWindowSize() - text_size) * 0.5f);

        ImGui::Text(s_StrNoProjectOpen);

        ImGui::SetCursorPosX(mid_screen.x);

        buttonAction(action_ctx, "File.New.Project", s_StrNewProject);

        ImGui::SetCursorPosX(mid_screen.x);

        buttonAction(action_ctx, "File.Open.Project", s_StrOpenProject);
      }
    }
    ImGui::End();
    ImGui::PopStyleVar(3);

    if (m_OpenProject)
    {
      const auto display_width = io.DisplaySize.x;

      const auto height = io.DisplaySize.y - (window_margin * 2.0f) - menu_bar_height;

      ImGui::SetNextWindowPos(ImVec2(window_margin, menu_bar_height + window_margin), ImGuiCond_Once);
      ImGui::SetNextWindowSize(ImVec2(window_width, height), ImGuiCond_Once);

      if (ImGui::Begin("Project View"))
      {
        ImGui::Separator();

        if (imgui_ext::inspect("Project Name", m_OpenProject->name()))
        {
        }

        ImGui::Separator();

        m_FileSystem.uiShow(*this);
      }

      ImGui::End();

      ImGui::SetNextWindowPos(ImVec2(display_width - window_margin, menu_bar_height + window_margin), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
      ImGui::SetNextWindowSize(ImVec2(window_width, height), ImGuiCond_Once);

      if (ImGui::Begin("Inspector View"))
      {
        static BifrostTransform transform = {};
        static bool             once      = true;

        if (once)
        {
          bfTransform_ctor(&transform);
          once = false;
        }

        m_Inspector.beginDocument(false);
        m_Inspector.serializeT("Transform", &transform);
        m_Inspector.serialize("Test Texture", m_TestTexture);
        m_Inspector.endDocument();
      }

      ImGui::End();
    }
    /*
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

          for (auto& base_type : type_info->baseClasses())
          {
            ImGui::Text("base: %.*s", base_type->name().length(), base_type->name().data());
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
    //*/
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

  EditorOverlay::EditorOverlay() :
    m_CurrentDialog{nullptr},
    m_OpenNewDialog{false},
    m_Actions{},
    m_Engine{nullptr},
    m_OpenProject{nullptr},
    m_FpsTimer{0.0f},
    m_CurrentFps{0},
    m_TestTexture{nullptr},
    m_FileSystem{allocator()},
    m_Inspector{allocator()}
  {
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
      if (currentlyOpenProject())
      {
        closeProject();
      }

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

          assetRefresh();
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
    using Callback = BifrostUUID (*)(Assets& engine, StringRange relative_path, StringRange meta_name);

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
    StringRange meta_name{nullptr, nullptr};
    FileEntry*  entry{nullptr};
  };

  template<typename T>
  static BifrostUUID fileExtensionHandlerImpl(Assets& assets, StringRange relative_path, StringRange meta_name)
  {
    return assets.indexAsset<T>(relative_path, meta_name);
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
    {".spv", &fileExtensionHandlerImpl<AssetShaderModuleInfo>},

    {".shader", &fileExtensionHandlerImpl<AssetShaderProgramInfo>},

    // {".obj", &fileExtensionHandlerImpl<>},
    // {".gltf", &fileExtensionHandlerImpl<>},
  };

  static void                        assetFindAssets(List<MetaAssetPath>& metas, const String& path, const String& current_string, FileSystem& filesystem, FileEntry& parent_entry);
  static const FileExtensionHandler* assetFindHandler(StringRange relative_path);

  void EditorOverlay::assetRefresh()
  {
    const auto& path = m_OpenProject->path();

    if (path::doesExist(path.cstr()))
    {
      FixedLinearAllocator<2048> allocator;
      List<MetaAssetPath>        metas_to_check{allocator};

      m_FileSystem.clear("Assets", path);
      assetFindAssets(metas_to_check, path, "", m_FileSystem, m_FileSystem.root());

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

            meta.entry->uuid = handler->handler(m_Engine->assets(), relative_path, meta.meta_name);
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

  void EditorOverlay::buttonAction(const ActionContext& ctx, const char* action_name) const
  {
    buttonAction(ctx, action_name, action_name);
  }

  void EditorOverlay::buttonAction(const ActionContext& ctx, const char* action_name, const char* custom_label) const
  {
    Action* const action = findAction(action_name);

    const bool is_disabled = action == nullptr || !action->isActive(ctx);

    if (is_disabled)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if (ImGui::Button(custom_label))
    {
      if (!is_disabled)
      {
        action->execute(ctx);
      }
    }

    if (is_disabled)
    {
      ImGui::PopItemFlag();
      ImGui::PopStyleVar();
    }
  }

  static void assetFindAssets(List<MetaAssetPath>& metas, const String& path, const String& current_string, FileSystem& filesystem, FileEntry& parent_entry)
  {
    FixedLinearAllocator<512> dir_allocator;
    path::DirectoryEntry*     dir = path::openDirectory(dir_allocator, path);

    if (dir)
    {
      do
      {
        const char* name = entryFilename(dir);

        if (name[0] != '.' && name[0] != '_')
        {
          const bool   is_directory = isDirectory(dir);
          const String full_path    = path + "/" + name;
          FileEntry&   entry        = filesystem.makeNode(name, full_path, !is_directory);

          if (is_directory)
          {
            assetFindAssets(metas, full_path, current_string + name + ".", filesystem, entry);
          }
          else
          {
            auto& m = metas.emplaceBack();

            m.file_name = string_utils::alloc_fmt(metas.memory(), nullptr, "%s/%s", path.cstr(), name);

            std::size_t meta_name_length;
            char* const meta_name = string_utils::alloc_fmt(metas.memory(), &meta_name_length, "%s%s.meta", current_string.cstr(), name);

            m.meta_name = {meta_name, meta_name + meta_name_length};
            m.entry     = &entry;
          }

          parent_entry.children.pushFront(entry);
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

  // bifrost_editor_filesystem.hpp

  void FileSystem::clear(String&& name, const String& path)
  {
    clearImpl();
    m_Root = &makeNode(std::move(name), path, false);
  }

  FileEntry& FileSystem::makeNode(String&& name, const String& path, bool is_file)
  {
    FileEntry* const entry = m_Memory.allocateT<FileEntry>(std::move(name), path, is_file);
    m_AllNodes.push(entry);

    return *entry;
  }

  void FileSystem::uiShow(EditorOverlay& editor)
  {
    if (m_Root)
    {
      static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersHOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable;

      if (ImGui::BeginTable("File System", 2, flags))
      {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 10);
        ImGui::TableAutoHeaders();

        uiShowImpl(editor, root());
        ImGui::EndTable();
      }

      if (m_HasBeenModified)
      {
        editor.assetRefresh();
        m_HasBeenModified = false;
      }
    }
  }

  StringRange FileSystem::relativePath(const FileEntry& entry) const
  {
    static constexpr std::size_t OFFSET_FROM_SLASH = 1;

    const std::size_t full_path_length = entry.full_path.length();
    const std::size_t root_path_length = root().full_path.length();
    const std::size_t length_diff      = full_path_length - root_path_length;
    const char* const path_bgn         = entry.full_path.cstr() + root_path_length + OFFSET_FROM_SLASH;

    return {path_bgn, length_diff};
  }

  void FileSystem::remove(FileEntry& entry)
  {
    if (path::deleteDirectory(entry.full_path.cstr()))
    {
      m_HasBeenModified = true;
    }
  }

  FileSystem::~FileSystem()
  {
    clearImpl();
  }

  void FileSystem::uiShowImpl(EditorOverlay& editor, FileEntry& entry)
  {
    ImGui::TableNextRow();

    if (entry.is_file)
    {
      ImGui::TreeNodeEx(entry.name.cstr(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanFullWidth);

      const StringRange rel_path = relativePath(entry);

      if (ImGui::IsItemHovered())
      {
        ImGui::SetTooltip("Name(%s)\nFullPath(%s)\nRelPath(%.*s)", entry.name.cstr(), entry.full_path.cstr(), int(rel_path.length()), rel_path.begin());
      }

      if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
      {
      }

      if (ImGui::IsItemDeactivated() && ImGui::IsItemHovered())
      {
        bfLogPrint("Clicked On %s", entry.uuid.as_string);
      }

      const auto flags = ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoDisableHover | ImGuiDragDropFlags_SourceNoHoldToOpenOthers;

      if (ImGui::BeginDragDropSource(flags))
      {
        if constexpr (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        {
          ImGui::Text("UUID %s", entry.uuid.as_string);
        }

        ImGui::SetDragDropPayload("Asset.UUID", &entry.uuid, sizeof(BifrostUUID));
        ImGui::EndDragDropSource();
      }

      ImGui::TableNextCell();
      ImGui::TextUnformatted("Asset");

      ImGui::TreePop();
    }
    else
    {
      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;

      if (&entry == &root())
      {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
      }

      const bool is_open = ImGui::TreeNodeEx(entry.name.cstr(), flags);

      if (ImGui::BeginPopupContextItem())
      {
        if (ImGui::BeginMenu("Create"))
        {
          if (ImGui::MenuItem("Folder"))
          {
            editor.enqueueDialog(make<NewFolderDialog>(entry.full_path));
          }

          ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Delete"))
        {
          remove(entry);
        }

        ImGui::EndPopup();
      }

      ImGui::TableNextCell();
      ImGui::TextUnformatted("Folder");

      if (is_open)
      {
        for (FileEntry& child : entry.children)
        {
          uiShowImpl(editor, child);
        }

        ImGui::TreePop();
      }
    }
  }
}  // namespace bifrost::editor
