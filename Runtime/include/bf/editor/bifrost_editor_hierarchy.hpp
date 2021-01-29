#ifndef BF_EDITOR_HIERARCHY_HPP
#define BF_EDITOR_HIERARCHY_HPP

#include "bf/data_structures/bifrost_string.hpp"  // String
#include "bifrost_editor_window.hpp"              // EditorWindow

namespace bf::editor
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
  };
}  // namespace bf::editor

#endif /* BF_EDITOR_HIERARCHY_HPP */