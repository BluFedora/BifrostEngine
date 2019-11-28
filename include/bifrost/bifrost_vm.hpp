// [https://blog.tartanllama.xyz/exploding-tuples-fold-expressions/]

#ifndef BIFROST_VM_HPP
#define BIFROST_VM_HPP

#include "meta/bifrost_meta_function_traits.hpp" /* function_traits          */
#include "meta/bifrost_meta_utils.hpp"           /* for_each                 */
#pragma warning(push, 0)
#include "script/bifrost_vm_internal.h"          /* bfVM__value*             */
#pragma warning(pop)
#include "utility/bifrost_non_copy_move.hpp"     /* bfNonCopyMoveable        */
#include <string>                                /* string                   */
#include <tuple>                                 /* tuple                    */
#include <type_traits>                           /* decay_t, is_arithmetic_v */

namespace bifrost
{
  using StringRange = bfStringRange;

  namespace detail
  {
    template<typename...>
    constexpr std::false_type always_false{};

    //
    // Tag type for 'writeToSlot' so that the stack is not written to.
    //
    struct RetainStack final
    {
    };

    template<typename T>
    static T readFromSlot(const BifrostVM* self, std::size_t slot)
    {
      // TODO(SR): This needs some type checking.
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
          std::size_t str_size;
          return bfVM_stackReadString(self, slot, &str_size);
        }
        else
        {
          return reinterpret_cast<T>(bfVM_stackReadInstance(self, slot));
        }
      }
      // TODO(SR): Currently this could never be hit because of the impl. This is bad.
      else if constexpr (std::is_reference_v<T>)
      {
        static_assert(always_false<T>, "Bifrost Script does not support references similar to Lua in a way.");
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
        else
        {
          // TODO(SR): When we add 'light userdata' aka references then we can just return that.
          static_assert(always_false<T>, "Type could not be automatically converted.");
          bfVM_stackSetNil(self, slot);
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

    template<typename F>
    void vmCallImpl(BifrostVM* vm, F&& fn)
    {
      using fn_traits   = meta::function_traits<decltype(fn)>;
      using return_type = typename fn_traits::return_type;

      typename fn_traits::tuple_type arguments;
      generateArgs(arguments, vm);

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
      throw "Invalid number of parameters passed to a native function.";
    }
  }

  template<auto fn>
  void vmBindNativeFn(BifrostVM* self, size_t idx, const char* variable)
  {
    using fn_traits = meta::function_traits<decltype(fn)>;
    bfVM_moduleBindNativeFn(self, idx, variable, &vmNativeFnWrapper<fn>, static_cast<int32_t>(fn_traits::arity));
  }

  template<typename ClzT, typename... Args>
  BifrostMethodBind vmMakeCtorBinding(const char* name = "ctor")
  {
    /* NOTE(Shareef): +1 for the self argument */
    return {name, &vmNativeCtor<ClzT, Args...>, sizeof...(Args) + 1};
  }

  template<auto mem_fn>
  BifrostMethodBind vmMakeMemberBinding(const char* name)
  {
    return {
     name,
     &vmNativeFnWrapper<mem_fn>,
     meta::function_traits<decltype(mem_fn)>::arity,
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
    static BifrostMethodBind s_Methods[] = {methods..., {nullptr, nullptr, 0}};

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

  struct VMParams : public BifrostVMParams
  {
    explicit VMParams() :
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
    explicit VMView(BifrostVM* self) :
      m_Self{self}
    {
    }

    VMView(const VMView& rhs) = default;
    VMView(VMView&& rhs)      = default;
    VMView& operator=(const VMView& rhs) = default;
    VMView& operator=(VMView&& rhs) = default;
    ~VMView() = default;

    [[nodiscard]] BifrostVM*       self() { return m_Self; }
    [[nodiscard]] const BifrostVM* self() const noexcept { return m_Self; }
    operator BifrostVM*() const { return m_Self; }
    [[nodiscard]] bool             isValid() const noexcept { return m_Self != nullptr; }

    BifrostVMError moduleMake(size_t idx, const char* module) noexcept
    {
      return bfVM_moduleMake(self(), idx, module);
    }

    BifrostVMError moduleLoad(size_t idx, const char* module) noexcept
    {
      return bfVM_moduleLoad(self(), idx, module);
    }

    void moduleBind(size_t idx, const char* variable, bfNativeFnT func, int32_t arity) noexcept
    {
      bfVM_moduleBindNativeFn(self(), idx, variable, func, arity);
    }

    void moduleBind(size_t idx, const BifrostVMClassBind& clz_bind) noexcept
    {
      bfVM_moduleBindClass(self(), idx, &clz_bind);
    }

    template<auto fn>
    void moduleBind(size_t idx, const char* variable) noexcept
    {
      vmBindNativeFn<fn>(self(), idx, variable);
    }

    void moduleStoreVariable(size_t module_idx, const char* variable_name, size_t value_src_idx) noexcept
    {
      bfVM_moduleStoreVariable(self(), module_idx, variable_name, value_src_idx);
    }

    void moduleUnload(const char* module) noexcept
    {
      bfVM_moduleUnload(self(), module);
    }

    BifrostVMError stackResize(size_t size) noexcept
    {
      return bfVM_stackResize(self(), size);
    }

    void stackMakeInstance(size_t clz_idx, size_t dst_idx) noexcept
    {
      bfVM_stackMakeInstance(self(), clz_idx, dst_idx);
    }

    void stackLoadVariable(size_t idx, size_t inst_or_class_or_module, const char* variable) noexcept
    {
      bfVM_stackLoadVariable(self(), idx, inst_or_class_or_module, variable);
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
  };

  // clang-format off
  // This class is movable but not copyable.
  class VM final : private bfNonCopyable<VM>, public VMView
  // clang-format on
  {
   private:
    BifrostVM m_VM;

   public:
    explicit VM(const BifrostVMParams& params) noexcept :
      VMView(&m_VM),
      m_VM{}
    {
      // Doesn't call 'VM::create' because of the 'noexcept' guarantee.
      bfVM_ctor(self(), &params);
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

    void create(const BifrostVMParams& params) noexcept(false)
    {
      if (!m_Self)
      {
        throw "Called create on VM when it is already created.";
      }

      m_Self = &m_VM;
      bfVM_ctor(self(), &params);
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

    //
    // TODO(SR):
    //   Currently this can be called on an invalid vm.
    //   Is that good behavior considering 'create' must be called
    //   be called in an invalid state thus this API is unbalanced.
    //
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
