/******************************************************************************/
/*!
 * @file   bifrost_vm.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   A C++ wrapper around the scripting virtual machine API
 *   with helpers for binding any callable object and classes.
 *
 *   References:
 *     [https://blog.tartanllama.xyz/exploding-tuples-fold-expressions/]
 *
 * @version 0.0.1
 * @date    2020-01-02
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BIFROST_VM_HPP
#define BIFROST_VM_HPP

#include "bifrost/data_structures/bifrost_string.hpp" /* */
#include "bifrost_vm.h"                               /* bfVM_*                   */
#include "meta/bifrost_meta_function_traits.hpp"      /* function_traits          */
#include "meta/bifrost_meta_utils.hpp"                /* for_each                 */
#include "utility/bifrost_non_copy_move.hpp"          /* bfNonCopyMoveable        */
#include <string>                                     /* string                   */
#include <tuple>                                      /* tuple                    */
#include <type_traits>                                /* decay_t, is_arithmetic_v */

namespace bifrost
{
  namespace detail
  {
    template<typename...>
    constexpr std::false_type always_false{};

    /*!
     * @brief
     *   Tag type for 'writeToSlot' so that the stack is not written to.
     */
    struct RetainStack final
    {
    };

    template<typename T>
    static T readFromSlot(const BifrostVM* self, std::size_t slot)
    {
      // TODO(Shareef): Add some type checking, atleast let the user be able to handle type mismatches.

      if constexpr (std::is_arithmetic_v<T>)
      {
        return static_cast<T>(bfVM_stackReadNumber(self, slot));
      }
      else if constexpr (std::is_same_v<T, bool>)
      {
        return bfVM_stackReadBool(self, slot);
      }
      else if constexpr (std::is_same_v<T, StringRange> || std::is_same_v<T, std::string>)
      {
        std::size_t       str_size;
        const char* const str = bfVM_stackReadString(self, slot, &str_size);

        // NOTE(Shareef): Both 'StringRange' and 'std::string' have a iterator / pointer style ctor.
        return {str, str + str_size};
      }
      else if constexpr (std::is_pointer_v<T>)
      {
        if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
        {
          return bfVM_stackReadString(self, slot, nullptr);
        }
        else
        {
          return reinterpret_cast<T>(bfVM_stackReadInstance(self, slot));
        }
      }
      else if constexpr (std::is_reference_v<T>)
      {
        return *reinterpret_cast<T*>(bfVM_stackReadInstance(self, slot));
      }
      else
      {
        static_assert(always_false<T>, "Type could not be automatically converted.");
        return {};
      }
    }

    template<typename T>
    static void writeToSlot(BifrostVM* self, std::size_t slot, const T& value)
    {
      using RawT = std::decay_t<T>;

      if constexpr (!std::is_same_v<RawT, RetainStack>)
      {
        if constexpr (std::is_same_v<RawT, std::string>)
        {
          bfVM_stackSetStringLen(self, slot, value.c_str(), value.length());
        }
        else if constexpr (std::is_same_v<RawT, StringRange>)
        {
          bfVM_stackSetStringLen(self, slot, value.bgn, value.end - value.bgn);
        }
        else if constexpr (std::is_same_v<RawT, char*> || std::is_same_v<RawT, const char*>)
        {
          bfVM_stackSetString(self, slot, value);
        }
        else if constexpr (std::is_same_v<RawT, bool>)
        {
          bfVM_stackSetBool(self, slot, value);
        }
        else if constexpr (std::is_same_v<RawT, std::nullptr_t>)
        {
          bfVM_stackSetNil(self, slot);
        }
        else if constexpr (std::is_arithmetic_v<RawT>)
        {
          bfVM_stackSetNumber(self, slot, static_cast<bfVMNumberT>(value));
        }
        else if constexpr (std::is_same_v<RawT, bfValueHandle>)
        {
          bfVM_stackLoadHandle(self, slot, value);
        }
        else if constexpr (std::is_pointer_v<T>)
        {
          bfVM_stackStoreWeakRef(self, slot, value);
        }
        else
        {
          static_assert(always_false<T>, "Type could not be automatically converted.");
        }
      }
    }

