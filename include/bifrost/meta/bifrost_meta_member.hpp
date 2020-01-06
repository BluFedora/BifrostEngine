#ifndef BIFROST_META_MEMBER_HPP
#define BIFROST_META_MEMBER_HPP

#include <tuple>       /* tuple                 */
#include <type_traits> /* decay_t, is_pointer_v */

namespace bifrost::meta
{
  class BaseMember
  {
   private:
    const char* const m_Name;

   public:
    explicit BaseMember(const char* name) :
      m_Name(name)
    {
    }

    [[nodiscard]] const char* name() const { return m_Name; }
  };

  template<typename Class, typename Base>
  class ClassInfo final : public BaseMember
  {
   public:
    using type      = Class;
    using type_base = Base;
    using class_t   = Class;
    // using member_ptr                         = Class;
    static constexpr bool is_writable = true;
    using is_function                 = std::bool_constant<false>;
    static constexpr bool is_readable = false;
    static constexpr bool is_class    = true;
    static constexpr bool is_ctor     = false;
    static constexpr bool is_field    = false;
    static constexpr bool is_property = false;
    static constexpr bool is_pointer  = false;
    static constexpr bool is_enum     = false;

   private:
    const std::size_t m_Size;
    const std::size_t m_Alignment;

   public:
    explicit ClassInfo(const char* name) :
      BaseMember{name},
      m_Size{sizeof(Class)},
      m_Alignment{alignof(Class)}
    {
    }

    [[nodiscard]] std::size_t size() const { return m_Size; }
    [[nodiscard]] std::size_t alignment() const { return m_Alignment; }
  };

  template<typename... Args>
  class CtorInfo final
  {
   public:
    using type                        = std::tuple<Args...>;
    static constexpr bool is_writable = true;
    using is_function                 = std::bool_constant<false>;
    static constexpr bool is_readable = false;
    static constexpr bool is_class    = false;
    static constexpr bool is_ctor     = true;
    static constexpr bool is_field    = false;
    static constexpr bool is_property = false;
    static constexpr bool is_pointer  = false;
    static constexpr bool is_enum     = false;
  };

  template<typename Class, typename PropertyT, bool read_only>
  class RawMember final : public BaseMember
  {
   public:
    using type                        = std::decay_t<PropertyT>;
    using class_t                     = Class;
    using member_ptr                  = PropertyT class_t::*;
    using is_function                 = std::bool_constant<false>;
    static constexpr bool is_writable = !read_only;
    static constexpr bool is_readable = true;
    static constexpr bool is_class    = false;
    static constexpr bool is_ctor     = false;
    static constexpr bool is_field    = true;
    static constexpr bool is_property = false;
    static constexpr bool is_pointer  = std::is_pointer_v<PropertyT>;
    static constexpr bool is_enum     = false;
    static constexpr bool is_value    = false;

   private:
    member_ptr     m_Pointer;
    std::ptrdiff_t m_Offset;

   public:
    explicit RawMember(const char* name, member_ptr ptr, std::ptrdiff_t offset) :
      BaseMember{name},
      m_Pointer{ptr},
      m_Offset{offset}
    {
    }

    [[nodiscard]] std::ptrdiff_t offset() const { return m_Offset; }

    // NOTE(Shareef)
    //   This should always be true.
    //   The use of 'm_Pointer' is just so there
    //   are no warnings from resharper.
    [[nodiscard]] bool isReadOnly() const { return m_Pointer != nullptr; }

    [[nodiscard]] PropertyT& get(Class& obj) const
    {
      return obj.*m_Pointer;
    }

    [[nodiscard]] PropertyT& rGet(Class& obj) const
    {
      return obj.*m_Pointer;
    }

    void set(Class& obj, const type& value) const
    {
      if constexpr (!read_only)
      {
        (obj.*m_Pointer) = value;
      }
    }
  };

  template<typename Class, typename PropertyT>
  class RefMember final : public BaseMember
  {
   public:
    using type                        = std::decay_t<PropertyT>;
    using class_t                     = Class;
    using getter_t                    = const type& (class_t::*)() const;
    using setter_t                    = void (class_t::*)(const type&);
    static constexpr bool is_writable = true;
    using is_function                 = std::bool_constant<false>;
    static constexpr bool is_readable = true;
    static constexpr bool is_class    = false;
    static constexpr bool is_ctor     = false;
    static constexpr bool is_field    = false;
    static constexpr bool is_property = true;
    static constexpr bool is_pointer  = std::is_pointer_v<PropertyT>;
    static constexpr bool is_enum     = false;
    static constexpr bool is_value    = false;

