//
// DISPLAYING GROUPS HIERARCHICALLY [https://caseymuratori.com/blog_0020]
//
#ifndef BF_EDITOR_HIERARCHY_HPP
#define BF_EDITOR_HIERARCHY_HPP

#include "bf/StlAllocator.hpp"                    // StlAllocator<T>
#include "bf/data_structures/bifrost_string.hpp"  // String
#include "bifrost_editor_window.hpp"              // EditorWindow

#include <unordered_set>  //unordered_set<T>

namespace bf::editor
{
  template<typename TKey, typename THasher = std::hash<TKey>, class TKeyEq = std::equal_to<TKey>>
  using UnorderedSet = std::unordered_set<TKey, THasher, TKeyEq, StlAllocator<TKey>>;

  class HierarchyView final : public EditorWindow<HierarchyView>
  {
   private:
    String                m_SearchQuery;
    UnorderedSet<Entity*> m_ExpandedState;
    UnorderedSet<Entity*> m_FilteredIn;
    UnorderedSet<Entity*> m_FilteredInBecauseOfChild;

   public:
    explicit HierarchyView();

   protected:
    const char* title() const override { return "Hierarchy View"; }
    void        onDrawGUI(EditorOverlay& editor) override;

   private:
    void guiEntityList(std::pair<Entity*, Entity*>& parent_to, EditorOverlay& editor, Entity* entity);
    bool isEntityFilteredIn(const Entity* entity) const;
  };
}  // namespace bf::editor

#endif /* BF_EDITOR_HIERARCHY_HPP */