    template<typename... Args>
    void generateArgs(std::tuple<Args...>& arguments, const BifrostVM* vm)
    {
      std::size_t i = 0;
      meta::for_each(arguments, [vm, &i](auto&& arg) {
        arg = readFromSlot<std::decay_t<decltype(arg)>>(vm, i++);
      });
    }

    template<typename F, typename... Args>
    void vmCallArgsImpl(BifrostVM* vm, F&& fn, std::tuple<Args...>& arguments)
    {
      using fn_traits   = meta::function_traits<decltype(fn)>;
      using return_type = typename fn_traits::return_type;

      if constexpr (std::is_same_v<return_type, void>)
      {
        meta::apply(fn, arguments);
        bfVM_stackSetNil(vm, 0);
      }
      else
      {
        const auto& ret = meta::apply(fn, arguments);
        writeToSlot<return_type>(vm, 0, ret);
      }
    }

    template<typename F>
    void vmCallImpl(BifrostVM* vm, F&& fn)
    {
      using fn_traits   = meta::function_traits<decltype(fn)>;
      using return_type = typename fn_traits::return_type;

      typename fn_traits::tuple_type arguments;
      generateArgs(arguments, vm);

      vmCallArgsImpl(vm, fn, arguments);
    }
  }  // namespace detail

  template<typename ClzT, typename... Args>
  void vmNativeCtor(BifrostVM* vm, int32_t /*num_args*/)
  {
    detail::vmCallImpl(
     vm,
     [](ClzT* obj, Args... args) -> detail::RetainStack {
       new (obj) ClzT(std::forward<Args>(args)...);
       return {};
     });
  }

  template<auto fn>
  void vmNativeFnWrapper(BifrostVM* vm, int32_t num_args)
  {
    using fn_traits = meta::function_traits<decltype(fn)>;

    if (static_cast<std::size_t>(num_args) == fn_traits::arity)
    {
      detail::vmCallImpl(vm, fn);
    }
    else
    {
      throw "Invalid number of parameters passed to a native function.";  // NOLINT(hicpp-exception-baseclass)
    }
  }

  template<auto fn>
  void vmBindNativeFn(BifrostVM* self, size_t idx, const char* variable)
  {
    using fn_traits = meta::function_traits<decltype(fn)>;
    bfVM_stackStoreNativeFn(self, idx, variable, &vmNativeFnWrapper<fn>, static_cast<int32_t>(fn_traits::arity));
  }

  template<typename ClzT, typename... Args>
  BifrostMethodBind vmMakeCtorBinding(const char* name = "ctor", uint32_t num_statics = 0, uint16_t extra_data = 0u)
  {
    /* NOTE(Shareef): +1 for the self argument */
    return {name, &vmNativeCtor<ClzT, Args...>, sizeof...(Args) + 1, num_statics, extra_data};
  }

  template<auto mem_fn>
  BifrostMethodBind vmMakeMemberBinding(const char* name, uint32_t num_statics = 0, uint16_t extra_data = 0u)
  {
    return {
     name,
     &vmNativeFnWrapper<mem_fn>,
     meta::function_traits<decltype(mem_fn)>::arity,
     num_statics,
     extra_data,
    };
  }

  template<typename ClzT>
  void vmMakeFinalizer(BifrostVM* /* vm */, void* instance)
  {
    reinterpret_cast<ClzT*>(instance)->~ClzT();
  }

  template<typename ClzT, typename... Args>
  BifrostVMClassBind vmMakeClassBinding(const char* name, Args&&... methods)
  {
    static BifrostMethodBind s_Methods[] = {methods..., {nullptr, nullptr, 0, 0, 0}};

    BifrostVMClassBind clz_bind;

    clz_bind.name            = name;
    clz_bind.extra_data_size = sizeof(ClzT);
    clz_bind.methods         = s_Methods;
    clz_bind.finalizer       = &vmMakeFinalizer<ClzT>;

    return clz_bind;
  }

  struct FunctionCallResult
  {
    std::size_t    return_slot;
    BifrostVMError error;
  };

  template<typename... Args>
  FunctionCallResult vmCall(BifrostVM* self, size_t fn_idx, Args&&... args)
  {
    static constexpr std::size_t kNumArgs = sizeof...(Args);

    const size_t args_start    = fn_idx + 1;
    size_t       current_index = args_start;

    bfVM_stackResize(self, args_start + kNumArgs);

    (detail::writeToSlot(self, current_index++, std::forward<Args>(args)), ...);

    (void)current_index;

    const auto err = bfVM_call(self, fn_idx, args_start, static_cast<int32_t>(kNumArgs));

    return {args_start, err};
  }

