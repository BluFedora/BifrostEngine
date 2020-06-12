#include "bifrost/core/bifrost_game_state_machine.hpp"

#include "bifrost/core/bifrost_igame_state_layer.hpp" /* IGameStateLayer */

namespace bifrost
{
  GameStateMachine::iterator::iterator(IGameStateLayer* pos) :
    base_iterator<iterator>{pos}
  {
  }

  GameStateMachine::iterator::self_type& GameStateMachine::iterator::operator++()
  {
    m_Position = m_Position->m_Next;
    return (*this);
  }

  GameStateMachine::reverse_iterator::reverse_iterator(IGameStateLayer* pos) :
    base_iterator<reverse_iterator>{pos}
  {
  }

  GameStateMachine::reverse_iterator::self_type& GameStateMachine::reverse_iterator::operator++()
  {
    m_Position = m_Position->m_Prev;
    return (*this);
  }

  GameStateMachine::iterator GameStateMachine::find(iterator it_bgn, iterator it_end, IGameStateLayer* state)
  {
    for (auto it = it_bgn; it != it_end; ++it)
    {
      if (it.m_Position == state)
      {
        return it;
      }
    }

    return it_end;
  }

  GameStateMachine::GameStateMachine(BifrostEngine& engine, IMemoryManager& memory) :
    m_Engine{engine},
    m_Memory{memory},
    m_LayerHead{nullptr},
    m_LayerTail{nullptr},
    m_OverlayHead{nullptr},
    m_OverlayTail{nullptr},
    m_DeleteList{nullptr}
  {
  }

  void GameStateMachine::remove(IGameStateLayer* state)
  {
    IGameStateLayer* const state_prev    = state->m_Prev;
    IGameStateLayer* const state_next    = state->m_Next;
    const iterator         end_it        = overlayHeadIt();
    iterator               prev_it       = m_LayerHead ? find(m_LayerHead, end_it, state_prev) : end_it;
    iterator               next_it       = m_LayerHead ? find(m_LayerHead, end_it, state_next) : end_it;
    const bool             prev_in_layer = prev_it != end_it;
    const bool             next_in_layer = next_it != end_it;

    if (!prev_in_layer)
    {
      prev_it = nullptr;
    }

    if (!next_in_layer)
    {
      next_it = nullptr;
    }

    // Book Keeping Updates
    // NOTE(Shareef):
    //   The extra "m_LayerHead" check is because Visual Studio static analysis
    //   thinks 'state' may be  null since I check 'm_LayerHead' for null earlier
    if (m_LayerHead && m_LayerHead == state)
    {
      m_LayerHead = next_it.value();
    }

    if (m_LayerTail == state)
    {
      m_LayerTail = prev_it.value();
    }

    if (m_OverlayHead == state)
    {
      m_OverlayHead = !next_in_layer ? state->m_Next : nullptr;
    }

    if (m_OverlayTail == state)
    {
      m_OverlayTail = !prev_in_layer ? state->m_Prev : nullptr;
    }

    // List Patchup
    if (state->m_Next)
    {
      state->m_Next->m_Prev = state->m_Prev;
    }

    if (state->m_Prev)
    {
      state->m_Prev->m_Next = state->m_Next;
    }

    // Delayed Deletion
    state->m_Next = m_DeleteList;

    if (m_DeleteList)
    {
      m_DeleteList->m_Prev = state;
    }

    m_DeleteList = state;
  }

  void GameStateMachine::removeAll()
  {
    while (m_LayerHead)
    {
      remove(m_LayerHead);
    }

    while (m_OverlayHead)
    {
      remove(m_OverlayHead);
    }

    purgeStates();
  }

  GameStateMachine::~GameStateMachine()
  {
    removeAll();
  }

  void GameStateMachine::purgeStates()
  {
    while (m_DeleteList)
    {
      const auto next = m_DeleteList->m_Next;

      m_DeleteList->onUnload(m_Engine);
      m_DeleteList->onDestroy(m_Engine);
      m_Memory.deallocateT(m_DeleteList);

      m_DeleteList = next;
    }
  }

  void GameStateMachine::pushImpl(IGameStateLayer* state)
  {
    appendToList(m_LayerHead, m_LayerTail, state);
  }

  void GameStateMachine::pushAfterImpl(IGameStateLayer& after, IGameStateLayer* state)
  {
    IGameStateLayer*& head_to_use = after.m_IsOverlay ? m_OverlayHead : m_LayerHead;
    IGameStateLayer*& tail_to_use = after.m_IsOverlay ? m_OverlayTail : m_LayerTail;
    IGameStateLayer*  after_ptr   = &after;

    state->m_IsOverlay = after.m_IsOverlay;

    appendToList(head_to_use, after_ptr, state);

    if (&after == tail_to_use)
    {
      tail_to_use = after_ptr;
    }
  }

  void GameStateMachine::pushBeforeImpl(IGameStateLayer& before, IGameStateLayer* state)
  {
    IGameStateLayer*& head_to_use = before.m_IsOverlay ? m_OverlayHead : m_LayerHead;
    IGameStateLayer*& tail_to_use = before.m_IsOverlay ? m_OverlayTail : m_LayerTail;

    state->m_IsOverlay = before.m_IsOverlay;

    if (!head_to_use || before.m_Prev)
    {
      appendToList(head_to_use, before.m_Prev, state);
    }
    else
    {
      head_to_use->m_Prev = state;
      state->m_Next       = head_to_use;
      state->m_Prev       = nullptr;
      head_to_use         = state;

      state->onCreate(m_Engine);
      state->onLoad(m_Engine);
    }

    if (!tail_to_use)
    {
      tail_to_use = head_to_use;
    }

    if (&before == m_OverlayHead)
    {
      m_LayerTail->m_Next = state;
      m_OverlayHead       = state;
    }
  }

  void GameStateMachine::addOverlayImpl(IGameStateLayer* state)
  {
    state->m_IsOverlay = true;

    const bool needs_append = m_OverlayHead == nullptr;

    appendToList(m_OverlayHead, m_OverlayTail, state);

    if (needs_append && m_LayerTail)
    {
      m_LayerTail->m_Next   = m_OverlayHead;
      m_OverlayHead->m_Prev = m_LayerTail;
    }
  }

  void GameStateMachine::appendToList(IGameStateLayer*& bgn, IGameStateLayer*& end, IGameStateLayer* state) const
  {
    if (!bgn)
    {
      bgn = state;
      end = state;
    }
    else
    {
      IGameStateLayer*       real_end   = end;
      IGameStateLayer* const after_next = real_end->m_Next;

      if (after_next)
      {
        after_next->m_Prev = state;
      }

      real_end->m_Next = state;
      state->m_Next    = after_next;
      state->m_Prev    = real_end;

      end = state;
    }

    state->onCreate(m_Engine);
    state->onLoad(m_Engine);
  }
}  // namespace bifrost
