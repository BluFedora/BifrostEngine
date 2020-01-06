#ifndef TIDE_CONTAINER_TUPLE_HPP
#define TIDE_CONTAINER_TUPLE_HPP

#include <tuple> /* size_t, tuple */

namespace bifrost
{
  namespace detail
  {
    template<class T, std::size_t N, class... Args>
    struct get_index_of_element_from_tuple_by_type_impl;

    template<class T, std::size_t N, class... Args>
    struct get_index_of_element_from_tuple_by_type_impl<T, N, T, Args...>
    {
      static constexpr auto value = N;
    };

    template<class T, std::size_t N, class U, class... Args>
    struct get_index_of_element_from_tuple_by_type_impl<T, N, U, Args...>
    {
      static constexpr auto value = get_index_of_element_from_tuple_by_type_impl<T, N + 1, Args...>::value;
    };

    template<class... Trest>
    struct unique_types; /* undefined */

    template<class T1, class T2, class... Trest>
    struct unique_types<T1, T2, Trest...> : private unique_types<T1, T2>
      , unique_types<T1, Trest...>
      , unique_types<T2, Trest...>
    {
    };

    template<class T1, class T2>
    struct unique_types<T1, T2>
    {
      static_assert(!std::is_same<T1, T2>::value, "All types must be unique in this arg list.");
    };

    /*!
     *  @brief
     *    One element is guaranteed to be unique.
     */
    template<class T1>
    struct unique_types<T1>
    {
    };
  }  // namespace detail

  /*!
   *  @brief
   *    Thin wrapper around a tuple of 'TContainer' of differing types.
   */
  template<template<typename...> class TContainer, typename... Args>
  class ContainerTuple : private detail::unique_types<Args...>
  {
   private:
    using ContainerTupleImpl = std::tuple<TContainer<Args>...>;

   private:
    ContainerTupleImpl m_Impl;

   public:
    inline ContainerTuple(void) noexcept :
      m_Impl()
    {
    }

    inline ContainerTupleImpl& raw() noexcept
    {
      return m_Impl;
    }

    inline const ContainerTupleImpl& raw() const noexcept
    {
      return m_Impl;
    }

    template<typename T>
    TContainer<T>& get() noexcept
    {
      return std::get<detail::get_index_of_element_from_tuple_by_type_impl<T, 0, Args...>::value>(raw());
    }

    template<typename T>
    const TContainer<T>& get() const noexcept
    {
      return std::get<detail::get_index_of_element_from_tuple_by_type_impl<T, 0, Args...>::value>(raw());
    }
  };
}  // namespace tide

#endif /* TIDE_CONTAINER_TUPLE_HPP */