  struct VMParams final : public BifrostVMParams
  {
    explicit VMParams() noexcept :
      BifrostVMParams()
    {
      bfVMParams_init(this);
    }
  };

  // Non Owning Virtual Machine Pointer for an OOP API over an existing VM.
  class VMView
  {
   protected:
    BifrostVM* m_Self;

   public:
    explicit VMView(BifrostVM* self) noexcept :
      m_Self{self}
    {
    }

    VMView(const VMView& rhs) = default;
    VMView(VMView&& rhs)      = default;
    VMView& operator=(const VMView& rhs) = default;
    VMView& operator=(VMView&& rhs) = default;
    ~VMView()                       = default;

    [[nodiscard]] BifrostVM*       self() noexcept { return m_Self; }
    [[nodiscard]] const BifrostVM* self() const noexcept { return m_Self; }
    operator BifrostVM*() const { return m_Self; }
    [[nodiscard]] bool isValid() const noexcept { return m_Self != nullptr; }

    BifrostVMError moduleMake(size_t idx, const char* module) noexcept
    {
      return bfVM_moduleMake(self(), idx, module);
    }

    void moduleLoad(size_t idx, uint32_t std_module_flags) noexcept
    {
      bfVM_moduleLoadStd(self(), idx, std_module_flags);
    }

    BifrostVMError moduleLoad(size_t idx, const char* module) noexcept
    {
      return bfVM_moduleLoad(self(), idx, module);
    }

    void moduleUnload(const char* module) noexcept
    {
      bfVM_moduleUnload(self(), module);
    }

    size_t stackSize() const noexcept
    {
      return bfVM_stackSize(self());
    }

    BifrostVMError stackResize(size_t size) noexcept
    {
      return bfVM_stackResize(self(), size);
    }

    void stackMakeInstance(size_t clz_idx, size_t dst_idx) noexcept
    {
      bfVM_stackMakeInstance(self(), clz_idx, dst_idx);
    }

    void* stackMakeReference(size_t idx, size_t extra_data_size) noexcept
    {
      return bfVM_stackMakeReference(self(), idx, extra_data_size);
    }

    void* stackMakeReference(size_t module_idx, const BifrostVMClassBind& class_bind, size_t dst_idx) noexcept
    {
      return bfVM_stackMakeReferenceClz(self(), module_idx, &class_bind, dst_idx);
    }

    void stackMakeWeakRef(size_t idx, void* value) noexcept
    {
      bfVM_stackMakeWeakRef(self(), idx, value);
    }

    void stackLoadVariable(size_t idx, size_t inst_or_class_or_module, const char* variable) noexcept
    {
      bfVM_stackLoadVariable(self(), idx, inst_or_class_or_module, variable);
    }

    void stackStore(size_t idx, const char* variable_name, size_t value_src_idx) noexcept
    {
      bfVM_stackStoreVariable(self(), idx, variable_name, value_src_idx);
    }

    void stackStore(size_t idx, const char* variable, bfNativeFnT func, int32_t arity) noexcept
    {
      bfVM_stackStoreNativeFn(self(), idx, variable, func, arity);
    }

    BifrostVMError stackStoreClosure(size_t inst_or_class_or_module, const char* field, bfNativeFnT func, int32_t arity, uint32_t num_statics = 0u, uint16_t extra_data = 0u)
    {
      return bfVM_stackStoreClosure(self(), inst_or_class_or_module, field, func, arity, num_statics, extra_data);
    }

    BifrostVMError closureGetStatic(size_t dst_idx, size_t static_idx)
    {
      return bfVM_closureGetStatic(self(), dst_idx, static_idx);
    }

    BifrostVMError closureSetStatic(size_t closure_idx, size_t static_idx, size_t value_idx)
    {
      return bfVM_closureSetStatic(self(), closure_idx, static_idx, value_idx);
    }

    // Compile time Fns
    template<auto fn>
    void stackStore(size_t idx, const char* variable) noexcept
    {
      vmBindNativeFn<fn>(self(), idx, variable);
    }

