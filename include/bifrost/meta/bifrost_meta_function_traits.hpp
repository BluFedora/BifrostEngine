// [https://functionalcpp.wordpress.com/2013/08/05/function-traits/]
// [https://en.cppreference.com/w/cpp/types/result_of]
//
// 'tuple_type_raw' is for the arguments of a member function / pointer without the class as the first argument
//
#ifndef BIFROST_META_FUNCTION_TRAIT_HPP
#define BIFROST_META_FUNCTION_TRAIT_HPP

#include <tuple>       /* tuple, tuple_size, get                                           */
#include <type_traits> /* index_sequence, make_index_sequence, remove_reference_t, decay_t */

namespace bifrost::meta
{
  template<class F>
  struct function_traits;

  // function pointer
  template<class R, class... Args>
  struct function_traits<R (*)(Args...)> : public function_traits<R(Args...)>
  {
  };

  template<class R, class... Args>
  struct function_traits<R(Args...)>
  {
    static constexpr std::size_t arity = sizeof...(Args);
    using return_type                  = R;
    using tuple_type                   = std::tuple<Args...>;

    template<std::size_t N>
    struct argument
    {
      static_assert(N < arity, "error: invalid parameter index.");
      using type = typename std::tuple_element<N, tuple_type>::type;
    };
  };

  // member function pointer
  template<class C, class R, class... Args>
  struct function_traits<R (C::*)(Args...)> : public function_traits<R(C&, Args...)>
  {
    using class_type     = std::decay_t<C>;
    using tuple_type_raw = std::tuple<Args...>;
  };

  // const member function pointer
  template<class C, class R, class... Args>
  struct function_traits<R (C::*)(Args...) const> : public function_traits<R(const C&, Args...)>
  {
    using class_type     = std::decay_t<C>;
    using tuple_type_raw = std::tuple<Args...>;
  };

  // In C++17 and on 'noexcept'-ness is part of a function's type.
#if __cplusplus > 201402L  // After C++14

  template<class R, class... Args>
  struct function_traits<R (*)(Args...) noexcept> : public function_traits<R(Args...)>
  {
  };

  // noexcept member function pointer
  template<class C, class R, class... Args>
  struct function_traits<R (C::*)(Args...) noexcept> : public function_traits<R(C&, Args...)>
  {
  };

  // noexcept const member function pointer
  template<class C, class R, class... Args>
  struct function_traits<R (C::*)(Args...) const noexcept> : public function_traits<R(C&, Args...)>
  {
  };

#endif

  // member object pointer
  template<class C, class R>
  struct function_traits<R(C::*)> : public function_traits<R(C&)>
  {
    using class_type     = std::decay_t<C>;
    using tuple_type_raw = std::tuple<>;
  };
}  // namespace bifrost::meta

#endif /* BIFROST_META_FUNCTION_TRAIT_HPP */