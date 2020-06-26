#ifndef BIFROST_EDITOR_HIERARCHY_HPP
#define BIFROST_EDITOR_HIERARCHY_HPP

#include "bifrost/data_structures/bifrost_string.hpp"  // String
#include "bifrost_editor_window.hpp"                   // EditorWindow

namespace bifrost::editor
{
  class HierarchyView final : public EditorWindow<HierarchyView>
  {
   private:
    String m_SearchQuery;

   public:
    explicit HierarchyView();

   protected:
    const char* title() const override { return "Hierarchy View"; }
    void        onDrawGUI(EditorOverlay& editor) override;

   private:
    void guiEntityList(EditorOverlay& editor, Entity* entity) const;
    void actionReparent(Entity* entity, Entity* new_parent);
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_HIERARCHY_HPP */