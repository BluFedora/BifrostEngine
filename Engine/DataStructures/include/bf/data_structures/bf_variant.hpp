/******************************************************************************/
/*!
* @file   bf_variant.hpp
* @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
* @brief
*   C++ safe tagged union type that will correctly call constructors and
*   destructors of the held type and the API will check to make sure you
*   don't access the incorrect type.
*
* @version 0.0.1
* @date    2017-01-00
*
* @copyright Copyright (c) 2018-2021
*/
/******************************************************************************/
#ifndef BF_VARIANT_HPP
#define BF_VARIANT_HPP

//
// NOTE(SR):
//   This should be profiled per application / compiler
//   This turns on an implementation of `Variant` in which the
//   polymorphic behavior is from static function pointer tables
//   rather than templated recursion search for matching type id.
//
//   Depending on how well the compiler can optimize each approach
//   and the number of types in the `Variant` the performance
//   profile changes.
//
#ifndef BF_VARIANT_USE_TABLE_BASED_HELPERS
#define BF_VARIANT_USE_TABLE_BASED_HELPERS 1
#endif

#ifndef BF_VARIANT_USE_EXCEPTIONS
#define BF_VARIANT_USE_EXCEPTIONS (__cpp_exceptions == 199711)
#endif

#include "bifrost_container_tuple.hpp" /* get_index_of_element_from_tuple_by_type_impl */

#include <type_traits> /* aligned_storage */

#if BF_VARIANT_USE_EXCEPTIONS
#include <typeinfo> /* bad_cast */
#define BF_VARIANT_CONDITIONAL_NO_EXCEPT
#else
#include <cassert>
#define BF_VARIANT_CONDITIONAL_NO_EXCEPT noexcept
#endif

namespace bf
{
  namespace detail
  {
    template<size_t arg1, size_t... others>
    struct static_max;

    template<size_t arg>
    struct static_max<arg>
    {
      static constexpr size_t value = arg;
    };

    template<size_t arg1, size_t arg2, size_t... others>
    struct static_max<arg1, arg2, others...>
    {
      static constexpr size_t value = arg1 >= arg2 ? static_max<arg1, others...>::value : static_max<arg2, others...>::value;
    };
  }  // namespace detail

#if !BF_VARIANT_USE_TABLE_BASED_HELPERS
  template<std::size_t N, typename... Ts>
  struct variant_helper;

  template<std::size_t N, typename F, typename... Ts>
  struct variant_helper<N, F, Ts...>
  {
    inline static void destroy(size_t id, void* data)
    {
      if (id != 0)
      {
        if (id == N)
          reinterpret_cast<F*>(data)->~F();
        else
          variant_helper<N + 1, Ts...>::destroy(id, data);
      }
    }

    inline static void move(size_t old_t, void* old_v, void* new_v)
    {
      if (old_t != 0)
      {
        if (old_t == N)
          new (new_v) F(std::move(*reinterpret_cast<F*>(old_v)));
        else
          variant_helper<N + 1, Ts...>::move(old_t, old_v, new_v);
      }
    }

    inline static void copy(size_t old_t, const void* old_v, void* new_v)
    {
      if (old_t != 0)
      {
        if (old_t == N)
          new (new_v) F(*reinterpret_cast<const F*>(old_v));
        else
          variant_helper<N + 1, Ts...>::copy(old_t, old_v, new_v);
      }
    }
  };

  template<std::size_t N>
  struct variant_helper<N>
  {
    inline static void destroy(size_t id, void* data)
    {
      (void)id;
      (void)data;
    }

    inline static void move(size_t old_t, void* old_v, void* new_v)
    {
      (void)old_t;
      (void)old_v;
      (void)new_v;
    }

    inline static void copy(size_t old_t, const void* old_v, void* new_v)
    {
      (void)old_t;
      (void)old_v;
      (void)new_v;
    }
  };
#endif
  template<typename... Ts>
  struct contains
  {
    static constexpr bool value = false;
  };

  template<typename F, typename S, typename... T>
  struct contains<F, S, T...>
  {
    static constexpr bool value = std::is_same<F, S>::value || contains<F, T...>::value;
  };

  template<typename... Ts>
  class Variant
  {
   public:
    template<typename T>
    static constexpr std::size_t type_of()
    {
      static_assert(contains<T, void, Ts...>::value, "Type T is not able to be used in this variant");
      return detail::get_index_of_element_from_tuple_by_type_impl<T, 0, void, Ts...>::value;
    }

