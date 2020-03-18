/*!
 * @file   bifrost_meta_utils.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Utilities for compile time template meta programming operations.
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_META_UTILS_HPP
#define BIFROST_META_UTILS_HPP

#include <tuple>       /* tuple_size, get                                         */
#include <type_traits> /* index_sequence, make_index_sequence, remove_reference_t */
#include <utility>     /* forward                                                 */

namespace bifrost::meta
{
  //
  // overloaded
  //

  template<class... Ts>
  struct overloaded : public Ts...
  {
    using Ts::operator()...;
  };
  template<class... Ts>
  overloaded(Ts...)->overloaded<Ts...>;

  //
  // for_each
  //

  template<typename Tuple, typename F, std::size_t... Indices>
  constexpr void for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>)
  {
    //using swallow = int[];
    //(void)swallow{1,
    //              (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...};

    (f(std::get<Indices>(tuple)), ...);
  }

  template<typename Tuple, typename F>
  constexpr void for_each(Tuple&& tuple, F&& f)
  {
    constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
    for_each_impl(
     std::forward<Tuple>(tuple),
     std::forward<F>(f),
     std::make_index_sequence<N>{});
  }

  //
  // for_each_template
  //

  namespace detail
  {
    //
    // C++20 comes with lambda templates so we wouldn't have to do this.
    // But alas we are stuck on C++17 for now.
    //
    template<typename T>
    struct type_holder
    {
      using held_type = T;

      type_holder() = default;
    };

    template<std::size_t, typename T, typename... Args>
    class for_each_template_impl
    {
     public:
      template<typename F>
      static void impl(F&& func)
      {
        func(type_holder<T>());
        for_each_template_impl<sizeof...(Args), Args...>::impl(func);
      }
    };

    template<typename T>
    class for_each_template_impl<1, T>
    {
     public:
      template<typename F>
      static void impl(F&& func)
      {
        func(type_holder<T>());
      }
    };

    template<std::size_t, typename T, typename... Args>
    class for_each_template_and_pointer_impl
    {
     public:
      template<typename F>
      static void impl(F&& func)
      {
        func(type_holder<T>());
        func(type_holder<T*>());
        for_each_template_and_pointer_impl<sizeof...(Args), Args...>::impl(func);
      }
    };

    template<typename T>
    class for_each_template_and_pointer_impl<1, T>
    {
     public:
      template<typename F>
      static void impl(F&& func)
      {
        func(type_holder<T>());
        func(type_holder<T*>());
      }
    };

    template<std::size_t N>
    struct num
    {
      static const constexpr auto value = N;
    };

    template<class F, std::size_t... Is>
    void for_constexpr_impl(F func, std::index_sequence<Is...>)
    {
      (func(detail::num<Is>{}), ...);
    }
  }  // namespace detail

#define bfForEachTemplateT(t) typename std::decay<decltype((t))>::type::held_type

  template<typename... Args, typename F>
  void for_each_template(F&& func)
  {
    detail::for_each_template_impl<sizeof...(Args), Args...>::impl(func);
  }

  // Same as 'for_each_template' but adds the T* version.
  template<typename... Args, typename F>
  void for_each_template_and_pointer(F&& func)
  {
    detail::for_each_template_and_pointer_impl<sizeof...(Args), Args...>::impl(func);
  }

  //
  // for_constexpr
  //

  // [https://nilsdeppe.com/posts/for-constexpr]
  template<std::size_t N, typename F>
  void for_constexpr(F func)
  {
    detail::for_constexpr_impl(func, std::make_index_sequence<N>());
  }
}  // namespace bifrost::meta

#endif /* BIFROST_META_UTILS_HPP */
