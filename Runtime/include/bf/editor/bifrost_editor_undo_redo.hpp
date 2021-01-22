/******************************************************************************/
/*!
* @file   bifrost_editor_undo_redo.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   References:
*     [https://rxi.github.io/a_simple_undo_system.html]
*
* @version 0.0.1
* @date    2020-05-26
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BF_EDITOR_UNDO_REDO_HPP
#define BF_EDITOR_UNDO_REDO_HPP

#include "bf/data_structures/bifrost_array.hpp" /* Array<T>     */
#include "bf/utility/bifrost_json.hpp"          /* json::Value  */
#include "bifrost_editor_memory.hpp"            /* UniquePtr<T> */

namespace bf::editor
{
  using IUndoRedoCommandPtr = UniquePtr<struct IUndoRedoCommand>;

  //
  // Interface that each command must implement.
  //
  struct IUndoRedoCommand
  {
    template<typename T, typename... Args>
    static IUndoRedoCommandPtr create(Args&&... args)
    {
      static_assert(std::is_base_of_v<IUndoRedoCommand, T>, "T must implement the IUndoRedoCommand interface.");
      return IUndoRedoCommandPtr(make<T>(std::forward<Args>(args)...));
    }

    // TODO(SR): This can probably be marked const but there always seems to be cases where it gets in the way.
    virtual StringRange name() = 0;
    virtual void        undo() = 0;
    virtual void        redo() = 0;

    // This is called only once ever in `UndoRedoStack::doCommand`
    virtual void exec() { redo(); }
    virtual ~IUndoRedoCommand() = default;
  };

  //
  // Basic Undo Redo Stack Implementation.
  // Owns the memory of the `IUndoRedoCommand`s
  //
  class UndoRedoStack final
  {
   private:
    Array<IUndoRedoCommandPtr> m_UndoRedoStack;  //!< Stack of commands with layout: [Undo Stack |^m_StackTop^| Redo Stack].
    std::uint32_t              m_StackTop;       //!< Index of the start of the redo part of the stack. (one after the last undo item)

   public:
    UndoRedoStack() :
      m_UndoRedoStack{allocator()},
      m_StackTop{0u}
    {
    }

    // Accessors

    const Array<IUndoRedoCommandPtr>& commands() const { return m_UndoRedoStack; }
    std::uint32_t                     stackTop() const { return m_StackTop; }
    bool                              canUndo() const { return m_StackTop != 0; }
    bool                              canRedo() const { return m_StackTop != m_UndoRedoStack.size(); }

    // The Main Logic

    void doCommand(IUndoRedoCommandPtr&& cmd)
    {
      clearRedo();
      m_UndoRedoStack.emplace(std::move(cmd))->exec();
      ++m_StackTop;
    }

    void undo()
    {
      assert(canUndo() && "`canUndo` must be checked before calling this function.");

      IUndoRedoCommandPtr& current_action = m_UndoRedoStack[--m_StackTop];
      current_action->undo();
    }

    void redo()
    {
      assert(canRedo() && "`canRedo` must be checked before calling this function.");

      IUndoRedoCommandPtr& current_action = m_UndoRedoStack[m_StackTop++];
      current_action->redo();
    }

   private:
    void clearRedo()
    {
      m_UndoRedoStack.resize(m_StackTop);
    }
  };

  // General Commands //

  enum class UndoRedoEventType : std::uint8_t
  {
    ON_CREATE,
    ON_UNDO,
    ON_REDO,
    ON_DESTROY,
  };

  template<typename TLambda>
  class LambdaUndoRedoCmd final : public IUndoRedoCommand
  {
   private:
    String  m_Name;
    TLambda m_Callback;

   public:
    explicit LambdaUndoRedoCmd(StringRange name, TLambda&& callback) :
      m_Name{name},
      m_Callback{std::move(callback)}
    {
      m_Callback(UndoRedoEventType::ON_CREATE);
    }

    StringRange name() override { return m_Name; }
    void        undo() override { m_Callback(UndoRedoEventType::ON_UNDO); }
    void        redo() override { m_Callback(UndoRedoEventType::ON_REDO); }
    ~LambdaUndoRedoCmd() override { m_Callback(UndoRedoEventType::ON_DESTROY); }
  };

  // Memory Undo System //

  //
  // Memory Layout of UndoItem:
  // [
  //   { current_state,  sizeof(void*)  }
  //   { old_state_size, sizeof(size_t) }
  //   { old_state,      old_state_size }
  //   { swap_aux,       old_state_size }
  // ]
  //
  // Diagram Key: { Field-Name, Size-Of-The-Field }
  //
  // IMPORTANT(SR): Only pointers to this object should be stored in containers.
  //
  struct MemoryUndoItem
  {
    void*       current_state;
    std::size_t save_state_size;
    alignas(std::max_align_t) char old_state[1];  // vla

    static std::size_t     totalBytes(std::size_t state_num_bytes);
    static MemoryUndoItem* make(IMemoryManager& memory, void* ptr, std::size_t state_num_bytes);
    static MemoryUndoItem* makeSentinel(IMemoryManager& memory);

    bool isCommitSentinel() const;
    bool hasDataChanged() const;
    void swapData();
    void free(IMemoryManager& memory);
  };

