#ifndef BIFROST_EDITOR_SELECTION_HPP
#define BIFROST_EDITOR_SELECTION_HPP

#include "bifrost/utility/bifrost_function_view.hpp"  // FunctionView<R(Args...)>
#include "bifrost_editor_window.hpp"                  // Selectable

namespace bifrost::editor
{
  class Selection;

  using SelectionOnChangeFn = FunctionView<void(Selection&)>;

  class Selection final
  {
   private:
    Array<Selectable>          m_Selectables;
    Array<SelectionOnChangeFn> m_OnChangeCallbacks;

   public:
    explicit Selection(IMemoryManager& memory);

    // const Array<Selectable>& selectables() const { return m_Selectables; }

    Array<Selectable>& selectables() { return m_Selectables; }

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

    bool contains(const Selectable& object);
    void select(const Selectable& object);
    void deselect(const Selectable& object);
    void clear();

    void addOnChangeListener(const SelectionOnChangeFn& callback);
    void removeOnChangeListener(const SelectionOnChangeFn& callback);

   private:
    bool find(const Selectable& object, std::size_t& out_index);
    bool findListener(const SelectionOnChangeFn& callback, std::size_t& out_index);
    void notifyOnChange();
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_SELECTION_HPP */
