/******************************************************************************/
/*!
* @file   bifrost_editor_undo_redo.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   
*
* @version 0.0.1
* @date    2020-05-26
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_EDITOR_UNDO_REDO_HPP
#define BIFROST_EDITOR_UNDO_REDO_HPP

#include "bifrost/data_structures/bifrost_array.hpp" /* Array<T>     */
#include "bifrost_editor_memory.hpp"                 /* UniquePtr<T> */

namespace bifrost::editor
{
  struct IUndoRedoCommand
  {
    virtual void undo()         = 0;
    virtual void redo()         = 0;
    virtual ~IUndoRedoCommand() = default;
    virtual void exec() { redo(); }
  };

  using IUndoRedoCommandPtr = UniquePtr<IUndoRedoCommand>;

  template<typename FUndo, typename FRedo>
  struct LambdaUndoRedoCmd final : public IUndoRedoCommand
  {
    FUndo undo_impl;
    FRedo redo_impl;

    LambdaUndoRedoCmd(FUndo&& u, FRedo&& r) :
      undo_impl{std::move(u)},
      redo_impl{std::move(r)}
    {
    }

    void undo() override
    {
      undo_impl();
    }

    void redo() override
    {
      redo_impl();
    }
  };

  template<typename State, typename FUndo, typename FRedo>
  struct StatefulLambdaUndoRedoCmd final : public IUndoRedoCommand
  {
    State state;
    FUndo undo_impl;
    FRedo redo_impl;

    StatefulLambdaUndoRedoCmd(State&& state, FUndo&& u, FRedo&& r) :
      state{std::move(state)},
      undo_impl{std::move(u)},
      redo_impl{std::move(r)}
    {
    }

    void undo() override
    {
      undo_impl(state);
    }

    void redo() override
    {
      redo_impl(state);
    }
  };

  class UndoRedoStack final
  {
   private:
    Array<IUndoRedoCommandPtr> m_UndoRedoStack;  //!< Stack of commands with layout: [Undo Stack |^m_StackTop^| Redo Stack].
    std::int32_t               m_StackTop;       //!< Index of last performed command.

   public:
    UndoRedoStack() :
      m_UndoRedoStack{allocator()},
      m_StackTop{-1}
    {
    }

    bool canUndo() const
    {
      return m_StackTop >= 0;
    }

    bool canRedo() const
    {
      return (m_StackTop + 1u) != m_UndoRedoStack.size();
    }

    void doCommand(IUndoRedoCommandPtr&& cmd)
    {
      cmd->exec();
      m_UndoRedoStack.emplace(std::move(cmd));
      ++m_StackTop;

      clearRedo();
    }

    void undo()
    {
      assert(canUndo());

      IUndoRedoCommandPtr& current_action = m_UndoRedoStack[m_StackTop--];
      current_action->undo();
    }

    void redo()
    {
      assert(canRedo());

      IUndoRedoCommandPtr& current_action = m_UndoRedoStack[++m_StackTop];
      current_action->redo();
    }

   private:
    void clearRedo()
    {
      m_UndoRedoStack.resize(std::size_t(m_StackTop) + 1u);
    }
  };

  template<typename T, typename... Args>
  IUndoRedoCommandPtr makeCommand(Args&&... args)
  {
    static_assert(std::is_base_of_v<IUndoRedoCommand, T>, "T must implement the IUndoRedoCommand interface.");
    return IUndoRedoCommandPtr(make<T>(std::forward<Args>(args)...));
  }

  template<typename FUndo, typename FRedo>
  IUndoRedoCommandPtr makeLambdaCommand(FUndo&& u, FRedo&& r)
  {
    return makeCommand<LambdaUndoRedoCmd<FUndo, FRedo>>(std::forward(u), std::forward(r));
  }

  template<typename State, typename FUndo, typename FRedo>
  IUndoRedoCommandPtr makeLambdaCommand(State&& state, FUndo&& u, FRedo&& r)
  {
    return makeCommand<StatefulLambdaUndoRedoCmd<State, FUndo, FRedo>>(std::forward<State>(state), std::forward<FUndo>(u), std::forward<FRedo>(r));
  }

  namespace cmd
  {
    IUndoRedoCommandPtr deleteEntity(Entity& entity);
  }

}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_UNDO_REDO_HPP */