  //
  // Stack Layout: [X******X***X*****X****]
  // Where X = Commit Sentinel
  //       * = UndoItem with saved data.
  //
  struct UndoItemStack
  {
    Array<MemoryUndoItem*> items;

    explicit UndoItemStack(IMemoryManager& memory);

    void push(MemoryUndoItem* item);
    void clear();
    void clearNoFree();

    // [start, end) Half open range
    void            freeItemsInRange(std::size_t start, std::size_t end);
    bool            isEmpty() const;
    MemoryUndoItem* pop();
    MemoryUndoItem* find(const void* item) const;
  };

  //
  // Handles undo redo actions that can be operated on
  // when pointers to the objects are stable.
  //
  class MemoryUndoRedo
  {
   private:
    IMemoryManager& m_UndoItemMemory;
    UndoItemStack   m_UndoStack;
    UndoItemStack   m_RedoStack;
    UndoItemStack   m_CurrentEditsStack;

   public:
    explicit MemoryUndoRedo(IMemoryManager& memory);

    bool canUndo() const { return !m_UndoStack.isEmpty(); }
    bool canRedo() const { return !m_RedoStack.isEmpty(); }
    bool hasPendingCommit() const { return !m_CurrentEditsStack.isEmpty(); }

    void beginEdit(void* item, std::size_t size);
    bool commitEdits();  // Returns if a new commit was created.
    void clear();
    void undo();
    void redo();

   private:
    static void undoRedoImpl(UndoItemStack& stack_to_pop, UndoItemStack& stack_to_transfer_items_to);
  };

  class MemoryUndoRedoCmd final : public IUndoRedoCommand
  {
   private:
    String          m_Name;
    MemoryUndoRedo& m_MemUndoRedo;

   public:
    MemoryUndoRedoCmd(StringRange cmd_name, MemoryUndoRedo& manager);

    StringRange name() override { return m_Name; }
    void        undo() override { m_MemUndoRedo.undo(); }
    void        redo() override { m_MemUndoRedo.redo(); }
  };

  class SerializeUndoRedo final : public IUndoRedoCommand
  {
   private:
    String       m_Name;
    Assets&      m_Assets;
    IBaseObject& m_Target;
    json::Value  m_ValueToSwapTo;

   public:
    SerializeUndoRedo(StringRange cmd_name, Assets& assets, IBaseObject& target);
    SerializeUndoRedo(StringRange cmd_name, Assets& assets, IBaseObject& target, json::Value old_value);

    StringRange name() override { return m_Name; }
    void        undo() override;
    void        redo() override;
    void        exec() override;

   private:
    void swapValues();
  };

  json::Value serialize(IBaseObject& target);

  class History;

  class PotentialSerializeEdit
  {
    friend class History;

   private:
    History&     m_History;
    Assets&      m_Assets;
    IBaseObject& m_Target;
    json::Value  m_SavedValue;
    bool         m_WasJustCreated;

   public:
    PotentialSerializeEdit(History& history, Assets& assets, IBaseObject& target);

    // Returns true the first time 'History::makePotentialSerializeEdit' is called
    // with a unique `IBaseObject& target` in between commits.
    bool wasJustCreated() const
    {
      return m_WasJustCreated;
    }

    // After a call to `commit` or `cancel` this object is now invalid.

    void commit(StringRange name);
    void cancel();
  };

  //
  // Defines the Interface for manipulating objects in the Editor
  // in a way that allows the user to undo their actions.
  //
  class History
  {
    friend class PotentialSerializeEdit;

   private:
    UndoRedoStack                m_UndoRedoStack;
    MemoryUndoRedo               m_MemoryUndoRedo;
    List<PotentialSerializeEdit> m_CurrentPotentialEdits;

   public:
    History() :
      m_UndoRedoStack{},
      m_MemoryUndoRedo{allocator()},
      m_CurrentPotentialEdits{allocator()}
    {
    }

    // Stack Manipulation

    const UndoRedoStack& stack() const { return m_UndoRedoStack; }

    bool canUndo() const { return m_UndoRedoStack.canUndo(); }
    bool canRedo() const { return m_UndoRedoStack.canRedo(); }
    void undo() { m_UndoRedoStack.undo(); }
    void redo() { m_UndoRedoStack.redo(); }

    // Lambda Edit (For one off actions)

    template<typename TLambda>
    void performLambdaAction(StringRange edit_name, TLambda&& lambda)
    {
      using TCmd = LambdaUndoRedoCmd<TLambda>;

      m_UndoRedoStack.doCommand(IUndoRedoCommand::create<TCmd>(edit_name, std::forward<TLambda>(lambda)));
    }

    // Memory Edits (For objects with stable pointers)

    // You can call `performMemoryEdit` multiple times before a `commitMemoryEdit`.
    void performMemoryEdit(void* item, std::size_t size);
    void commitMemoryEdit(StringRange edit_name);

    // Serialize Edit (Sledgehammer method for objects with stable pointers)

    void                    performSerializeEdit(StringRange edit_name, Assets& assets, IBaseObject& reflectable_object);
    PotentialSerializeEdit* makePotentialSerializeEdit(Assets& assets, IBaseObject& reflectable_object);
  };

}  // namespace bf::editor

#endif /* BF_EDITOR_UNDO_REDO_HPP */