   private:
    getter_t m_Getter;
    setter_t m_Setter;

   public:
    explicit RefMember(const char* name, getter_t getter, setter_t setter) :
      BaseMember{name},
      m_Getter{getter},
      m_Setter{setter}
    {
    }

    [[nodiscard]] bool             isReadOnly() const { return m_Setter == nullptr; }
    [[nodiscard]] const PropertyT& get(const Class& obj) const { return (obj.*m_Getter)(); }

    void set(Class& obj, const type& value) const
    {
      if (m_Setter)
      {
        (obj.*m_Setter)(value);
      }
    }
  };

  template<typename Class, typename PropertyT>
  class ValMember final : public BaseMember
  {
   public:
    using type                        = std::decay_t<PropertyT>;
    using class_t                     = Class;
    using getter_t                    = type (class_t::*)() const;
    using setter_t                    = void (class_t::*)(type);
    static constexpr bool is_writable = true;
    using is_function                 = std::bool_constant<false>;
    static constexpr bool is_readable = true;
    static constexpr bool is_class    = false;
    static constexpr bool is_ctor     = false;
    static constexpr bool is_field    = false;
    static constexpr bool is_property = true;
    static constexpr bool is_pointer  = std::is_pointer_v<PropertyT>;
    static constexpr bool is_enum     = false;
    static constexpr bool is_value    = true;

   private:
    getter_t m_Getter;
    setter_t m_Setter;

   public:
    explicit ValMember(const char* name, getter_t getter, setter_t setter) :
      BaseMember{name},
      m_Getter{getter},
      m_Setter{setter}
    {
    }

    [[nodiscard]] bool isReadOnly() const { return m_Setter == nullptr; }

    PropertyT get(const Class& obj) const
    {
      return (obj.*m_Getter)();
    }

    void set(Class& obj, const type& value) const
    {
      if (m_Setter)
      {
        (obj.*m_Setter)(value);
      }
    }
  };

  template<typename Class, typename R, typename... Args>
  class FnMember final : public BaseMember
  {
   public:
    using type        = R (Class::*)(Args...);
    using class_t     = Class;
    using is_function = std::bool_constant<true>;

    static constexpr bool is_writable = true;
    static constexpr bool is_readable = true;
    static constexpr bool is_class    = false;
    static constexpr bool is_ctor     = false;
    static constexpr bool is_field    = false;
    static constexpr bool is_property = false;
    static constexpr bool is_pointer  = false;
    static constexpr bool is_enum     = false;

   private:
    type m_Pointer;

   public:
    explicit FnMember(const char* name, type ptr) :
      BaseMember{name},
      m_Pointer{ptr}
    {
    }

    // NOTE(Shareef)
    //   This should always be true.
    //   The use of 'm_Pointer' is just so there
    //   are no warnings from resharper.
    [[nodiscard]] bool isReadOnly() const { return m_Pointer != nullptr; }
#if 0
    decltype(auto) call(Class& obj, Args&&... args) const
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (obj.*m_Pointer)(std::forward<Args>(args)...);
      }
      else
      {
        return (obj.*m_Pointer)(std::forward<Args>(args)...);
      }
    }
#endif
    decltype(auto) call(Class* obj, Args... args) const
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (obj->*m_Pointer)(std::forward<Args>(args)...);
      }
      else
      {
        return (obj->*m_Pointer)(std::forward<Args>(args)...);
      }
    }

    // This part of the concept is not available for functions.
    // void get(const Class& obj) const;
    // void set(Class& obj, const type& value) const;
  };

  template<typename Class, typename R, typename... Args>
  class FnCMember final : public BaseMember
  {
   public:
    using type        = R (Class::*)(Args...) const;
    using class_t     = Class;
    using is_function = std::bool_constant<true>;

    static constexpr bool is_writable = true;
    static constexpr bool is_readable = true;
    static constexpr bool is_class    = false;
    static constexpr bool is_ctor     = false;
    static constexpr bool is_field    = false;
    static constexpr bool is_property = false;
    static constexpr bool is_pointer  = false;
    static constexpr bool is_enum     = false;

   private:
    type m_Pointer;

   public:
    explicit FnCMember(const char* name, type ptr) :
      BaseMember{name},
      m_Pointer{ptr}
    {
    }

    // NOTE(Shareef)
    //   This should always be true.
    //   The use of 'm_Pointer' is just so there are no warnings from resharper.
    [[nodiscard]] bool isReadOnly() const { return m_Pointer != nullptr; }
#if 0

    decltype(auto) call(const Class& obj, Args&&... args) const
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (obj.*m_Pointer)(std::forward<Args>(args)...);
      }
      else
      {
        return (obj.*m_Pointer)(std::forward<Args>(args)...);
      }
    }
