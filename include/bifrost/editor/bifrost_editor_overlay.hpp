/******************************************************************************/
/*!
* @file   bifrost_editor_overlay.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-16
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_EDITOR_OVERLAY_HPP
#define BIFROST_EDITOR_OVERLAY_HPP

#include "bifrost/asset_io/bifrost_material.hpp"
#include "bifrost/bifrost.hpp"
#include "bifrost/math/bifrost_rect2.hpp"
#include "bifrost_editor_filesystem.hpp"
#include "bifrost_editor_inspector.hpp"
#include "bifrost_editor_serializer.hpp"

namespace bifrost::editor
{
  class EditorOverlay;

  IMemoryManager& allocator();

  template<typename T>
  void deallocateT(T* ptr)
  {
    allocator().deallocateT(ptr);
  }

  struct ActionContext final
  {
    EditorOverlay* editor;

    bool actionButton(const char* name) const;
  };

  class Action
  {
   public:
    virtual void execute(const ActionContext& ctx) = 0;

    virtual bool isActive(const ActionContext& ctx) const
    {
      return true;
    }

    virtual ~Action() = default;
  };

  namespace ui
  {
    class Dialog
    {
     protected:
      bool        m_WantsToClose;
      const char* m_Name;

     public:
      explicit Dialog(const char* name) :
        m_WantsToClose{false},
        m_Name{name}
      {
      }

      [[nodiscard]] bool        wantsToClose() const { return m_WantsToClose; }
      [[nodiscard]] const char* name() const { return m_Name; }

      virtual void show(const ActionContext& ctx) = 0;

      void close()
      {
        m_WantsToClose = true;
      }

      virtual ~Dialog() = default;
    };

    class BaseMenuItem
    {
     private:
      const char* m_Name;

     protected:
      explicit BaseMenuItem(const char* name) :
        m_Name{name}
      {
      }

     public:
      virtual bool beginItem(const ActionContext& ctx) = 0;
      virtual void doAction(const ActionContext& ctx)  = 0;
      virtual void endItem()                           = 0;

      virtual ~BaseMenuItem() = default;
      [[nodiscard]] const char* name() const { return m_Name; }
    };

    class MenuDropdown : public BaseMenuItem
    {
     private:
      Array<BaseMenuItem*> m_SubItems;

     public:
      bool beginItem(const ActionContext& ctx) override;
      void doAction(const ActionContext& ctx) override;
      void endItem() override;

     public:
      MenuDropdown(const char* name, IMemoryManager& memory) :
        BaseMenuItem{name},
        m_SubItems{memory}
      {
      }

      MenuDropdown* addItem(BaseMenuItem* item);

      ~MenuDropdown() override = default;
    };

    class MainMenu final : public MenuDropdown
    {
     public:
      bool beginItem(const ActionContext& ctx) override;
      void endItem() override;

      MainMenu(const char* name, IMemoryManager& memory) :
        MenuDropdown{name, memory}
      {
      }
    };

    class MenuAction : public BaseMenuItem
    {
     public:
      bool beginItem(const ActionContext& ctx) override;
      void doAction(const ActionContext& ctx) override;
      void endItem() override;

     private:
      Action* m_Action;

     public:
      MenuAction(const char* name, Action* action) :
        BaseMenuItem{name},
        m_Action{action}
      {
      }

      ~MenuAction() override = default;
    };

  }  // namespace ui

  template<typename T>
  using UniquePtr = std::unique_ptr<T, meta::function_caller<&deallocateT<T>>>;

  class Project final
  {
   private:
    String m_Name;
    String m_ProjectFilePath;
    String m_Path;  // TODO: Make it a StringRange of 'Project::m_ProjectFilePath'
    String m_MetaPath;

   public:
    explicit Project(String&& name, String&& project_file, String&& path, String&& meta_path) :
      m_Name{name},
      m_ProjectFilePath{project_file},
      m_Path{path},
      m_MetaPath{meta_path}
    {
    }

    String&       name() { return m_Name; }
    String&       projectFilePath() { return m_ProjectFilePath; }
    const String& path() const { return m_Path; }
    const String& metaPath() const { return m_MetaPath; }
  };

  using ActionPtr  = UniquePtr<Action>;
  using ProjectPtr = UniquePtr<Project>;

  class ARefreshAsset;

  using ActionMap = HashTable<String, ActionPtr>;

  class EditorOverlay final : public IGameStateLayer
  {
    friend class ARefreshAsset;

   private:
    ui::Dialog*        m_CurrentDialog;
    bool               m_OpenNewDialog;
    ActionMap          m_Actions;
    Engine*            m_Engine;
    ProjectPtr         m_OpenProject;
    float              m_FpsTimer;
    int                m_CurrentFps;
    AssetTextureHandle m_TestTexture;
    FileSystem         m_FileSystem;
    Rect2i             m_SceneViewViewport;  // Global Window Coordinates
    Array<Inspector>   m_InspectorWindows;
    ImGuiID            m_InspectorDefaultDockspaceID;
    int                m_SceneViewGBuffer;

   protected:
    void onCreate(Engine& engine) override;
    void onLoad(Engine& engine) override;
    void onEvent(Engine& engine, Event& event) override;
    void onUpdate(Engine& engine, float delta_time) override;
    void onUnload(Engine& engine) override;
    void onDestroy(Engine& engine) override;

   public:
    EditorOverlay();

    const ProjectPtr& currentlyOpenProject() const { return m_OpenProject; }
    const char*       name() override { return "Bifrost Editor"; }
    Engine&           engine() const { return *m_Engine; }
    FileSystem&       fileSystem() { return m_FileSystem; }

    Action* findAction(const char* name) const;
    void    enqueueDialog(ui::Dialog* dlog);
    bool    openProjectDialog();
    bool    openProject(StringRange path);
    void    saveProject();
    void    closeProject();
    void    assetRefresh();
    void    viewAddInspector();
    bool    isPointOverSceneView(const Vector2i& point) const;

    template<typename T>
    void select(T&& selectable)
    {
      for (Inspector& inspector : m_InspectorWindows)
      {
        inspector.select(selectable);
      }
    }

   private:
    void buttonAction(const ActionContext& ctx, const char* action_name) const;
    void buttonAction(const ActionContext& ctx, const char* action_name, const char* custom_label, const ImVec2& size = ImVec2(0.0f, 0.0f)) const;
    void selectableAction(const ActionContext& ctx, const char* action_name) const;
    void selectableAction(const ActionContext& ctx, const char* action_name, const char* custom_label) const;
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_OVERLAY_HPP */
