/*!
 * @file   bifrost_meta_function_traits.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Allows for compile time introspection on the properties of
 *   all types of callable objects.
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_META_FUNCTION_TRAIT_HPP
#define BIFROST_META_FUNCTION_TRAIT_HPP

#include <functional>  /* invoke                                           */
#include <tuple>       /* tuple, tuple_element, apply                      */
#include <type_traits> /* decay_t, invoke_result_t, is_nothrow_invocable_v */

namespace bifrost::meta
{
  //
  // 'tuple_type_raw' is for the arguments of a member function / pointer without the class as the first argument
  //

  // NOTE(Shareef):
  //   How to use 'ParameterPack':
  //     using ParamPack = ParameterPack<A, B, C>;
  //     template <typename...> struct S {};
  //     ParamPack::apply<S> var;   [Equivalent to `S<A, B, C> var;`]
  template<typename... P>
  struct ParameterPack
  {
    template<template<typename...> typename T>
    using apply                = T<P...>;
    static constexpr auto size = sizeof...(P);
  };

  template<typename F>
  struct function_traits;

  template<typename... Args>
  using FunctionTuple = std::tuple<Args... /*std::remove_const_t<Args>...*/>;

  namespace detail
  {
    template<typename TFirst, typename... TRest>
    struct remove_first_tuple
    {
      using type = std::tuple<TRest...>;
    };

    template<typename TFirst, typename... TRest>
    struct remove_first_tuple<std::tuple<TFirst, TRest...>>
    {
      using type = std::tuple<TRest...>;
    };

    template<typename TFirst, typename... TRest>
    using remove_first_tuple_t = typename remove_first_tuple<TFirst, TRest...>::type;
  }  // namespace detail

  template<typename R, typename... Args>
  struct function_traits<R(Args...)>
  {
    static constexpr std::size_t arity = sizeof...(Args);
    using return_type                  = R;
    using tuple_type                   = FunctionTuple<Args...>;
    using parameter_pack               = ParameterPack<Args...>;

    template<std::size_t N>
    struct argument
    {
      static_assert(N < arity, "error: invalid parameter index.");
      using type = typename std::tuple_element<N, tuple_type>::type;
    };
  };

  // function pointer
  template<typename R, typename... Args>
  struct function_traits<R (*)(Args...)> : public function_traits<R(Args...)>
  {
    static constexpr bool is_member_fn = false;
  };

  // member function pointer
  template<typename C, typename R, typename... Args>
  struct function_traits<R (C::*)(Args...)> : public function_traits<R(C*, Args...)>
  {
    static constexpr bool is_member_fn = true;

    using class_type     = std::decay_t<C>;
    using tuple_type_raw = FunctionTuple<Args...>;
  };

  // const member function pointer
  template<typename C, typename R, typename... Args>
  struct function_traits<R (C::*)(Args...) const> : public function_traits<R(const C*, Args...)>
  {
    static constexpr bool is_member_fn = true;

    using class_type     = std::decay_t<C>;
    using tuple_type_raw = FunctionTuple<Args...>;
  };

  // In C++17 and onward 'noexcept'-ness is part of a function's type.
#if __cplusplus > 201402L  // After C++14

  template<typename R, typename... Args>
  struct function_traits<R (*)(Args...) noexcept> : public function_traits<R(Args...)>
  {
    static constexpr bool is_member_fn = true;
  };

  // noexcept member function pointer
  template<typename C, typename R, typename... Args>
  struct function_traits<R (C::*)(Args...) noexcept> : public function_traits<R(C*, Args...)>
  {
    static constexpr bool is_member_fn = true;
  };

  // noexcept const member function pointer
  template<typename C, typename R, typename... Args>
  struct function_traits<R (C::*)(Args...) const noexcept> : public function_traits<R(C*, Args...)>
  {
    static constexpr bool is_member_fn = true;
  };

#endif

  // member object pointer
  template<typename C, typename R>
  struct function_traits<R(C::*)> : public function_traits<R(C&)>
  {
    static constexpr bool is_member_fn = false;

    using class_type     = std::decay_t<C>;
    using tuple_type_raw = std::tuple<>;
  };

  // function objects / lambdas
  template<typename F>
  struct function_traits
  {
    static constexpr bool is_member_fn = false;

   private:
    using call_type = function_traits<decltype(&F::operator())>;

   public:
    static constexpr std::size_t arity = call_type::arity - 1;
    using return_type                  = typename call_type::return_type;
    using tuple_type                   = detail::remove_first_tuple_t<typename call_type::tuple_type>;
    using parameter_pack               = detail::remove_first_tuple_t<typename call_type::parameter_pack>;

    template<std::size_t N>
    struct argument
    {
      static_assert(N < arity, "error: invalid parameter index.");
      using type = typename call_type::template argument<N + 1>::type;
    };
  };

  // function that is passed as a r-value / universal reference.
  template<typename F>
  struct function_traits<F&&> : public function_traits<F>
  {
  };

  // function reference
  template<typename F>
  struct function_traits<F&> : public function_traits<F>
  {
  };

  template<typename F, typename... Args>
  std::invoke_result_t<F, Args...> invoke(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>)
  {
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
  }

  template<typename F, typename Tuple>
  constexpr decltype(auto) apply(F&& f, Tuple&& t)
  {
    return std::apply(std::forward<F>(f), std::forward<Tuple>(t));
  }

  template<typename T, typename Tuple, size_t... Is>
  void construct_from_tuple_(T* obj, Tuple&& tuple, std::index_sequence<Is...>)
  {
    new (obj) T(std::get<Is>(std::forward<Tuple>(tuple))...);
  }

  template<typename T, typename Tuple>
  void construct_from_tuple(T* obj, Tuple&& tuple)
  {
    construct_from_tuple_(obj, std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>::value>{});
  }

  // https://dev.krzaq.cc/post/you-dont-need-a-stateful-deleter-in-your-unique_ptr-usually/
  // This is useful for stateless unique_ptrs.
  template<auto func>
  struct function_caller final
  {
    template<typename... Us>
    auto operator()(Us&&... us) const
    {
      return func(std::forward<Us...>(us...));
    }
  };
}  // namespace bifrost::meta

#endif /* BIFROST_META_FUNCTION_TRAIT_HPP */