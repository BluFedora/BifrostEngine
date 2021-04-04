/******************************************************************************/
/*!
* @file   bifrost_editor_undo_redo.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   References:
*     [https://rxi.github.io/a_simple_undo_system.html]
*
* @version 0.0.1
* @date    2020-05-26
*
* @copyright Copyright (c) 2020-2021
*/
/******************************************************************************/
#include "bf/editor/bifrost_editor_undo_redo.hpp"

namespace bf::editor
{
  // UndoItemStack

  UndoItemStack::UndoItemStack(IMemoryManager& memory) :
    items{memory}
  {
  }

  void UndoItemStack::push(MemoryUndoItem* item)
  {
    items.push(item);
  }

  void UndoItemStack::clear()
  {
    freeItemsInRange(0, items.size());
    clearNoFree();
  }

  void UndoItemStack::clearNoFree()
  {
    items.clear();
  }

  void UndoItemStack::freeItemsInRange(std::size_t start, std::size_t end)
  {
    for (std::size_t i = start; i < end; ++i)
    {
      items[i]->free(items.memory());
    }
  }

  bool UndoItemStack::isEmpty() const
  {
    return items.isEmpty();
  }

  MemoryUndoItem* UndoItemStack::pop()
  {
    assert(!isEmpty() && "Attempt to pop an empty stack.");

    MemoryUndoItem* const item = items.back();
    items.pop();

    return item;
  }

  MemoryUndoItem* UndoItemStack::find(const void* item) const
  {
    const std::size_t count = items.size();

    for (std::size_t i = 0; i < count; ++i)
    {
      if (items[i]->current_state == item)
      {
        return items[i];
      }
    }

    return nullptr;
  }

  // MemoryUndoItem

  std::size_t MemoryUndoItem::totalBytes(std::size_t state_num_bytes)
  {
    return offsetof(MemoryUndoItem, old_state) + state_num_bytes * 2;
  }

  MemoryUndoItem* MemoryUndoItem::make(IMemoryManager& memory, void* ptr, std::size_t state_num_bytes)
  {
    MemoryUndoItem* const result = new (memory.allocate(totalBytes(state_num_bytes))) MemoryUndoItem;

    result->current_state   = ptr;
    result->save_state_size = state_num_bytes;

    if (ptr)
    {
      std::memcpy(result->old_state, ptr, state_num_bytes);
    }

    return result;
  }

  MemoryUndoItem* MemoryUndoItem::makeSentinel(IMemoryManager& memory)
  {
    return make(memory, nullptr, 0u);
  }

  bool MemoryUndoItem::isCommitSentinel() const
  {
    return current_state == nullptr;
  }

  bool MemoryUndoItem::hasDataChanged() const
  {
    return std::memcmp(current_state, old_state, save_state_size) != 0;
  }

  void MemoryUndoItem::swapData()
  {
    if (!isCommitSentinel())
    {
      char* const swap_aux = old_state + save_state_size;

      std::memcpy(swap_aux, current_state, save_state_size);
      std::memcpy(current_state, old_state, save_state_size);
      std::memcpy(old_state, swap_aux, save_state_size);
    }
  }

  void MemoryUndoItem::free(IMemoryManager& memory)
  {
    this->~MemoryUndoItem();
    memory.deallocate(this, totalBytes(save_state_size));
  }

  // MemoryUndoRedo

  MemoryUndoRedo::MemoryUndoRedo(IMemoryManager& memory) :
    m_UndoItemMemory{memory},
    m_UndoStack{memory},
    m_RedoStack{memory},
    m_CurrentEditsStack{memory}
  {
  }

  void MemoryUndoRedo::beginEdit(void* item, std::size_t size)
  {
    MemoryUndoItem* const undo_item = m_CurrentEditsStack.find(item);

    if (!undo_item)
    {
      m_CurrentEditsStack.push(MemoryUndoItem::make(m_UndoItemMemory, item, size));
    }
    else
    {
      assert(undo_item->save_state_size == size && "An edit to the same pointer must be the same region in memory.");
    }
  }

  bool MemoryUndoRedo::commitEdits()
  {
    bool has_changed_item = false;

    if (!m_CurrentEditsStack.isEmpty())
    {
      for (MemoryUndoItem* item : ReverseLoop(m_CurrentEditsStack.items))
      {
        if (item->hasDataChanged())
        {
          if (!has_changed_item)
          {
            m_RedoStack.clear();
            m_UndoStack.push(MemoryUndoItem::makeSentinel(m_UndoItemMemory));

            has_changed_item = true;
          }

          m_UndoStack.push(item);
        }
        else
        {
          item->free(m_UndoItemMemory);
        }
      }

      m_CurrentEditsStack.clearNoFree();
    }

    return has_changed_item;
  }

  void MemoryUndoRedo::clear()
  {
    m_UndoStack.clear();
    m_RedoStack.clear();
    m_CurrentEditsStack.clear();
  }

  void MemoryUndoRedo::undo()
  {
    undoRedoImpl(m_UndoStack, m_RedoStack);
  }

  void MemoryUndoRedo::redo()
  {
    undoRedoImpl(m_RedoStack, m_UndoStack);
  }

