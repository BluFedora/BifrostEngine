#ifndef BIFROST_EDITOR_OVERLAY_HPP
#define BIFROST_EDITOR_OVERLAY_HPP

#include "bifrost/bifrost.hpp"

namespace bifrost::editor
{
  class EditorOverlay;

  IMemoryManager& allocator();

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
  void deallocateT(T* ptr)
  {
    allocator().deallocateT(ptr);
  }

  template<typename T>
  using UniquePtr = std::unique_ptr<T, meta::function_caller<&deallocateT<T>>>;

  class Project final
  {
   private:
    String m_Name;

   public:
    explicit Project(String&& name) :
      m_Name{std::move(name)}
    {
    }

    const String& name() const { return m_Name; }
  };

  using ActionPtr  = UniquePtr<Action>;
  using ProjectPtr = UniquePtr<Project>;

  class EditorOverlay final : public IGameStateLayer
  {
   private:
    ui::Dialog*                  m_CurrentDialog = nullptr;
    bool                         m_OpenNewDialog = false;
    HashTable<String, ActionPtr> m_Actions       = {};
    BifrostEngine*               m_Engine        = nullptr;
    ProjectPtr                   m_OpenProject   = nullptr;

   protected:
    void onCreate(BifrostEngine& engine) override;
    void onLoad(BifrostEngine& engine) override;
    void onEvent(BifrostEngine& engine, Event& event) override;
    void onUpdate(BifrostEngine& engine, float delta_time) override;
    void onUnload(BifrostEngine& engine) override;
    void onDestroy(BifrostEngine& engine) override;

   public:
    const ProjectPtr& currentlyOpenProject() const { return m_OpenProject; }

    const char* name() override
    {
      return "Bifrost Editor";
    }

    Action* findAction(const char* name) const;
    void    enqueueDialog(ui::Dialog* dlog);
    bool    openProjectDialog();
    bool    openProject(StringRange path);
    void    closeProject();
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_OVERLAY_HPP */