    template<typename Fn>
    void stackStore(size_t idx, const char* variable, Fn&& func) noexcept
    {
      using fn_traits = meta::function_traits<decltype(func)>;

      const std::size_t old_size = stackSize();

      if constexpr (fn_traits::is_member_fn)
      {
        // NOTE(Shareef): Space for the 'idx' (module) and 'idx + 1' (Closure)

        stackResize(idx + 2);
        stackStoreClosure(idx, variable, &callMember<Fn>, fn_traits::arity, 1, sizeof(func));
        stackLoadVariable(idx + 1, idx, variable);

        void* const extra_data = bfVM_closureStackGetExtraData(self(), idx + 1);
        std::memcpy(extra_data, &func, sizeof(func));
      }
      else
      {
        BifrostMethodBind call_method_bind{};
        call_method_bind.name  = bfVM_buildInSymbolStr(self(), BIFROST_VM_SYMBOL_CALL);
        call_method_bind.fn    = &callImpl_<Fn>;
        call_method_bind.arity = fn_traits::arity + 1;  // NOTE(Shareef): + 1 for the Fn 'class'.

        const BifrostVMClassBind clz_bind = vmMakeClassBinding<Fn>(variable, call_method_bind);

        // NOTE(Shareef): Space for the 'idx' (module) and 'idx + 1' (reference).
        stackResize(idx + 2);
        void* func_storage = stackMakeReference(idx, clz_bind, idx + 1);
        new (func_storage) Fn(std::forward<Fn>(func));
        stackStore(idx, variable, idx + 1);
      }

      stackResize(old_size);
    }

    void stackStore(size_t idx, const BifrostVMClassBind& clz_bind) noexcept
    {
      bfVM_stackStoreClass(self(), idx, &clz_bind);
    }

    void referenceSetClass(size_t idx, size_t class_idx) noexcept
    {
      bfVM_referenceSetClass(self(), idx, class_idx);
    }

    void stackSet(size_t idx, const std::string& value) noexcept
    {
      stackSet(idx, value.c_str(), value.length());
    }

    void stackSet(size_t idx, const char* value) noexcept
    {
      bfVM_stackSetString(self(), idx, value);
    }

    void stackSet(size_t idx, const char* value, size_t len) noexcept
    {
      bfVM_stackSetStringLen(self(), idx, value, len);
    }

    void stackSet(size_t idx, bfVMNumberT value) noexcept
    {
      bfVM_stackSetNumber(self(), idx, value);
    }

    void stackSet(size_t idx, bfBool32 value) noexcept
    {
      bfVM_stackSetBool(self(), idx, value);
    }

    void stackSet(size_t idx, std::nullptr_t) noexcept
    {
      bfVM_stackSetNil(self(), idx);
    }

    [[nodiscard]] void* stackReadInstance(size_t idx) const noexcept
    {
      return bfVM_stackReadInstance(self(), idx);
    }

    [[nodiscard]] StringRange stackReadString(size_t idx) const noexcept
    {
      size_t      str_len;
      const char* str = bfVM_stackReadString(self(), idx, &str_len);

      return bfMakeStringRangeLen(str, str_len);
    }

    [[nodiscard]] bfVMNumberT stackReadNumber(size_t idx) const noexcept
    {
      return bfVM_stackReadNumber(self(), idx);
    }

    [[nodiscard]] bfBool32 stackReadBool(size_t idx) const noexcept
    {
      return bfVM_stackReadBool(self(), idx);
    }

    [[nodiscard]] BifrostVMType stackGetType(size_t idx) noexcept
    {
      return bfVM_stackGetType(self(), idx);
    }

    [[nodiscard]] int32_t stackGetArity(size_t idx) noexcept
    {
      return bfVM_stackGetArity(self(), idx);
    }

    [[nodiscard]] bfValueHandle stackMakeHandle(size_t idx) noexcept
    {
      return bfVM_stackMakeHandle(self(), idx);
    }

    void stackLoadHandle(size_t dst_idx, bfValueHandle handle) noexcept
    {
      bfVM_stackLoadHandle(self(), dst_idx, handle);
    }

    void stackDestroyHandle(bfValueHandle handle) noexcept
    {
      bfVM_stackDestroyHandle(self(), handle);
    }

