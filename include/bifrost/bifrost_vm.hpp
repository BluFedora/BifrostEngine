// [https://blog.tartanllama.xyz/exploding-tuples-fold-expressions/]

#ifndef BIFROST_VM_HPP
#define BIFROST_VM_HPP

#include "meta/bifrost_meta_utils.hpp"  /* for_each                 */
#include "script/bifrost_vm_internal.h" /*                          */
#include <string>                       /* string                   */
#include <tuple>                        /* tuple                    */
#include <type_traits>                  /* decay_t, is_arithmetic_v */

namespace bifrost
{
  namespace detail
  {
    template<class...>
    constexpr std::false_type always_false{};

    template<typename T>
    struct ValueConvert
    {
      static T convert(bfVMValue value)
      {
        if constexpr (std::is_arithmetic_v<T>)
        {
          return static_cast<T>(bfVM__valueToNumber(value));
        }
        else if constexpr (std::is_pointer_v<T>)
        {
          if (bfVM__valueIsNull(value))
          {
            return nullptr;
          }

          return reinterpret_cast<T*>(bfVM__valueToPointer(value));
        }
        else if constexpr (std::is_reference_v<T>)
        {
          return *reinterpret_cast<T*>(bfVM__valueToPointer(value));
        }
        else
        {
          static_assert(always_false<T>, "Type could not be automatrically converted.");
          return {};
        }
      }
    };

    template<typename T>
    static void writeToSlot(BifrostVM* self, std::size_t slot, const T& value)
    {
      using RawT = std::decay_t<T>;

      // TODO(SR): Add bfStringRange
      if constexpr (std::is_same_v<RawT, std::string>)
      {
        bfVM_stackSetStringLen(self, slot, value.c_str(), value.length());
      }
      else if constexpr (std::is_same_v<RawT, char*> || std::is_same_v<RawT, const char*>)
      {
        bfVM_stackSetString(self, slot, value);
      }
      else if constexpr (std::is_same_v<RawT, bool>)
      {
        bfVM_stackSetBool(self, slot, value);
      }
      else if constexpr (std::is_arithmetic_v<RawT>)
      {
        bfVM_stackSetNumber(self, slot, static_cast<bfVMNumberT>(value));
      }
      else
      {
        // TODO(SR): When we add 'light userdata' aka references then we can just return that.
        static_assert(always_false<T>, "Type could not be automatically converted.");
      }
    }

    template<typename ObjT, typename Function, typename Tuple, size_t... I>
    auto callMember(ObjT* obj, Function f, Tuple t, std::index_sequence<I...>)
    {
      return (obj->*f)(std::get<I>(t)...);
    }

    template<typename ObjT, typename Function, typename Tuple>
    auto callMember(ObjT* obj, Function f, Tuple t)
    {
      static constexpr auto size = std::tuple_size<Tuple>::value;
      return callMember(obj, f, t, std::make_index_sequence<size>{});
    }

    template<typename Function, typename Tuple, size_t... I>
    auto callFn(Function f, Tuple t, std::index_sequence<I...>)
    {
      (void)t;
      return f(std::get<I>(t)...);
    }

    template<typename Function, typename Tuple>
    auto callFn(Function f, Tuple t)
    {
      static constexpr auto size = std::tuple_size<Tuple>::value;
      return callFn(f, t, std::make_index_sequence<size>{});
    }

    template<class T, class Tuple, size_t... Is>
    void construct_from_tuple(T* obj, Tuple&& tuple, std::index_sequence<Is...>)
    {
      new (obj) T(std::get<Is>(std::forward<Tuple>(tuple))...);
    }

    template<class T, class Tuple>
    void construct_from_tuple(T* obj, Tuple&& tuple)
    {
      construct_from_tuple<T>(
       obj,
       std::forward<Tuple>(tuple),
       std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
    }

    template<typename... Args>
    void generateArgs(std::tuple<Args...>& arguments, BifrostVM* vm, size_t i = 1)
    {
      meta::for_each(arguments, [vm, &i](auto&& arg) {
        arg = ValueConvert<std::decay_t<decltype(arg)>>::convert(vm->stack_top[i]);
        ++i;
      });
    }

    template<typename ClzT, ClzT>
    struct ProxyMemFn;

    template<typename ClzT, typename Ret, typename... Args, Ret (ClzT::*memfn)(Args...)>
    struct ProxyMemFn<Ret (ClzT::*)(Args...), memfn>
    {
      static constexpr std::size_t arity = sizeof...(Args);

      static void apply(BifrostVM* vm, int32_t num_args)
      {
        static constexpr std::size_t kNumArgs = sizeof...(Args);

        if (int32_t(kNumArgs + 1) == num_args)
        {
          std::tuple<Args...> arguments;
          generateArgs(arguments, vm);

          ClzT* obj = reinterpret_cast<ClzT*>(bfVM_stackReadInstance(vm, 0));

          if constexpr (std::is_same_v<Ret, void>)
          {
            callMember(obj, memfn, arguments);
            bfVM_stackSetNil(vm, 0);
          }
          else
          {
            const auto& ret = callMember(obj, memfn, arguments);

            if constexpr (std::is_same_v<Ret, std::string>)
            {
              bfVM_stackSetStringLen(vm, 0, ret.c_str(), ret.size());
            }
            else if constexpr (std::is_same_v<Ret, bool>)
            {
              bfVM_stackSetBool(vm, 0, ret);
            }
            else if constexpr (std::is_same_v<Ret, const char*>)
            {
              bfVM_stackSetString(vm, 0, ret);
            }
            else if constexpr (std::is_arithmetic_v<Ret>)
            {
              bfVM_stackSetNumber(vm, 0, bfVMNumberT(ret));
            }
            else
            {
              // TODO(SR): When we add 'light userdata' aka references this can just return that.
              bfVM_stackSetNil(vm, 0);
            }
          }
        }
        else
        {
          throw "Function called with invalid number of parameters.";
        }
      }
    };

    template<typename ClzT, typename Ret, typename... Args, Ret (ClzT::*memfn)(Args...) const>
    struct ProxyMemFn<Ret (ClzT::*)(Args...) const, memfn>
    {
      static constexpr std::size_t arity = sizeof...(Args);

      static void apply(BifrostVM* vm, int32_t num_args)
      {
        static constexpr std::size_t kNumArgs = sizeof...(Args);

        if (int32_t(kNumArgs + 1) == num_args)
        {
          std::tuple<Args...> arguments;
          generateArgs(arguments, vm);

          ClzT* obj = reinterpret_cast<ClzT*>(bfVM_stackReadInstance(vm, 0));

          if constexpr (std::is_same_v<Ret, void>)
          {
            callMember(obj, memfn, arguments);
            bfVM_stackSetNil(vm, 0);
          }
          else
          {
            const auto& ret = callMember(obj, memfn, arguments);

            if constexpr (std::is_same_v<Ret, std::string>)
            {
              bfVM_stackSetStringLen(vm, 0, ret.c_str(), ret.size());
            }
            else if constexpr (std::is_same_v<Ret, bool>)
            {
              bfVM_stackSetBool(vm, 0, ret);
            }
            else if constexpr (std::is_same_v<Ret, const char*>)
            {
              bfVM_stackSetString(vm, 0, ret);
            }
            else if constexpr (std::is_arithmetic_v<Ret>)
            {
              bfVM_stackSetNumber(vm, 0, bfVMNumberT(ret));
            }
            else
            {
              // TODO(SR): When we add 'light userdata' aka references this can just return that.
              bfVM_stackSetNil(vm, 0);
            }
          }
        }
        else
        {
          throw "Function called with invalid number of parameters.";
        }
      }
    };
  }  // namespace detail

