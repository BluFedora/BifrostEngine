#include "bf/editor/bifrost_editor_undo_redo.hpp"

#include "bf/ecs/bifrost_entity.hpp"  // Entity

// [https://rxi.github.io/a_simple_undo_system.html]
namespace MemoryUndoRedo
{
  static constexpr std::size_t k_XXHACK_STACK_SIZE = 1024;

  static CRTAllocator s_UndoItemMemory;

  //
  // Memory Layout:
  // [
  //   { current_state,  sizeof(void*)  }
  //   { old_state_size, sizeof(size_t) }
  //   { old_state,      old_state_size }
  //   { swap_aux,       old_state_size }
  // ]
  //
  // Key: { Field-Name, Size-Of-The-Field }
  //

  struct UndoItem
  {
    void*       current_state;
    std::size_t save_state_size;
    alignas(std::max_align_t) char old_state[1];

    static std::size_t totalBytes(std::size_t state_num_bytes)
    {
      return offsetof(UndoItem, old_state) + state_num_bytes * 2;
    }

    static UndoItem* make(IMemoryManager& memory, void* ptr, std::size_t state_num_bytes)
    {
      UndoItem* const result = new (memory.allocate(totalBytes(state_num_bytes))) UndoItem;

      result->current_state   = ptr;
      result->save_state_size = state_num_bytes;

      if (ptr)
      {
        std::memcpy(result->old_state, ptr, state_num_bytes);
      }

      return result;
    }

    static UndoItem* makeSentinel(IMemoryManager& memory)
    {
      return make(memory, nullptr, 0u);
    }

    bool isCommitSentinel() const
    {
      return current_state == nullptr;
    }

    bool isDataChange() const
    {
      return std::memcmp(current_state, old_state, save_state_size) != 0;
    }

    void swapData()
    {
      if (!isCommitSentinel())
      {
        char* const swap_aux = old_state + save_state_size;

        std::memcpy(swap_aux, current_state, save_state_size);
        std::memcpy(current_state, old_state, save_state_size);
        std::memcpy(old_state, swap_aux, save_state_size);
      }
    }

    void free(IMemoryManager& memory)
    {
      this->~UndoItem();
      memory.deallocate(this, totalBytes(save_state_size));
    }
  };

  struct UndoItemStack
  {
    UndoItem*   items[k_XXHACK_STACK_SIZE];
    std::size_t num_items;

    void push(UndoItem* item)
    {
      assert(num_items < k_XXHACK_STACK_SIZE && "Stack buffer overflow.");

      items[num_items++] = item;
    }

    void removeOldestCommit()
    {
      //
      // The first item is always a commit sentinel
      //                    AND
      // A commit sentinel always has at least one _real_ item after it.
      //                    SO
      // We start on the third item.
      //

      for (std::size_t i = 2; i < num_items; ++i)
      {
        UndoItem* const item = items[i];

        if (item->isCommitSentinel())
        {
          const std::size_t num_items_to_move = num_items - i;

          UndoItem** const copy_bgn = items + i;
          UndoItem** const copy_end = copy_bgn + num_items_to_move;

          // std::memmove(items, items + i, sizeof(items[0]) * num_items_to_move);
          std::copy(copy_bgn, copy_end, items);

          num_items = num_items_to_move;
          break;
        }
      }
    }

    void clear()
    {
      const std::size_t count = num_items;

      for (std::size_t i = 0; i < count; ++i)
      {
        items[i]->free(s_UndoItemMemory);
      }

      num_items = 0;
    }

    bool isEmpty() const { return num_items == 0u; }

    UndoItem* pop()
    {
      assert(num_items != 0u && "Attempt to pop an empty stack.");

      return items[--num_items];
    }

    UndoItem* find(const void* item) const
    {
      const std::size_t count = num_items;

      for (std::size_t i = 0; i < count; ++i)
      {
        if (items[i]->current_state == item)
        {
          return items[i];
        }
      }

      return nullptr;
    }
  };

  static UndoItemStack s_UndoStack;
  static UndoItemStack s_RedoStack;
  static UndoItemStack s_CurrentEditsStack;

  void begin_edit(void* item, std::size_t size)
  {
    UndoItem* const undo_item = s_CurrentEditsStack.find(item);

    if (!undo_item)
    {
      s_CurrentEditsStack.push(UndoItem::make(s_UndoItemMemory, item, size));
    }
    else
    {
      assert(undo_item->save_state_size == size && "If the same pointer is passed in it must have been an edit to the region of memory.");
    }
  }

  void commit_edits()
  {
    if (!s_CurrentEditsStack.isEmpty())
    {
      const auto backward_bgn = std::make_reverse_iterator(s_CurrentEditsStack.items + s_CurrentEditsStack.num_items);
      const auto backward_end = std::make_reverse_iterator(s_CurrentEditsStack.items);

      bool is_first_changed_item = true;

      std::for_each(
       backward_bgn,
       backward_end,
       [&is_first_changed_item](UndoItem* item) {
         if (item->isDataChange())
         {
           if (is_first_changed_item)
           {
             s_RedoStack.clear();
             s_UndoStack.push(UndoItem::makeSentinel(s_UndoItemMemory));

             is_first_changed_item = false;
           }

           s_UndoStack.push(item);
         }
         else
         {
           item->free(s_UndoItemMemory);
         }
       });

      s_CurrentEditsStack.num_items = 0;
    }
  }

  void do_xxxx(UndoItemStack& stack_to_pop, UndoItemStack& stack_to_transfer_items_to)
  {
    const std::size_t old_stack_top = stack_to_pop.num_items;

    while (!stack_to_pop.isEmpty())
    {
      UndoItem* const popped_item = stack_to_pop.pop();

      // We hit the end of the current commit.
      if (popped_item->isCommitSentinel())
      {
        const std::size_t new_stack_top = stack_to_pop.num_items;

        // Copy over the items in order from commit item => the top of the stack_to_pop before we started popping off elements.
        for (std::size_t i = new_stack_top; i < old_stack_top; ++i)
        {
          UndoItem* const item = stack_to_pop.items[i];

          item->swapData();

          stack_to_transfer_items_to.push(item);
        }

        break;
      }
    }
  }

  void do_undo()
  {
    do_xxxx(s_UndoStack, s_RedoStack);
  }

  void do_redo()
  {
    do_xxxx(s_RedoStack, s_UndoStack);
  }
}  // namespace MemoryUndoRedo

namespace bf::editor
{
  IUndoRedoCommandPtr cmd::deleteEntity(Entity& entity)
  {
    struct State final
    {
      EntityRef parent{nullptr};
      EntityRef entity{nullptr};
    };

    State s;

    s.entity = EntityRef{entity};

    return makeLambdaCommand(
     std::move(s),
     [](State& state) {
       Entity& entity = *state.entity.editorCachedEntity();
       entity.editorLinkEntity(state.parent.editorCachedEntity());
     },
     [](State& state) {
       Entity& entity = *state.entity.editorCachedEntity();

       state.parent = EntityRef{entity.editorUnlinkEntity()};
     });
  }
}  // namespace bf::editor