    template<std::size_t I>
    using type_at = std::tuple_element_t<I, std::tuple<Ts...>>;

    constexpr static size_t invalid_type()
    {
      return type_of<void>();
    }

   protected:
    static constexpr std::size_t data_size  = detail::static_max<sizeof(Ts)...>::value;
    static constexpr std::size_t data_align = detail::static_max<alignof(Ts)...>::value;

    using self_t = Variant<Ts...>;
    using data_t = typename std::aligned_storage<data_size, data_align>::type;
#if BF_VARIANT_USE_TABLE_BASED_HELPERS
    using deleter_t = void (*)(void*);
    using mover_t   = void (*)(void*, void*);
    using copier_t  = void (*)(const void*, void*);

    template<typename T>
    static void delete_t(void* object_data)
    {
      static_cast<T*>(object_data)->~T();
    }

    static void delete_void(void* object_data)
    {
      // NoOp
      (void)object_data;
    }

    template<typename T>
    static void move_t(void* old_value, void* new_value)
    {
      new (new_value) T(std::move(*reinterpret_cast<T*>(old_value)));
    }

    static void move_void(void* old_value, void* new_value)
    {
      // NoOp
      (void)old_value;
      (void)new_value;
    }

    template<typename T>
    static void copy_t(const void* old_value, void* new_value)
    {
      new (new_value) T(*reinterpret_cast<const T*>(old_value));
    }

    static void copy_void(const void* old_value, void* new_value)
    {
      // NoOp
      (void)old_value;
      (void)new_value;
    }
#else
    using helper_t = variant_helper<1, Ts...>;
#endif

   protected:
    data_t      m_Data   = data_t();
    std::size_t m_TypeID = invalid_type();

   public:
    Variant() = default;

    Variant(std::nullptr_t) noexcept :
      Variant()
    {
    }

    template<typename T>
    Variant(const T& data_in) noexcept :
      m_Data(),
      m_TypeID(invalid_type())
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");

      try
      {
        set<T>(data_in);
      }
      catch (...)
      {
      }
    }

    Variant(const self_t& rhs) :
      m_Data(),
      m_TypeID(rhs.m_TypeID)
    {
      copy(&rhs.m_Data);
    }

    Variant(self_t&& rhs) noexcept :
      m_Data(),
      m_TypeID(rhs.m_TypeID)
    {
      move(&rhs.m_Data);
    }

    template<typename T>
    self_t& operator=(const self_t& value)
    {
      set<T>(value);
      return *this;
    }

    self_t& operator=(const self_t& old)
    {
      if (this != &old)
      {
        destroy();
        m_TypeID = old.type();
        copy(&old.m_Data);
      }

      return *this;
    }

    self_t& operator=(self_t&& old) noexcept
    {
      if (this != &old)
      {
        destroy();
        m_TypeID = old.type();
        move(&old.m_Data);
      }

      return *this;
    }

    std::size_t type() const noexcept
    {
      return m_TypeID;
    }

    template<typename T>
    static constexpr bool canContainT()
    {
      return contains<std::decay_t<T>, Ts...>::value;
    }

