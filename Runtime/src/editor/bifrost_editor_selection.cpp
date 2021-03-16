#include "bf/editor/bifrost_editor_selection.hpp"

namespace bf::editor
{
  Selection::Selection(IMemoryManager& memory) :
    m_Selectables(memory),
    m_OnChangeCallbacks{memory}
  {
  }

  bool Selection::contains(const Selectable& object)
  {
    std::size_t index;

    return find(object, index);
  }

  void Selection::select(const Selectable& object)
  {
    if (!contains(object))
    {
      m_Selectables.push(object);

      if (object.type() == Selectable::type_of<IBaseAsset*>())
      {
        object.as<IBaseAsset*>()->acquire();
      }

      notifyOnChange();
    }
  }

  void Selection::toggle(const Selectable& object)
  {
    if (contains(object))
    {
      deselect(object);
    }
    else
    {
      select(object);
    }
  }

  void Selection::deselect(const Selectable& object)
  {
    std::size_t index;

    if (find(object, index))
    {
      if (object.type() == Selectable::type_of<IBaseAsset*>())
      {
        object.as<IBaseAsset*>()->release();
      }

      m_Selectables.removeAt(index);
      notifyOnChange();
    }
  }

  void Selection::clear()
  {
    if (!m_Selectables.isEmpty())
    {
      forEachOfType<IBaseAsset*>([](IBaseAsset* asset) {
        asset->release();
      });

      m_Selectables.clear();
      notifyOnChange();
    }
  }

  void Selection::addOnChangeListener(const SelectionOnChangeFn& callback)
  {
    std::size_t index;

    if (!findListener(callback, index))
    {
      m_OnChangeCallbacks.push(callback);
    }
    else
    {
      assert(!"Tried to add callback that already exist.");
    }
  }

  void Selection::removeOnChangeListener(const SelectionOnChangeFn& callback)
  {
    std::size_t index;

    if (findListener(callback, index))
    {
      m_OnChangeCallbacks.swapAndPopAt(index);
    }
    else
    {
      assert(!"Tried to remove callback that does not exist.");
    }
  }

  bool Selection::find(const Selectable& object, std::size_t& out_index)
  {
    const std::size_t num_selectables = m_Selectables.size();
    bool              result          = false;

    for (std::size_t i = 0; i < num_selectables; ++i)
    {
      Selectable& selectable = m_Selectables[i];

      if (selectable.type() == object.type())
      {
        visit([&result, &object](const auto& item) {
          using T = std::decay_t<decltype(item)>;

          if (item == object.as<T>())
          {
            result = true;
          }
        },
              selectable);

        if (result)
        {
          out_index = i;
          return result;
        }
      }
    }

    return result;
  }

  bool Selection::findListener(const SelectionOnChangeFn& callback, std::size_t& out_index)
  {
    const std::size_t num_callbacks = m_OnChangeCallbacks.size();

    for (std::size_t i = 0; i < num_callbacks; ++i)
    {
      const SelectionOnChangeFn& fn = m_OnChangeCallbacks[i];

      if (fn == callback)
      {
        out_index = i;
        return true;
      }
    }

    return false;
  }

  void Selection::notifyOnChange()
  {
    for (SelectionOnChangeFn& fn : m_OnChangeCallbacks)
    {
      fn.call(*this);
    }
  }
}  // namespace bifrost::editor