    [[nodiscard]] static int32_t handleGetArity(bfValueHandle handle) noexcept
    {
      return bfVM_handleGetArity(handle);
    }

    [[nodiscard]] static BifrostVMType handleGetType(bfValueHandle handle) noexcept
    {
      return bfVM_handleGetType(handle);
    }

    BifrostVMError callRaw(size_t idx, size_t args_start, int32_t num_args) noexcept
    {
      return bfVM_call(self(), idx, args_start, num_args);
    }

    template<typename... Args>
    FunctionCallResult call(size_t fn_idx, Args&&... args) noexcept
    {
      return vmCall(self(), fn_idx, std::forward<Args>(args)...);
    }

    BifrostVMError execInModule(const char* module, const char* source, size_t source_length) noexcept
    {
      return bfVM_execInModule(self(), module, source, source_length);
    }

    void gc() noexcept
    {
      bfVM_gc(self());
    }

    [[nodiscard]] const char* buildInSymbolStr(BifrostVMBuildInSymbol symbol) const noexcept
    {
      return bfVM_buildInSymbolStr(self(), symbol);
    }

    [[nodiscard]] const char* errorString() const noexcept
    {
      return bfVM_errorString(self());
    }

    // TODO(SR): This should be part of the C-API as well.
    [[nodiscard]] StringRange errorStringRange() const noexcept
    {
      const char* err_str = bfVM_errorString(self());
      return {err_str, err_str + String_length(err_str)};
    }

   private:
    template<typename FObj, typename Ret, typename... Args>
    static Ret stackStore_(FObj* obj, Args&&... args)
    {
      return meta::invoke(obj, args);
    }

    template<typename Fn>
    static void callImplShared(BifrostVM* vm, Fn* fn_obj)
    {
      using fn_traits   = meta::function_traits<Fn>;
      using return_type = typename fn_traits::return_type;

      typename fn_traits::tuple_type arguments;
      detail::generateArgs(arguments, vm);

      if constexpr (std::is_same_v<return_type, void>)
      {
        meta::apply(*fn_obj, arguments);
        bfVM_stackSetNil(vm, 0);
      }
      else
      {
        const auto& ret = meta::apply(*fn_obj, arguments);
        detail::writeToSlot<return_type>(vm, 0, ret);
      }
    }

    template<typename Fn>
    static void callImpl_(BifrostVM* vm, int /*arity*/)
    {
      callImplShared<Fn>(vm, static_cast<Fn*>(bfVM_stackReadInstance(vm, 0)));
    }

    template<typename Fn>
    static void callMember(BifrostVM* vm, int /*arity*/)
    {
      callImplShared<Fn>(vm, static_cast<Fn*>(bfVM_closureGetExtraData(vm)));
    }
  };

  // clang-format off
  class VM final : private bfNonCopyable<VM>, public VMView  // NOLINT(hicpp-special-member-functions)
  // clang-format on
  {
   private:
    BifrostVM m_VM;

   public:
    explicit VM(const BifrostVMParams& params) noexcept :
      VM()
    {
      create(params);
    }

    explicit VM() noexcept :
      VMView(nullptr),
      m_VM{}
    {
    }

    explicit VM(VM&& rhs) noexcept :
      VMView(rhs.m_Self),
      m_VM{rhs.m_VM}
    {
      rhs.m_Self = nullptr;
    }

    void create(const BifrostVMParams& params) noexcept
    {
      if (!isValid())
      {
        m_Self = &m_VM;
        bfVM_ctor(self(), &params);
      }
    }

    VM& operator=(VM&& rhs) noexcept
    {
      if (this != &rhs)
      {
        destroy();

        if (rhs.isValid())
        {
          m_Self = &m_VM;
          std::memcpy(self(), rhs.self(), sizeof(BifrostVM));
        }
        else
        {
          m_Self = nullptr;
        }

        rhs.m_Self = nullptr;
      }

      return *this;
    }

    void destroy() noexcept
    {
      if (isValid())
      {
        bfVM_dtor(self());
        m_Self = nullptr;
      }
    }

    ~VM() noexcept
    {
      destroy();
    }
  };
}  // namespace bifrost

#endif /* BIFROST_VM_HPP */