#endif

    decltype(auto) call(const Class* obj, Args... args) const
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (obj->*m_Pointer)(std::forward<Args>(args)...);
      }
      else
      {
        return (obj->*m_Pointer)(std::forward<Args>(args)...);
      }
    }

    // This part of the concept is not available for functions.
    // void get(const Class& obj) const;
    // void set(Class& obj, const type& value) const;
  };

  template<typename Clz, typename Base = void>
  decltype(auto) class_info(const char* name)
  {
    static_assert(
     std::is_base_of_v<Base, Clz> || std::is_same_v<Base, void>,
     "Class must inherit from the specified Base.");

    return ClassInfo<Clz, Base>(name);
  }

  template<typename... Args>
  decltype(auto) ctor()
  {
    return CtorInfo<Args...>();
  }

  template<bool is_readonly = false, typename Clz, typename T>
  decltype(auto) field(const char* name, T Clz::*ptr, std::ptrdiff_t offset = 0u)
  {
    return RawMember<Clz, T, is_readonly>(name, ptr, offset);
  }

  template<typename Clz, typename T>
  decltype(auto) field_readonly(const char* name, T Clz::*ptr, std::ptrdiff_t offset = 0u)
  {
    return field<true>(name, ptr, offset);
  }

  template<typename Clz, typename T>
  decltype(auto) property(const char* name, const T& (Clz::*g)() const, void (Clz::*s)(const T&) = nullptr)
  {
    return RefMember<Clz, T>(name, g, s);
  }

  template<typename Clz, typename T>
  decltype(auto) property(const char* name, T (Clz::*g)() const, void (Clz::*s)(T) = nullptr)
  {
    return ValMember<Clz, T>(name, g, s);
  }

  template<typename Clz, typename R, typename... Args>
  decltype(auto) function(const char* name, R (Clz::*fn)(Args...))
  {
    return FnMember<Clz, R, Args...>(name, fn);
  }

  template<typename Clz, typename R, typename... Args>
  decltype(auto) function(const char* name, R (Clz::*fn)(Args...) const)
  {
    return FnCMember<Clz, R, Args...>(name, fn);
  }

  class Meta final
  {
   public:
    template<typename T>
    static const auto& registerMembers();
  };

  // default for unregistered classes
  template<typename T>
  const auto& Meta::registerMembers()
  {
    static const std::tuple<> emptyTuple;
    return emptyTuple;
  }

  namespace detail
  {
    template<typename T, typename TupleType>
    class MetaHolder final
    {
     public:
      static TupleType members;
    };

    template<typename T, typename TupleType>
    TupleType MetaHolder<T, TupleType>::members = Meta::registerMembers<T>();
  }  // namespace detail

  template<typename... Args>
  auto members(Args&&... args)
  {
    return std::make_tuple(std::forward<Args>(args)...);
  }

  template<typename T>
  const auto& membersOf()
  {
    return Meta::registerMembers<T>();
  }

  template<typename MemberT>
  static constexpr bool is_class_v = std::decay_t<MemberT>::is_class;

  template<typename MemberT>
  static constexpr bool is_ctor_v = std::decay_t<MemberT>::is_ctor;

  template<typename MemberT>
  static constexpr bool is_field_v = std::decay_t<MemberT>::is_field;

  template<typename MemberT>
  static constexpr bool is_property_v = std::decay_t<MemberT>::is_property;

  template<typename MemberT>
  static constexpr bool is_function_v = std::decay_t<MemberT>::is_function::value;

  template<typename MemberType>
  using member_t = std::decay_t<MemberType>;

  template<typename MemberType>
  using member_type_t = typename member_t<MemberType>::type;
}  // namespace bifrost::meta

#define BIFROST_META_FRIEND friend class ::bifrost::meta::Meta

#define BIFROST_META_REGISTER(name) \
  template<>                        \
  inline const auto& ::bifrost::meta::Meta::registerMembers<name>()

#define BIFROST_META_BEGIN() \
  static auto member_ptrs = ::bifrost::meta::members(

#define BIFROST_META_MEMBERS(...) __VA_ARGS__);

#define BIFROST_META_END() \
  return member_ptrs;

#endif /* BIFROST_META_MEMBER_HPP */