  void MemoryUndoRedo::undoRedoImpl(UndoItemStack& stack_to_pop, UndoItemStack& stack_to_transfer_items_to)
  {
    const std::size_t old_stack_top = stack_to_pop.items.size();

    while (!stack_to_pop.isEmpty())
    {
      MemoryUndoItem* const popped_item = stack_to_pop.pop();

      // We hit the end of the current commit.
      if (popped_item->isCommitSentinel())
      {
        const std::size_t new_stack_top = stack_to_pop.items.size();

        // Copy over the items in order from commit item => the top of the stack_to_pop before we started popping off elements.
        for (std::size_t i = new_stack_top; i < old_stack_top; ++i)
        {
          MemoryUndoItem* const item = stack_to_pop.items[i];

          item->swapData();

          stack_to_transfer_items_to.push(item);
        }

        break;
      }
    }
  }

  // MemoryUndoRedoCmd

  MemoryUndoRedoCmd::MemoryUndoRedoCmd(StringRange cmd_name, MemoryUndoRedo& manager) :
    m_Name{cmd_name},
    m_MemUndoRedo{manager}
  {
  }

  // SerializeUndoRedo

  SerializeUndoRedo::SerializeUndoRedo(StringRange cmd_name, Assets& assets, IBaseObject& target) :
    SerializeUndoRedo(cmd_name, assets, target, serialize(target))
  {
  }

  SerializeUndoRedo::SerializeUndoRedo(StringRange cmd_name, Assets& assets, IBaseObject& target, json::Value old_value) :
    m_Name{cmd_name},
    m_Assets{assets},
    m_Target{target},
    m_ValueToSwapTo(std::move(old_value))
  {
  }

  void SerializeUndoRedo::undo()
  {
    swapValues();
  }

  void SerializeUndoRedo::redo()
  {
    swapValues();
  }

  void SerializeUndoRedo::exec()
  {
    // Empty on purpose
  }

  void SerializeUndoRedo::swapValues()
  {
    // Save The targets current value.
    json::Value current_value = serialize(m_Target);

    // Apply the value saved in this command
    {
      JsonSerializerReader json_reader{m_Assets, allocator(), m_ValueToSwapTo};

      if (json_reader.beginDocument())
      {
        m_Target.reflect(json_reader);
        json_reader.endDocument();
      }
    }

    // Next time we swap in the current value into this object.
    {
      m_ValueToSwapTo = std::move(current_value);
    }
  }

  json::Value serialize(IBaseObject& target)
  {
    JsonSerializerWriter json_writer{allocator()};
    json::Value          current_value = {};

    if (json_writer.beginDocument())
    {
      target.reflect(json_writer);
      json_writer.endDocument();

      current_value = json_writer.document();
    }

    return current_value;
  }

  // PotentialSerializeEdit

  PotentialSerializeEdit::PotentialSerializeEdit(History& history, Assets& assets, IBaseObject& target) :
    m_History{history},
    m_Assets{assets},
    m_Target{target},
    m_SavedValue(serialize(m_Target)),
    m_WasJustCreated{true}
  {
  }

  void PotentialSerializeEdit::commit(StringRange name)
  {
    m_History.m_UndoRedoStack.doCommand(IUndoRedoCommand::create<SerializeUndoRedo>(name, m_Assets, m_Target, m_SavedValue));
    cancel();
  }

  void PotentialSerializeEdit::cancel()
  {
    m_History.m_CurrentPotentialEdits.erase(m_History.m_CurrentPotentialEdits.makeIterator(*this));
  }

  // History

  void History::performMemoryEdit(void* item, std::size_t size)
  {
    m_MemoryUndoRedo.beginEdit(item, size);
  }

  void History::commitMemoryEdit(StringRange edit_name)
  {
    if (m_MemoryUndoRedo.commitEdits())
    {
      m_UndoRedoStack.doCommand(IUndoRedoCommand::create<MemoryUndoRedoCmd>(edit_name, m_MemoryUndoRedo));
    }
  }

  void History::performSerializeEdit(StringRange edit_name, Assets& assets, IBaseObject& reflectable_object)
  {
    m_UndoRedoStack.doCommand(IUndoRedoCommand::create<SerializeUndoRedo>(edit_name, assets, reflectable_object));
  }

  PotentialSerializeEdit* History::makePotentialSerializeEdit(Assets& assets, IBaseObject& reflectable_object)
  {
    for (PotentialSerializeEdit& edit : m_CurrentPotentialEdits)
    {
      if (&edit.m_Target == &reflectable_object)
      {
        edit.m_WasJustCreated = false;
        return &edit;
      }
    }

    return &m_CurrentPotentialEdits.emplaceBack(*this, assets, reflectable_object);
  }
}  // namespace bf::editor

#if 0
  void UndoItemStack::removeOldestCommit()
  {
    const std::size_t count = items.size();

    // There is always at least two items in the stack.
    // If there are exactly two that means there is only a single commit.

    if (count == 2)
    {
      clear();
    }
    else
    {
      //
      // The first item is always a commit sentinel
      //                    AND
      // A commit sentinel always has at least one _real_ item after it.
      //                    SO
      // We start on the third item.
      //

      for (std::size_t i = 2; i < count; ++i)
      {
        MemoryUndoItem* const item = items[i];

        if (item->isCommitSentinel())
        {
          const std::size_t num_items_to_move = count - i;

          MemoryUndoItem** const copy_bgn = items.data() + i;
          MemoryUndoItem** const copy_end = copy_bgn + num_items_to_move;

          freeItemsInRange(0, i);
          std::copy(copy_bgn, copy_end, items);

          items.resize(num_items_to_move);
          break;
        }
      }
    }
  }
#endif
