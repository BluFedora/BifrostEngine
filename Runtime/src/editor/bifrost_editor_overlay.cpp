#include "bf/editor/bifrost_editor_overlay.hpp"

#include "bf/FreeListAllocator.hpp"
#include "bf/Gfx2DPainter.hpp"
#include "bf/asset_io/bf_path_manip.hpp"  // path::*
#include "bf/asset_io/bf_spritesheet_asset.hpp"
#include "bf/asset_io/bifrost_assets.hpp"
#include "bf/bifrost.hpp"
#include "bf/bifrost_imgui_glfw.hpp"
#include "bf/core/bifrost_engine.hpp"
#include "bf/data_structures/bifrost_intrusive_list.hpp"
#include "bf/editor/bifrost_editor_inspector.hpp"
#include "bf/editor/bifrost_editor_scene.hpp"
#include "bf/utility/bifrost_json.hpp"

#include <imgui/imgui.h>          /* ImGUI::* */
#include <nativefiledialog/nfd.h> /* nfd**    */

#include <imgui/imgui_internal.h>  // Must appear after '<imgui/imgui.h>'

#include <ImGuizmo/ImGuizmo.h>

#include <utility>

#include "bf/CRTAllocator.hpp"

namespace bf::editor
{
  StringPoolRef::StringPoolRef(const StringPoolRef& rhs) noexcept :
    pool{rhs.pool},
    entry_idx{rhs.entry_idx}
  {
    ++pool->m_EntryStorage[entry_idx].ref_count;
  }

  StringPoolRef::StringPoolRef(StringPoolRef&& rhs) noexcept :
    pool{rhs.pool},
    entry_idx{rhs.entry_idx}
  {
    rhs.pool = nullptr;
  }

  StringPoolRef& StringPoolRef::operator=(const StringPoolRef& rhs) noexcept
  {
    if (this != &rhs)
    {
      clear();
      pool      = rhs.pool;
      entry_idx = rhs.entry_idx;

      ++pool->m_EntryStorage[entry_idx].ref_count;
    }

    return *this;
  }

  StringPoolRef& StringPoolRef::operator=(StringPoolRef&& rhs) noexcept
  {
    clear();
    pool      = rhs.pool;
    entry_idx = rhs.entry_idx;
    rhs.pool  = nullptr;

    return *this;
  }

  const char* StringPoolRef::string() const noexcept
  {
    return pool ? pool->m_EntryStorage[entry_idx].data.begin() : nullptr;
  }

  std::size_t StringPoolRef::length() const noexcept
  {
    return pool ? pool->m_EntryStorage[entry_idx].data.length() : 0;
  }

  void StringPoolRef::clear() noexcept
  {
    try
    {
      if (pool)
      {
        if (--pool->m_EntryStorage[entry_idx].ref_count == 0)
        {
          pool->m_Table.remove(pool->m_EntryStorage[entry_idx].data);
          pool->m_EntryStorage.memory().deallocate(const_cast<char*>(pool->m_EntryStorage[entry_idx].data.bgn), length() + 1);
          pool->m_EntryStorage[entry_idx].free_list_next = pool->m_EntryStorageFreeList;
          pool->m_EntryStorageFreeList                   = entry_idx;
        }

        pool = nullptr;
      }
    }
    catch (...)
    {
    }
  }

  StringPoolRef::~StringPoolRef() noexcept
  {
    clear();
  }
}  // namespace bf::editor

/////////////////////////////////////////////////////////////////////////////////////

using namespace bf;

// clang-format off
template<std::size_t Size, typename TAllocator = bf::FreeListAllocator>
class BlockAllocator final : public IMemoryManager, private NonCopyMoveable<BlockAllocator<Size>>
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
    assert(size <= Size);  // TODO(Shareef): Handle the case of If Size <= size.

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

  void deallocate(void* ptr, std::size_t num_bytes) override
  {
    MemoryBlock* cursor = &m_SmallBacking;

    const char* const ptr_char = static_cast<const char*>(ptr);

    while (cursor)
    {
      const char* const cursor_bgn = cursor->memory_backing;
      const char* const cursor_end = cursor->memory_backing + Size;

      if (cursor_bgn <= ptr_char && ptr_char < cursor_end)
      {
        cursor->allocator.deallocate(ptr, num_bytes);
        return;
      }

      cursor = cursor->next;
    }

    assert(!"BlockAllocator::deallocate invalid pointer passed in.");
  }

  // ReSharper disable once CppHidingFunction
  virtual ~BlockAllocator()
  {
    MemoryBlock* cursor = m_SmallBacking.next;

    while (cursor)
    {
      MemoryBlock* const next = cursor->next;
      m_BlockAllocator.deallocateT(cursor);
      cursor = next;
    }
  }
};

namespace bf::editor
{
  static char              s_EditorMemoryBacking[bfMegabytes(16)];
  static FreeListAllocator s_EditorMemory{s_EditorMemoryBacking, sizeof(s_EditorMemoryBacking)};

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

  namespace ui
  {
    MenuDropdown* makeDropdown(const StringPoolRef& name)
    {
      return make<MenuDropdown>(name, s_EditorMemory);
    }

