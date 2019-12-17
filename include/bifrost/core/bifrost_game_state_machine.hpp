#ifndef BIFROST_GAME_STATE_MACHINE_HPP
#define BIFROST_GAME_STATE_MACHINE_HPP

#include "bifrost/memory/bifrost_imemory_manager.hpp" /* IMemoryManager       */
#include "bifrost/utility/bifrost_non_copy_move.hpp"  /* bfNonCopyMoveable<T> */

class BifrostEngine;

namespace bifrost
{
  class IGameStateLayer;

  class GameStateMachine final : private bfNonCopyMoveable<GameStateMachine>
  {
    friend class BifrostEngine;
    friend class IGameStateLayer;

    template<typename T>
    class base_iterator
    {
      friend class GameStateMachine;

     public:
      typedef T               self_type;
      typedef IGameStateLayer value_type;
      typedef value_type&     reference;
      typedef value_type*     pointer;

     protected:
      IGameStateLayer* m_Position;

     public:
      explicit base_iterator(IGameStateLayer* pos) :
        m_Position{pos}
      {
      }

      [[nodiscard]] self_type next() const
      {
        self_type it = (*this);
        ++it;
        return it;
      }

      self_type operator++(int)  // Post-fix
      {
        self_type it = (*this);
        ++(*this);
        return it;
      }

      bool operator==(const base_iterator& rhs) const
      {
        return m_Position == rhs.m_Position;
      }

      bool operator==(pointer rhs) const
      {
        return m_Position == rhs;
      }

      bool operator!=(const base_iterator& rhs) const
      {
        return m_Position != rhs.m_Position;
      }

      bool operator!=(pointer rhs) const
      {
        return m_Position != rhs;
      }

      pointer operator->()
      {
        return m_Position;
      }

      reference operator*()
      {
        return *m_Position;
      }

      const value_type* operator->() const
      {
        return m_Position;
      }

      const value_type& operator*() const
      {
        return *m_Position;
      }

      [[nodiscard]] pointer value() const
      {
        return m_Position;
      }
    };

    class iterator : public base_iterator<iterator>
    {
     public:
      typedef iterator        self_type;
      typedef IGameStateLayer value_type;
      typedef value_type&     reference;
      typedef value_type*     pointer;

     public:
      iterator(IGameStateLayer* pos);

      self_type& operator++();  // pre-fix
    };

    class reverse_iterator : public base_iterator<reverse_iterator>
    {
     public:
      typedef reverse_iterator self_type;
      typedef IGameStateLayer  value_type;
      typedef value_type&      reference;
      typedef value_type*      pointer;

     public:
      reverse_iterator(IGameStateLayer* pos);

      self_type& operator++();  // pre-fix
    };

   public:
    [[nodiscard]] static iterator find(iterator it_bgn, iterator it_end, IGameStateLayer* state);

   private:
    BifrostEngine&   m_Engine;
    IMemoryManager&  m_Memory;
    IGameStateLayer* m_LayerHead;
    IGameStateLayer* m_LayerTail;
    IGameStateLayer* m_OverlayHead;
    IGameStateLayer* m_OverlayTail;
    IGameStateLayer* m_DeleteList;

   public:
    GameStateMachine(BifrostEngine& engine, IMemoryManager& memory);

    // Iteration and Book Keeping.
    [[nodiscard]] iterator         begin() const { return m_LayerHead ? m_LayerHead : m_OverlayHead; }
    [[nodiscard]] iterator         end() const { return nullptr; }
    [[nodiscard]] iterator         cbegin() const { return m_LayerHead ? m_LayerHead : m_OverlayHead; }
    [[nodiscard]] iterator         cend() const { return nullptr; }
    [[nodiscard]] reverse_iterator rbegin() const { return m_OverlayTail ? m_OverlayTail : m_LayerTail; }
    [[nodiscard]] reverse_iterator rend() const { return nullptr; }
    [[nodiscard]] iterator         overlayHeadIt() const { return m_OverlayHead; }
    [[nodiscard]] IGameStateLayer* head() const { return m_LayerHead; }
    [[nodiscard]] IGameStateLayer* tail() const { return m_LayerTail; }
    [[nodiscard]] IGameStateLayer* overlayHead() const { return m_OverlayHead; }
    [[nodiscard]] IGameStateLayer* overlayTail() const { return m_OverlayTail; }

    // Manipulation API
    template<typename T, typename... Args>
    T* push(Args&&... args);

    template<typename T, typename... Args>
    T* pushAfter(IGameStateLayer& after, Args&&... args);

    template<typename T, typename... Args>
    T* pushBefore(IGameStateLayer& before, Args&&... args);

    template<typename T, typename... Args>
    T* addOverlay(Args&&... args);

    void remove(IGameStateLayer* state);
    void removeAll();

    ~GameStateMachine();

   private:
    void purgeStates();
    void pushImpl(IGameStateLayer* state);
    void pushAfterImpl(IGameStateLayer& after, IGameStateLayer* state);
    void pushBeforeImpl(IGameStateLayer& before, IGameStateLayer* state);
    void addOverlayImpl(IGameStateLayer* state);
    void appendToList(IGameStateLayer*& bgn, IGameStateLayer*& end, IGameStateLayer* state) const;
  };

  template<typename T, typename... Args>
  T* GameStateMachine::push(Args&&... args)
  {
    static_assert(std::is_convertible_v<T*, IGameStateLayer*>,
                  "T must publicly derive from the 'bifrost::IGameStateLayer'.");

    T* const state = m_Memory.alloc_t<T>(std::forward<Args>(args)...);
    pushImpl(state);
    return state;
  }

  template<typename T, typename... Args>
  T* GameStateMachine::pushAfter(IGameStateLayer& after, Args&&... args)
  {
    static_assert(std::is_convertible_v<T*, IGameStateLayer*>,
                  "T must publicly derive from the 'bifrost::IGameStateLayer'.");

    T* const state = m_Memory.alloc_t<T>(std::forward<Args>(args)...);
    pushAfterImpl(after, state);
    return state;
  }

  template<typename T, typename... Args>
  T* GameStateMachine::pushBefore(IGameStateLayer& before, Args&&... args)
  {
    static_assert(std::is_convertible_v<T*, IGameStateLayer*>,
                  "T must publicly derive from the 'bifrost::IGameStateLayer'.");

    T* const state = m_Memory.alloc_t<T>(std::forward<Args>(args)...);
    pushBeforeImpl(before, state);
    return state;
  }

  template<typename T, typename... Args>
  T* GameStateMachine::addOverlay(Args&&... args)
  {
    static_assert(std::is_convertible_v<T*, IGameStateLayer*>,
                  "T must publicly derive from the 'bifrost::IGameStateLayer'.");

    T* const state = m_Memory.alloc_t<T>(std::forward<Args>(args)...);
    addOverlayImpl(state);
    return state;
  }
}  // namespace bifrost

#endif /* BIFROST_GAME_STATE_MACHINE_HPP */
