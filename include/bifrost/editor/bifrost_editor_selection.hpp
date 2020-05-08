#ifndef BIFROST_EDITOR_SELECTION_HPP
#define BIFROST_EDITOR_SELECTION_HPP

#include "bifrost_editor_window.hpp"  // Selectable

namespace bifrost::editor
{
  class Selection final
  {
   private:
    Array<Selectable> m_Selectables;

   public:
    explicit Selection(IMemoryManager& memory);

    template<typename T, typename F>
    void forEachOfType(F&& callback)
    {
      for (Selectable& selectable : m_Selectables)
      {
        if (selectable.is<T>())
        {
          callback(selectable.as<T>());
        }
      }
    }

    bool contains();
    void select();
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_SELECTION_HPP */