    MenuAction* makeAction(const StringPoolRef& name, Action* action = nullptr)
    {
      return make<MenuAction>(name, action);
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

  class NewProjectDialog : public ui::Dialog
  {
   private:
    char m_ProjectName[256];
    char m_ProjectPath[256];

   public:
    explicit NewProjectDialog() :
      Dialog("New Project"),
      m_ProjectName{"New Game Project"},
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

            const json::Value json_data = {
             json::Pair{"Name", m_ProjectName},
             json::Pair{"Path", m_ProjectPath},
            };

            String json_str;
            toString(json_data, json_str);

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
    char   m_FolderName[120];  //!< 120 is the max folder name length on windows.

   public:
    explicit NewFolderDialog(String base_path) :
      Dialog("New Folder"),
      m_BasePath{std::move(base_path)},
      m_FolderName{"FolderName"}
    {
    }

    void show(const ActionContext& ctx) override
    {
      ImGui::PushID(this);

      if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
        ImGui::SetKeyboardFocusHere(0);

      ImGui::SetItemDefaultFocus();
      const bool enter_hit = ImGui::InputText("Name", m_FolderName, sizeof(m_FolderName), ImGuiInputTextFlags_EnterReturnsTrue);

      ImGui::Separator();

      if (file::isValidName(m_FolderName))
      {
        if (enter_hit || ImGui::Button("Create"))
        {
          String full_path{m_BasePath};

          full_path.append('/');
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

      ImGui::PopID();
    }
  };

  class FolderRenameDialog : public ui::Dialog
  {
   private:
    FileEntry& m_FileEntry;
    char       m_FolderName[120];  // 120 is the max folder name length on windows.

   public:
    explicit FolderRenameDialog(FileEntry& file_entry) :
      Dialog("Rename Folder"),
      m_FileEntry{file_entry},
      m_FolderName{"_"}
    {
      std::strcpy(m_FolderName, m_FileEntry.name.c_str());
    }

    void show(const ActionContext& ctx) override
    {
      ImGui::PushID(this);

      if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
        ImGui::SetKeyboardFocusHere(0);

      ImGui::SetItemDefaultFocus();
      const bool enter_hit = ImGui::InputText("Name", m_FolderName, sizeof(m_FolderName), ImGuiInputTextFlags_EnterReturnsTrue);

      ImGui::Separator();

      if (file::isValidName(m_FolderName))
      {
        if (enter_hit || ImGui::Button("Rename"))
        {
          if (path::renameDirectory(m_FileEntry.full_path.cstr(), m_FolderName))
          {
            const StringRange base_path = path::directory(m_FileEntry.full_path);

            m_FileEntry.name = m_FolderName;
            m_FileEntry.full_path.resize(base_path.length() + 1);  // The + 1 adds the '/'
            m_FileEntry.full_path.append(m_FileEntry.name);
          }
          else
          {
            bfLogError("Failed to rename Folder: %s", m_FileEntry.full_path.cstr());
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

      ImGui::PopID();
    }
  };

  template<typename TAssetInfo>
  class NewAssetDialog : public ui::Dialog
  {
   private:
    FileEntry&  m_FileEntry;
    char        m_AssetName[120];
    StringRange m_Extension;

   public:
    explicit NewAssetDialog(const char* dialog_name, FileEntry& file_entry, const StringRange& default_name, const StringRange& ext) :
      Dialog(dialog_name),
      m_FileEntry{file_entry},
      m_AssetName{"_"},
      m_Extension{ext}
    {
      std::strncpy(m_AssetName, default_name.begin(), default_name.length());
    }

    void show(const ActionContext& ctx) override
    {
      ImGui::PushID(this);

      if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
        ImGui::SetKeyboardFocusHere(0);

      ImGui::SetItemDefaultFocus();
      const bool enter_hit = ImGui::InputText("Name", m_AssetName, sizeof(m_AssetName), ImGuiInputTextFlags_EnterReturnsTrue);

      ImGui::Separator();

      if (file::isValidName(m_AssetName))
      {
        if (enter_hit || ImGui::Button("Create"))
        {
          Assets&      assets        = ctx.editor->engine().assets();
          const String file_name     = "/" + (m_AssetName + m_Extension);
          const String abs_file_path = m_FileEntry.full_path + file_name;

          if (assets.writeJsonToFile(abs_file_path, json::Object{}))
          {
            assets.loadAsset(abs_file_path);
            assets.saveAssets();
            ctx.editor->assetRefresh();
          }
          else
          {
            bfLogError("Failed to create asset: %s", m_AssetName);
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

      ImGui::PopID();
    }
  };

  bool ui::MenuDropdown::beginItem(const ActionContext& ctx)
  {
    return ImGui::BeginMenu(name().string());
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
    return ImGui::MenuItem(name().string(), nullptr, false, m_Action && m_Action->isActive(ctx));
  }

  void ui::MenuAction::doAction(const ActionContext& ctx)
  {
    m_Action->execute(ctx);
  }

  void ui::MenuAction::endItem()
  {
  }

  class ASaveProject final : public MemberAction<void>
  {
   public:
    explicit ASaveProject() :
      MemberAction<void>{&EditorOverlay::saveProject}
    {
    }

    bool isActive(const ActionContext& ctx) const override
    {
      return ctx.editor->currentlyOpenProject() != nullptr;
    }
  };

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

  void EditorOverlay::onCreate(Engine& engine)
  {
    m_Engine = &engine;

    imgui::startup(engine.renderer().context(), m_MainWindow);

    ImGuiStyle&   style  = ImGui::GetStyle();
    ImVec4* const colors = style.Colors;

    style.FrameRounding       = 2.0f;
    style.FramePadding        = ImVec2(4.0f, 2.0f);
    style.FrameBorderSize     = 1.0f;
    style.WindowBorderSize    = 1.0f;
    style.WindowPadding       = ImVec2(5.0f, 5.0f);
    style.WindowRounding      = 3.0f;
    style.WindowTitleAlign    = ImVec2(0.5f, 0.5f);
    style.ChildRounding       = 2.0f;
    style.GrabMinSize         = 6.0f;
    style.GrabRounding        = 2.0f;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ItemSpacing         = ImVec2(4.0f, 4.0f);
    style.IndentSpacing       = 12.0f;
    style.PopupRounding       = 2.0f;
    style.ScrollbarRounding   = 3.0f;
    style.TabRounding         = 2.0f;
    style.WindowMinSize.x     = 80.0f;

    colors[ImGuiCol_Text]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_WindowBg]           = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.06f, 0.06f, 0.07f, 0.54f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_Border]             = ImVec4(0.09f, 0.05f, 0.11f, 0.73f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.00f, 0.00f, 0.00f, 0.66f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ResizeGrip]         = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.29f, 0.28f, 0.33f, 0.81f);
    colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.16f, 0.15f, 0.20f, 0.95f);
    colors[ImGuiCol_Tab]                = ImVec4(0.12f, 0.09f, 0.16f, 0.86f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.41f, 0.39f, 0.44f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.08f, 0.08f, 0.09f, 0.31f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.26f, 0.26f, 0.27f, 0.80f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.14f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.63f, 0.65f, 0.68f, 0.44f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.37f, 0.38f, 0.40f, 0.89f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.75f, 0.75f, 0.77f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.51f, 0.53f, 0.71f, 0.40f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.45f, 0.43f, 0.52f, 0.86f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.26f, 0.24f, 0.30f, 0.82f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.11f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.59f, 0.57f, 0.65f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.62f, 0.62f, 0.62f, 0.78f);
    colors[ImGuiCol_SeparatorActive]    = ImVec4(0.66f, 0.66f, 0.66f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.48f, 0.45f, 0.53f, 0.80f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.22f, 0.22f, 0.27f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.28f, 0.27f, 0.35f, 1.00f);
    colors[ImGuiCol_DockingPreview]     = ImVec4(0.19f, 0.31f, 0.33f, 0.70f);
    colors[ImGuiCol_TextSelectedBg]     = ImVec4(0.44f, 0.58f, 0.61f, 0.35f);
    colors[ImGuiCol_DragDropTarget]     = ImVec4(0.52f, 0.56f, 0.63f, 0.90f);

    m_Actions.emplace("File.New.Project", ActionPtr(make<ShowDialogAction<NewProjectDialog>>()));
    m_Actions.emplace("File.Open.Project", ActionPtr(make<MemberAction<bool>>(&EditorOverlay::openProjectDialog)));
    m_Actions.emplace("File.Save.Project", ActionPtr(make<ASaveProject>()));
    m_Actions.emplace("Project.Close", ActionPtr(make<ACloseProject>()));
    m_Actions.emplace("Asset.Refresh", ActionPtr(make<ARefreshAsset>()));
    m_Actions.emplace("View.AddInspector", ActionPtr(make<MemberAction<void>>(&EditorOverlay::viewAddInspector)));
    m_Actions.emplace("View.HierarchyView", ActionPtr(make<MemberAction<HierarchyView&>>(&EditorOverlay::getWindow<HierarchyView>)));
    m_Actions.emplace("View.GameView", ActionPtr(make<MemberAction<GameView&>>(&EditorOverlay::getWindow<GameView>)));

    addMenuItem("File/New/Project", "File.New.Project");
    addMenuItem("File/Open/Project", "File.Open.Project");
    addMenuItem("File/Save/Project", "File.Save.Project");
    addMenuItem("File/Close Project", "Project.Close");

    addMenuItem("Assets/Refresh", "Asset.Refresh");
    addMenuItem("Window/Inspector View", "View.AddInspector");
    addMenuItem("Window/Hierarchy View", "View.HierarchyView");
    addMenuItem("Window/Game View", "View.GameView");

    InspectorRegistry::overrideInspector<MeshRenderer>(
     [](ImGuiSerializer& serializer, meta::MetaVariant& object, void* user_data) {
       MeshRenderer* mesh_renderer = meta::variantToCompatibleT<MeshRenderer*>(object);

       ImGui::Text("This is a custom Mesh Renderer Callback");

       serializer.serializeT(mesh_renderer);

       ImGui::Text("This is a custom Mesh Renderer Callback");
     });

    InspectorRegistry::overrideInspector<SpriteAnimator>([](ImGuiSerializer& serializer, meta::MetaVariant& object, void* user_data) {
      SpriteAnimator* sprite_animator = meta::variantToCompatibleT<SpriteAnimator*>(object);

      serializer.serializeT(sprite_animator);

      ARC<SpritesheetAsset> sheet = sprite_animator->spritesheet();

      if (sheet)
      {
        bfSpritesheet* const ss            = sheet->spritesheet();
        const auto           sprite_handle = sprite_animator->animatedSprite();
        const char*          preview_str   = "No Animation Selected";

        bfAnim2DSpriteState sprite_state;

        if (bfAnim2DSprite_grabState(sprite_handle, &sprite_state))
        {
          preview_str = sprite_state.animation->name.str;
        }

        if (ImGui::BeginCombo("Animations", preview_str, ImGuiComboFlags_None))
        {
          for (uint32_t i = 0; i < ss->num_animations; ++i)
          {
            const bfAnimation* const anim = ss->animations + i;

            if (ImGui::Selectable(anim->name.str))
            {
              bfAnim2DSprite_setSpritesheet(sprite_handle, ss);

              bfAnim2DPlayExOptions play_options;

              play_options.animation         = anim;
              play_options.playback_speed    = 1.0f;
              play_options.start_frame       = 0;
              play_options.is_looping        = true;
              play_options.does_ping_ponging = false;
              play_options.force_restart     = false;

              bfAnim2DSprite_playAnimationEx(sprite_handle, &play_options);
            }
          }

          ImGui::EndCombo();
        }
      }
    });
  }

  void EditorOverlay::onLoad(Engine& engine)
  {
    engine.setState(EngineState::EDITOR_PLAYING);
  }

  void EditorOverlay::onEvent(Engine& engine, Event& event)
  {
    imgui::onEvent(event.receiver, event);

    if (event.isFalsified())
    {
      return;
    }

    ImGuiIO&   io                = ImGui::GetIO();
    const bool imgui_wants_input = (io.WantTextInput && event.isKeyEvent()) ||
                                   (io.WantCaptureMouse && event.isMouseEvent());

    for (BaseEditorWindowPtr& window : m_OpenWindows)
    {
      window->handleEvent(*this, event);
    }

    if (event.type == BIFROST_EVT_ON_KEY_DOWN)
    {
      if (event.keyboard.key == BIFROST_KEY_ESCAPE)
      {
        if (m_CurrentDialog)
        {
          m_CurrentDialog->close();
          event.accept();
        }
      }
    }

    if (event.type == BIFROST_EVT_ON_WINDOW_RESIZE || imgui_wants_input)
    {
      event.accept();
    }
    else
    {
      const auto is_key_down = event.type == BIFROST_EVT_ON_KEY_DOWN;

      if (is_key_down || event.type == BIFROST_EVT_ON_KEY_UP)
      {
        if (event.keyboard.key < int(bfCArraySize(m_IsKeyDown)))
        {
          m_IsKeyDown[event.keyboard.key] = is_key_down;
        }

        m_IsShiftDown = event.keyboard.modifiers & BIFROST_KEY_FLAG_SHIFT;
      }
    }
  }

  void EditorOverlay::onUpdate(Engine& engine, float delta_time)
  {
    int window_width, window_height;
    bfWindow_getSize(m_MainWindow, &window_width, &window_height);

    imgui::beginFrame(
     engine.renderer().surface(),
     float(window_width),
     float(window_height),
     delta_time);

    // ImGui::ShowDemoWindow();

    const ActionContext action_ctx{this};

    ImGuizmo::BeginFrame();

    if (m_MainMenu.beginItem(action_ctx))
    {
      static bool s_ShowFPS = true;

      m_MainMenu.doAction(action_ctx);

      m_FpsTimer -= delta_time;

      if (m_FpsTimer <= 0.0f)
      {
        m_CurrentFps = int(1.0f / delta_time);
        m_CurrentMs  = int(delta_time * 1000.0f);
        m_FpsTimer   = 1.0f;
      }

      {
        LinearAllocatorScope mem_scope{engine.tempMemory()};
        if (s_ShowFPS)
        {
          char* buffer = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "| %ifps | Memory (bytes) (%i / %i) |", m_CurrentFps, s_EditorMemory.usedMemory(), s_EditorMemory.size());

          if (ImGui::Selectable(buffer, &s_ShowFPS, ImGuiSelectableFlags_None, ImVec2(ImGui::CalcTextSize(buffer).x, 0.0f)))
          {
          }
        }
        else
        {
          char* buffer = string_utils::fmtAlloc(engine.tempMemory(), nullptr, "| %ims | Memory (%i / %i) |", m_CurrentMs, s_EditorMemory.usedMemory(), s_EditorMemory.size());

          if (ImGui::Selectable(buffer, &s_ShowFPS, ImGuiSelectableFlags_None, ImVec2(ImGui::CalcTextSize(buffer).x, 0.0f)))
          {
          }
        }
      }

      m_MainMenu.endItem();
    }

    // Dock Space
    {
      static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoWindowMenuButton;

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

      ImGui::Begin("Main DockSpace", nullptr, window_flags);

      ImGui::PopStyleVar(3);

      const ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");

      if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)  // Initial layout
      {
        LinearAllocatorScope mem_scope{engine.tempMemory()};

        ImGui::DockBuilderRemoveNode(dockspace_id);                             // Clear out existing layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);  // Add empty node
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        ImGuiID       dock_main_id        = dockspace_id;
        ImGuiID       dock_id_left_top    = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
        const ImGuiID dock_id_left_bottom = ImGui::DockBuilderSplitNode(dock_id_left_top, ImGuiDir_Down, 0.5f, nullptr, &dock_id_left_top);
        const ImGuiID dock_id_right       = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);

        HierarchyView& hierarchy_window = getWindow<HierarchyView>();
        Inspector&     inspector_window = getWindow<Inspector>(allocator());
        GameView&      game_window      = getWindow<GameView>();
        SceneView&     scene_window     = getWindow<SceneView>();

        ImGui::DockBuilderDockWindow("Project View", dock_id_left_top);
        ImGui::DockBuilderDockWindow(hierarchy_window.fullImGuiTitle(engine.tempMemory()), dock_id_left_bottom);
        ImGui::DockBuilderDockWindow(inspector_window.fullImGuiTitle(engine.tempMemory()), dock_id_right);
        ImGui::DockBuilderDockWindow(game_window.fullImGuiTitle(engine.tempMemory()), dock_main_id);
        ImGui::DockBuilderDockWindow(scene_window.fullImGuiTitle(engine.tempMemory()), dock_main_id);

        ImGui::DockBuilderFinish(dockspace_id);
      }

      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

      ImGui::End();
    }

    if (m_OpenProject)
    {
      const auto current_scene = engine.currentScene();

      if (ImGui::Begin("Project View"))
      {
        if (imgui_ext::inspect("Project Name", m_OpenProject->name()))
        {
        }

        ImGui::Separator();

        m_FileSystem.uiShow(*this);

        ImGui::Separator();
      }
      ImGui::End();
    }

    // TODO(SR): These two loops can probably be combined.
    for (BaseEditorWindowPtr& window : m_OpenWindows)
    {
      window->update(*this, delta_time);
    }

    for (BaseEditorWindowPtr& window : m_OpenWindows)
    {
      window->uiShow(*this);
    }

    auto       open_windows_bgn = m_OpenWindows.begin();
    const auto open_windows_end = m_OpenWindows.end();

    // TODO(SR): Actually check if any windows want to be closed. This is very pessimistic.
    const auto split = std::partition(
     open_windows_bgn,
     open_windows_end,
     [](const BaseEditorWindowPtr& window) {
       return window->isOpen();
     });

    for (auto closed_window = split; closed_window != open_windows_end; ++closed_window)
    {
      (*closed_window)->onDestroy(*this);
    }

    m_OpenWindows.resize(split - open_windows_bgn);

    /*
    if (ImGui::Begin("Command Palette"))
    {
      for (const auto& action : m_Actions)
      {
        buttonAction(action_ctx, action.key().cstr(), action.key().cstr(), ImVec2(-1.0f, 0.0f));
      }
    }
    ImGui::End();

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

  void EditorOverlay::onDraw2D(Engine& engine)
  {
    imgui::endFrame();
  }

  void EditorOverlay::onUnload(Engine& engine)
  {
    for (BaseEditorWindowPtr& window : m_OpenWindows)
    {
      window->onDestroy(*this);
    }

    m_OpenWindows.clear();
  }

  void EditorOverlay::onDestroy(Engine& engine)
  {
    imgui::shutdown();
    enqueueDialog(nullptr);
  }

  EditorOverlay::EditorOverlay(bfWindow* main_window) :
    m_CurrentDialog{nullptr},
    m_OpenNewDialog{false},
    m_Actions{},
    m_MenuNameStringPool{allocator()},
    m_MainMenu{m_MenuNameStringPool.intern("__MainMenu__"), allocator()},
    m_Engine{nullptr},
    m_OpenProject{nullptr},
    m_FpsTimer{0.0f},
    m_CurrentFps{0},
    m_CurrentMs{0},
    m_FileSystem{allocator()},
    m_OpenWindows{allocator()},
    m_IsKeyDown{false},
    m_IsShiftDown{false},
    m_Selection{allocator()},
    m_MainUndoStack{},
    m_MainWindow{main_window}
  {
  }

  Action* EditorOverlay::findAction(const char* name) const
  {
    ActionPtr* action_ptr = name ? m_Actions.at(name) : nullptr;

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
    // const nfdchar_t* filter = "project.json";
    const nfdchar_t* filter = nullptr;

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
      LinearAllocatorScope temp_mem_scope = m_Engine->tempMemory();

      if (currentlyOpenProject())
      {
        closeProject();
      }

      const StringRange project_dir      = path::directory(path);
      const TempBuffer  project_json_str = project_file.readAll(m_Engine->tempMemoryNoFree());
      const AssetError  err              = m_Engine->assets().setRootPath(std::string_view{project_dir.bgn, project_dir.length()});

      if (err == AssetError::NONE)
      {
        const json::Value project_json = json::fromString(project_json_str.buffer(), project_json_str.size());

        assert(project_json.isObject());

        const json::Value* const project_name = project_json.at("Name");

        if (project_name && project_name->isString())
        {
          const auto& project_name_str = project_name->as<json::String>();

          m_OpenProject.reset(make<Project>(String(project_name_str.c_str()), path, project_dir));

          assetRefresh();
          return true;
        }
      }
    }

    return false;
  }

  void EditorOverlay::saveProject()
  {
    m_Engine->assets().saveAssets();

    File file{m_OpenProject->projectFilePath(), file::FILE_MODE_WRITE};

    if (file)
    {
      using namespace json;

      const Value project_save_data = {
       Pair{"Name", m_OpenProject->name()},
       Pair{"Path", m_OpenProject->path()},
      };

      ::String json_data;
      toString(project_save_data, json_data);

      file.writeBytes(json_data.cstr(), json_data.size());
      file.close();
    }
  }

  void EditorOverlay::closeProject()
  {
    select(nullptr);
    m_Engine->openScene(nullptr);
    m_OpenProject.reset(nullptr);
  }

  // Asset Management

  struct MetaAssetPath final
  {
    char*      file_name{nullptr};
    FileEntry* entry{nullptr};
  };

  static void assetFindAssets(List<MetaAssetPath>& metas, const String& path, const String& current_string, FileSystem& filesystem, FileEntry& parent_entry);

  void EditorOverlay::assetRefresh()
  {
    const auto& path = m_OpenProject->path();

    if (path::doesExist(path.cstr()))
    {
      Assets&              assets = m_Engine->assets();
      BlockAllocator<8192> allocator{editor::allocator()};
      List<MetaAssetPath>  metas_to_check{allocator};

      m_FileSystem.clear("Assets", path);
      assetFindAssets(metas_to_check, path, "", m_FileSystem, m_FileSystem.root());

      for (const auto& meta : metas_to_check)
      {
        file::canonicalizePath(meta.file_name, meta.file_name + std::strlen(meta.file_name));

        IBaseAsset* const asset = assets.findAsset(AbsPath(meta.file_name));

        meta.entry->asset_info = asset;

        if (!asset)
        {
          bfLogWarn("Unknown file type (%s)", meta.file_name);
        }

        string_utils::fmtFree(allocator, meta.file_name);
      }
    }
  }

  void EditorOverlay::viewAddInspector()
  {
    getWindow<Inspector>(allocator());
  }

  void EditorOverlay::buttonAction(const ActionContext& ctx, const char* action_name) const
  {
    buttonAction(ctx, action_name, action_name);
  }

  void EditorOverlay::buttonAction(const ActionContext& ctx, const char* action_name, const char* custom_label, const ImVec2& size) const
  {
    Action* const action      = findAction(action_name);
    const bool    is_disabled = action == nullptr || !action->isActive(ctx);

    if (is_disabled)
    {
      ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
      ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if (ImGui::Button(custom_label, size))
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

  void EditorOverlay::selectableAction(const ActionContext& ctx, const char* action_name) const
  {
    selectableAction(ctx, action_name, action_name);
  }

  void EditorOverlay::selectableAction(const ActionContext& ctx, const char* action_name, const char* custom_label) const
  {
    Action* const              action           = findAction(action_name);
    const bool                 is_disabled      = action == nullptr || !action->isActive(ctx);
    const ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_None | ImGuiSelectableFlags_Disabled * is_disabled;

    if (ImGui::Selectable(custom_label, false, selectable_flags))
    {
      action->execute(ctx);
    }
  }

  void EditorOverlay::addMenuItem(const StringRange& menu_path, const char* action_name)
  {
    LinearAllocatorScope mem_scope        = {m_Engine->tempMemory()};
    IMemoryManager&      token_allocator  = m_Engine->tempMemoryNoFree();
    ui::MenuDropdown*    current_dropdown = &m_MainMenu;
    const TokenizeResult tokens           = string_utils::tokenizeAlloc(token_allocator, menu_path, '/');
    StringLink*          link_start       = tokens.head;

    assert(tokens.size > 0 && "This is not a valid path for a menu item.");

    while (link_start != tokens.tail)
    {
      ui::MenuDropdown* new_dropdown = current_dropdown->findDropdown(link_start->string);

      if (!new_dropdown)
      {
        new_dropdown = ui::makeDropdown(m_MenuNameStringPool.intern(link_start->string));

        current_dropdown->addItem(new_dropdown);
      }

      current_dropdown = new_dropdown;

      link_start = link_start->next;
    }

    current_dropdown->addItem(ui::makeAction(m_MenuNameStringPool.intern(tokens.tail->string), findAction(action_name)));

    string_utils::tokenizeFree(token_allocator, tokens);
  }

  static void assetFindAssets(List<MetaAssetPath>& metas, const String& path, const String& current_string, FileSystem& filesystem, FileEntry& parent_entry)
  {
    CRTAllocator          mallocator;
    path::DirectoryEntry* dir = path::openDirectory(mallocator, path);

    if (dir)
    {
      do
      {
        const StringRange name = StringRange(entryFilename(dir));

        if (!path::startWith(name, ".") && file::extensionOfFile(name) != Assets::k_MetaFileExtension)
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

            m.file_name = string_utils::fmtAlloc(metas.memory(), nullptr, "%s/%s", path.cstr(), name.begin());
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

  // bifrost_editor_filesystem.hpp

  FileEntry::FileEntry(String&& name, const String& full_path, bool is_file) :
    name{name},
    full_path{full_path},
    file_extension{file::extensionOfFile(this->full_path)},
    children{&FileEntry::next},
    next{},
    asset_info{nullptr},
    is_file{is_file}
  {
  }

  FileSystem::FileSystem(IMemoryManager& memory) :
    m_Memory{memory},
    m_AllNodes{memory},
    m_Root{nullptr},
    m_RenamedNode{nullptr},
    m_HasBeenModified{false}
  {
  }

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
      static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_Hideable | ImGuiTableFlags_BordersHOuter | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Hideable;

      if (ImGui::BeginTable("File System", 2, flags))
      {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
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

  static StringRange bufferToStr(const TempBuffer& buffer)
  {
    return {buffer.buffer(), buffer.size()};
  }

#if 0
  void FileSystem::rename(EditorOverlay& editor, FileEntry& entry, const StringRange& new_name) const
  {
    Engine&         engine      = editor.engine();
    IMemoryManager& tmp_no_free = engine.tempMemoryNoFree();

    if (entry.is_file)
    {
      Assets&              assets               = engine.assets();
      LinearAllocatorScope mem_scope            = engine.tempMemory();
      const StringRange    old_rel_path         = path::relative(root().full_path, entry.full_path);
      std::size_t          old_meta_name_length = 0;
      const char*          old_meta_name        = assets.metaFileName(tmp_no_free, old_rel_path, old_meta_name_length);
      const TempBuffer     old_meta_path        = assets.metaFullPath(tmp_no_free, old_meta_name);
      const StringRange    path_root_dir        = path::directory(bufferToStr(old_meta_path));
      std::size_t          new_path_length      = 0;
      char* const          new_path             = string_utils::fmtAlloc(
       tmp_no_free,
       &new_path_length,
       "%*.s/%*.s.%*.s",
       int(path_root_dir.length()),
       path_root_dir.begin(),
       int(new_name.length()),
       new_name.begin(),
       int(entry.file_extension.length()),
       entry.file_extension.begin());

      if (path::renameFile({old_meta_path.buffer(), old_meta_path.size()}, {new_path, new_path_length}))
      {
        entry = FileEntry(new_name, new_path, true);
      }
    }
    else
    {
      char* const new_path = string_utils::fmtAlloc(
       tmp_no_free,
       nullptr,
       "%*.s.%*.s",
       int(new_name.length()),
       new_name.begin(),
       int(entry.file_extension.length()),
       entry.file_extension.begin());

      if (path::renameDirectory(entry.full_path.c_str(), new_path))
      {
        entry = FileEntry(new_name, new_path, false);
      }
    }
  }
#endif

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

    if (entry.isFile())
    {
      ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_SpanFullWidth /*| ImGuiTreeNodeFlags_DefaultOpen*/;

      if (entry.asset_info && !entry.asset_info->hasSubAssets())
      {
        tree_node_flags |= ImGuiTreeNodeFlags_Bullet;
      }

      if (!entry.asset_info || !entry.asset_info->hasSubAssets())
      {
        tree_node_flags |= ImGuiTreeNodeFlags_Leaf;
      }

      if (entry.asset_info)
      {
        ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
      }

      const bool is_open = ImGui::TreeNodeEx(entry.name.cstr(), tree_node_flags);

      if (entry.asset_info)
      {
        ImGui::PopStyleColor();
      }

      if (ImGui::IsItemHovered())
      {
        BifrostUUIDString uuid_str;

        if (entry.asset_info)
        {
          bfUUID_numberToString(entry.asset_info->uuid().data, uuid_str.data);
        }

        ImGui::SetTooltip("Asset(%s)", entry.asset_info ? uuid_str.data : "<null>");
      }

      if (entry.asset_info)
      {
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
          if (entry.file_extension == ".scene")
          {
            editor.engine().openScene(static_cast<SceneAsset*>(entry.asset_info));
          }
        }

        if (ImGui::IsItemDeactivated() && ImGui::IsItemHovered())
        {
          editor.select(entry.asset_info);
        }
      }

      const auto flags = ImGuiDragDropFlags_SourceAllowNullID | ImGuiDragDropFlags_SourceNoDisableHover | ImGuiDragDropFlags_SourceNoHoldToOpenOthers;

      if (entry.asset_info)
      {
        if (ImGui::BeginDragDropSource(flags))
        {
          if constexpr (!(flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
          {
            BifrostUUIDString uuid_str;

            bfUUID_numberToString(entry.asset_info->uuid().data, uuid_str.data);

            ImGui::Text("UUID %s", uuid_str.data);
          }

          ImGui::SetDragDropPayload("Asset.UUID", &entry.asset_info->uuid(), sizeof(bfUUID));
          ImGui::EndDragDropSource();
        }
      }

      if (ImGui::BeginPopupContextItem())
      {
        if (ImGui::MenuItem("Rename"))
        {
          editor.enqueueDialog(make<FolderRenameDialog>(entry));
        }

        if (ImGui::MenuItem("Delete"))
        {
          remove(entry);
        }

        ImGui::EndPopup();
      }

      ImGui::TableNextCell();
      ImGui::TextUnformatted("Asset");

      if (is_open)
      {
        if (entry.asset_info)
        {
          for (const IBaseAsset& sub_asset : entry.asset_info->subAssets())
          {
            ImGui::TableNextRow();

            ImGui::PushStyleColor(ImGuiCol_Text, 0xFFFF0FF0);

            if (ImGui::TreeNodeEx(sub_asset.name().begin(), ImGuiTreeNodeFlags_Bullet | ImGuiTreeNodeFlags_SpanFullWidth))
            {
              ImGui::TreePop();
            }

            ImGui::PopStyleColor();

            ImGui::TableNextCell();
            ImGui::TextUnformatted("SubAsset");
          }
        }

        ImGui::TreePop();
      }
    }
    else
    {
      ImGuiTreeNodeFlags tree_node_flags   = ImGuiTreeNodeFlags_SpanFullWidth;
      const auto         drag_source_flags = ImGuiDragDropFlags_SourceNoHoldToOpenOthers;
      FileEntry* const   entry_ptr         = &entry;
      const bool         is_root           = entry_ptr == &root();

      if (is_root)
      {
        tree_node_flags |= ImGuiTreeNodeFlags_DefaultOpen;
      }

      const bool is_open = ImGui::TreeNodeEx(entry.name.cstr(), tree_node_flags);

      if (ImGui::BeginDragDropSource(drag_source_flags))
      {
        if constexpr (!(drag_source_flags & ImGuiDragDropFlags_SourceNoPreviewTooltip))
        {
          ImGui::Text("Moving Folder %s", entry.name.c_str());
        }

        ImGui::SetDragDropPayload("FileSystem.Folder", &entry_ptr, sizeof(FileEntry*));
        ImGui::EndDragDropSource();
      }

      if (ImGui::BeginDragDropTarget())
      {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        if (payload && payload->IsDataType("FileSystem.Folder"))
        {
          FileEntry* const data = *static_cast<FileEntry**>(payload->Data);

          assert(payload->DataSize == sizeof(data));

          if (data != entry_ptr && ImGui::AcceptDragDropPayload("FileSystem.Folder", ImGuiDragDropFlags_None))
          {
            const char* const destination_path = entry.full_path.c_str();
            const char* const source_path      = data->full_path.c_str();

            if (path::moveDirectory(destination_path, source_path))
            {
              m_HasBeenModified = true;
            }
          }
        }

        ImGui::EndDragDropTarget();
      }

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

        if (ImGui::MenuItem("Rename"))
        {
          editor.enqueueDialog(make<FolderRenameDialog>(entry));
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

  void FileSystem::clearImpl()
  {
    for (FileEntry* const entry : m_AllNodes)
    {
      m_Memory.deallocateT(entry);
    }

    m_AllNodes.clear();
  }

  Inspector::Inspector(IMemoryManager& memory) :
    m_LockedSelection{memory},
    m_Serializer{memory},
    m_IsLocked{false}
  {
  }

  void Inspector::onDrawGUI(EditorOverlay& editor)
  {
    Engine& engine = editor.engine();

    m_Serializer.setAssets(&engine.assets());

    if (ImGui::BeginMenuBar())
    {
      if (ImGui::BeginMenu("Options"))
      {
        if (ImGui::MenuItem("Is Selection Locked", nullptr, &m_IsLocked))
        {
          if (m_IsLocked)
          {
            m_LockedSelection = editor.selection().selectables();
          }
        }

        ImGui::EndMenu();
      }

      ImGui::EndMenuBar();
    }

    const auto&       selection      = m_IsLocked ? m_LockedSelection : editor.selection().selectables();
    const std::size_t selection_size = selection.size();

    m_Serializer.beginDocument(false);

    if (selection.isEmpty())
    {
      ImGui::Text("(No Selection)");
    }
    else if (selection.size() == 1)
    {
      guiDrawSelection(engine, selection[0]);
    }
    else
    {
      for (std::size_t i = 0; i < selection_size; ++i)
      {
        char number_buffer[22];
        string_utils::fmtBuffer(number_buffer, sizeof(number_buffer), nullptr, "%i", int(i));

        if (ImGui::TreeNode(number_buffer))
        {
          guiDrawSelection(engine, selection[i]);
          ImGui::Separator();

          ImGui::TreePop();
        }
      }
    }

    if (m_IsLocked)
    {
      if (ImGui::Button("Clear Locked Selection"))
      {
        m_LockedSelection.clear();
      }
    }

    m_Serializer.endDocument();
  }

  void Inspector::guiDrawSelection(Engine& engine, const Selectable& selectable)
  {
    ARC<SceneAsset> current_scene = engine.currentScene();

    visit_all(
     meta::overloaded{
      [this, &engine](IBaseAsset* asset) {
        m_Serializer.beginChangeCheck();
        asset->reflect(m_Serializer);
        ImGui::Separator();

        if (m_Serializer.endChangedCheck())
        {
          engine.assets().markDirty(asset);
        }
      },
      [this](IBaseObject* object) {
        m_Serializer.serialize(*object);
      },
      [this, &current_scene, &engine](Entity* object) {
        m_Serializer.beginChangeCheck();

        imgui_ext::inspect(engine, *object, m_Serializer);

        if (m_Serializer.endChangedCheck())
        {
          engine.assets().markDirty(current_scene.handle());
        }
      },
     },
     selectable);
  }
}  // namespace bf::editor