    template<typename T>
    bool is() const noexcept
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");
      return (m_TypeID == type_of<T>());
    }

    bool valid() const noexcept
    {
      return (m_TypeID != invalid_type());
    }

    template<typename T, typename... Args>
    T& set(Args&&... args)
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");

      destroy();
      new (&m_Data) T(std::forward<Args>(args)...);
      m_TypeID = type_of<T>();

      return as<T>();
    }

    template<typename T>
    operator T&() noexcept
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");

      return this->get<T>();
    }

    template<typename T>
    operator const T&() const noexcept
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");

      return this->get<T>();
    }

    template<typename T>
    T& as() BF_VARIANT_CONDITIONAL_NO_EXCEPT
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");

      return this->get<T>();
    }

    template<typename T>
    [[nodiscard]] const T& as() const BF_VARIANT_CONDITIONAL_NO_EXCEPT
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");

      return this->get<T>();
    }

    template<typename T>
    T& get() BF_VARIANT_CONDITIONAL_NO_EXCEPT
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");
      checkType<T>();

      return *reinterpret_cast<T*>(&m_Data);
    }

    template<typename T>
    const T& get() const BF_VARIANT_CONDITIONAL_NO_EXCEPT
    {
      static_assert(canContainT<T>(), "Type T is not able to be used in this variant");

      checkType<T>();

      return *reinterpret_cast<const T*>(&m_Data);
    }

    void destroy()
    {
#if BF_VARIANT_USE_TABLE_BASED_HELPERS
      static constexpr deleter_t s_DeleteTable[] = {delete_void, delete_t<Ts>...};
      s_DeleteTable[type()](&m_Data);
      m_TypeID = type_of<void>();
#else
      helper_t::destroy(type(), &m_Data);
#endif
    }

    ~Variant()
    {
      destroy();
    }

   private:
    template<typename T>
    void checkType() const BF_VARIANT_CONDITIONAL_NO_EXCEPT
    {
      if (!this->is<T>())
      {
#if BF_VARIANT_USE_EXCEPTIONS
        throw std::bad_cast();
#else
        assert(!"Bad cast");
#endif
      }
    }

    void move(void* old_data)
    {
#if BF_VARIANT_USE_TABLE_BASED_HELPERS
      static constexpr mover_t s_MoveTable[] = {move_void, move_t<Ts>...};
      s_MoveTable[type()](old_data, &m_Data);
#else
      helper_t::move(type(), old_data, &m_Data);
#endif
    }

    void copy(const void* old_data)
    {
#if BF_VARIANT_USE_TABLE_BASED_HELPERS
      static constexpr copier_t s_CopyTable[] = {copy_void, copy_t<Ts>...};
      s_CopyTable[type()](old_data, &m_Data);
#else
      helper_t::copy(type(), old_data, &m_Data);
#endif
    }
  };

  template<typename T>
  using Optional = Variant<T>;

  struct BadVisitException final
  {};

  namespace detail
  {
    template<typename TVisitor, typename... Ts>
    struct is_callable_by_type_list;

    template<typename TVisitor, typename T>
    struct is_callable_by_type_list<TVisitor, T>
    {
      static constexpr bool value = std::is_invocable_v<TVisitor, T>;
    };

    template<typename TVisitor, typename T, typename... Ts>
    struct is_callable_by_type_list<TVisitor, T, Ts...>
    {
      static constexpr bool value = is_callable_by_type_list<TVisitor, T>::value || is_callable_by_type_list<TVisitor, Ts...>::value;
    };

    template<typename FVisitor, typename TVariant, typename T, typename... Ts>
    static auto visit_helper(FVisitor&& visitor, TVariant& variant) -> decltype(visitor(variant.template as<T>()))
    {
      // using ReturnType = decltype(visitor(variant.template as<T>()));

      if constexpr (std::is_invocable_v<FVisitor, T> || std::is_invocable_v<FVisitor, T&> || std::is_invocable_v<FVisitor, const T&>)
      {
        if (variant.template is<T>())
        {
          return visitor(variant.template as<T>());
        }
      }

      if constexpr (sizeof...(Ts) > 0)
      {
        return visit_helper<FVisitor, TVariant, Ts...>(std::forward<FVisitor>(visitor), variant);
      }
      else
      {
        throw BadVisitException{};
      }
    }
  }  // namespace detail

  template<typename FVisitor, typename... Ts>
  static void visit(FVisitor&& visitor, Variant<Ts...>& variant)
  {
    detail::visit_helper<FVisitor, Variant<Ts...>, Ts...>(std::forward<FVisitor>(visitor), variant);
  }

  template<typename FVisitor, typename... Ts>
  static decltype(auto) visit_all(FVisitor&& visitor, const Variant<Ts...>& variant)
  {
    static_assert(detail::is_callable_by_type_list<FVisitor, Ts...>::value, "The visitor does not handle all types contained in the Variant.");

    return detail::visit_helper<FVisitor, const Variant<Ts...>, Ts...>(std::forward<FVisitor>(visitor), variant);
  }

  template<typename FVisitor, typename... Ts>
  static decltype(auto) visit_all(FVisitor&& visitor, Variant<Ts...>& variant)
  {
    static_assert(detail::is_callable_by_type_list<FVisitor, Ts...>::value, "The visitor does not handle all types contained in the Variant.");

    return detail::visit_helper<FVisitor, Variant<Ts...>, Ts...>(std::forward<FVisitor>(visitor), variant);
  }
}  // namespace bf

#endif /* BF_VARIANT_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2018-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