  template<typename ClzT, typename... Args>
  void vmNativeCtor(BifrostVM* vm, int32_t num_args)
  {
    (void)num_args;

    ClzT* const obj = reinterpret_cast<ClzT*>(bfVM_stackReadInstance(vm, 0));

    std::tuple<Args...> arguments;
    detail::generateArgs(arguments, vm);

    detail::construct_from_tuple(obj, arguments);
  }

  using GetArityT = unsigned;

  template<typename T>
  struct get_arity;

  template<typename R, typename... Args>
  struct get_arity<R(Args...)> : public std::integral_constant<GetArityT, sizeof...(Args)>
  {
    using FRet      = R;
    using TuplePack = std::tuple<Args...>;
  };
  template<typename R, typename C, typename... Args>
  struct get_arity<R (C::*)(Args...)> : public std::integral_constant<GetArityT, sizeof...(Args)>
  {
    using FRet      = R;
    using TuplePack = std::tuple<Args...>;
  };
  template<typename R, typename C, typename... Args>
  struct get_arity<R (C::*)(Args...) const> : public std::integral_constant<GetArityT, sizeof...(Args)>
  {
    using FRet      = R;
    using TuplePack = std::tuple<Args...>;
  };

  template<typename FnT, FnT* fn>
  void vmNativeFnWrapper(BifrostVM* vm, int32_t num_args)
  {
    (void)num_args;

    if (static_cast<GetArityT>(num_args) == get_arity<FnT>::value)
    {
      typename get_arity<FnT>::TuplePack arguments;
      detail::generateArgs(arguments, vm, 0);
      detail::callFn(fn, arguments);
    }
    else
    {
      throw "Invalid number of parameters passed to a native function.";
    }
  }

  template<typename FnT, FnT* fn>
  void vmBindNativeFn(BifrostVM* self, size_t idx, const char* variable)
  {
    bfVM_moduleBindNativeFn(self, idx, variable, &vmNativeFnWrapper<FnT, fn>, static_cast<int32_t>(get_arity<FnT>::value));
  }

  template<typename ClzT, typename... Args>
  BifrostMethodBind vmMakeCtorBinding(const char* name)
  {
    return {
     name,
     &vmNativeCtor<ClzT, Args...>,
     sizeof...(Args) + 1u /* NOTE(Shareef): +1 for the self argument */
    };
  }

#define bfVM_makeMemberFunction(mem_fn) (&bifrost::detail::ProxyMemFn<decltype(mem_fn), mem_fn>::apply)
#define bfVM_makeMemberBinding(name, mem_fn)                           \
  BifrostMethodBind                                                    \
  {                                                                    \
    (name),                                                            \
     (&bifrost::detail::ProxyMemFn<decltype(mem_fn), mem_fn>::apply),  \
     bifrost::detail::ProxyMemFn<decltype(mem_fn), mem_fn>::arity + 1u \
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
    clz_bind.finalizer       = &bifrost::vmMakeFinalizer<ClzT>;

    return clz_bind;
  }

  struct bfFunctionCallResult
  {
    size_t         return_slot;
    BifrostVMError error;
  };

  template<typename... Args>
  bfFunctionCallResult vmCall(BifrostVM* self, size_t fn_idx, Args&&... args)
  {
    static constexpr std::size_t kNumArgs = sizeof...(Args);

    const size_t args_start = fn_idx + 1;

    bfVM_stackResize(self, args_start + kNumArgs);

    size_t current_index = args_start;

    (detail::writeToSlot(self, current_index++, std::forward<Args>(args)), ...);

    const auto err = bfVM_call(self, fn_idx, args_start, static_cast<int32_t>(kNumArgs));

    return {args_start, err};
  }
}  // namespace bifrost

#endif /* BIFROST_VM_HPP */
