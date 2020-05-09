#ifndef BIFROST_META_RUNTIME_IMPL_HPP
#define BIFROST_META_RUNTIME_IMPL_HPP

#include "bifrost_meta_function_traits.hpp"
#include "bifrost_meta_member.hpp"
#include "bifrost_meta_runtime.hpp"
#include "bifrost_meta_utils.hpp"

#include <array>

namespace bifrost::meta
{
  class InvalidMethodCall
  {};

  template<typename MemberConcept>
  class PropertyMetaInfo : public BasePropertyMetaInfo
  {
    using Class         = typename MemberConcept::class_t;
    using PropertyT     = typename MemberConcept::type;
    using PropertyBaseT = typename MemberConcept::type_base;

   private:
    const MemberConcept& m_Impl;

   public:
    PropertyMetaInfo(const MemberConcept& impl);

    static constexpr bool isFieldImpl()
    {
      return std::is_lvalue_reference_v<std::invoke_result_t<decltype(&MemberConcept::get), const MemberConcept&, Class&>>;
    }

    Any get(const Any& instance) override final
    {
      Class& instance_as_class = *instance.as<Class*>();

      if constexpr (isFieldImpl())
      {
        return &m_Impl.get(instance_as_class);
      }
      else
      {
        return m_Impl.get(instance_as_class);
      }
    }

    void set(Any& instance, const Any& value) override final
    {
      if constexpr (MemberConcept::is_writable)
      {
        Class& instance_as_class = *instance.as<Class*>();

        if constexpr (isFieldImpl())
        {
          m_Impl.set(instance_as_class, *value.as<PropertyBaseT*>());
        }
        else
        {
          m_Impl.set(instance_as_class, value.as<PropertyBaseT>());
        }
      }
    }
  };

  template<typename FunctionConcept>
  class MethodMetaInfo final : public BaseMethodMetaInfo
  {
    using Class      = typename FunctionConcept::class_t;
    using PropertyT  = typename FunctionConcept::type;
    using FnTraits   = function_traits<PropertyT>;
    using ReturnType = typename FnTraits::return_type;

   private:
    const FunctionConcept& m_Impl;

   public:
    MethodMetaInfo(const FunctionConcept& impl);

    Any invokeImpl(const Any* arguments) override
    {
      std::array<Any, FnTraits::arity> args;

      bool is_compatible = true;

      for_constexpr<args.size()>([arguments, &args, &is_compatible](auto i) {
        if (is_compatible)
        {
          if (!arguments[i.value].template isSimilar<typename FnTraits::template argument<i.value>::type>())
          {
            is_compatible = false;
          }
          else
          {
            args[i.value] = arguments[i.value];
          }
        }
      });

      if (is_compatible)
      {
        return meta::apply(
         [this](auto&&... args) -> Any {
           if constexpr (std::is_same_v<ReturnType, void>)
           {
             m_Impl.call(args...);
             return {};
           }
           else
           {
             return m_Impl.call(args...);
           }
         },
         args);
      }

      return InvalidMethodCall{};
    }
  };

  template<typename Class, typename CtorConcept>
  class CtorMetaInfo final : public BaseCtorMetaInfo
  {
    using ArgsTuple = typename CtorConcept::type;

   public:
    CtorMetaInfo();

   protected:
    bool isCompatible(const Any* arguments) override
    {
      bool is_compatible = true;

      for_constexpr<std::tuple_size<ArgsTuple>::value>([arguments, &is_compatible](auto i) {
        if (is_compatible)
        {
          if (!arguments[i.value].template isSimilar<typename std::tuple_element<i.value, ArgsTuple>::type>())
          {
            is_compatible = false;
          }
        }
      });

      return is_compatible;
    }

    template<std::size_t I>
    decltype(auto) castAs(Any& value)
    {
      return value.castSimilar<typename std::tuple_element<I, ArgsTuple>::type>();
    }

    template<std::size_t N, size_t... Is>
    void construct_from_any_(Class* obj, std::array<Any, N>&& tuple, std::index_sequence<Is...>)
    {
      new (obj) Class(castAs<Is>(std::get<Is>(tuple))...);
    }

    template<std::size_t N>
    void construct_from_any(Class* obj, std::array<Any, N>& tuple)
    {
      construct_from_any_(obj, std::forward<std::array<Any, N>>(tuple), std::make_index_sequence<N>{});
    }

    Any instantiateImpl(IMemoryManager& memory, const Any* arguments) override
    {
      std::array<Any, std::tuple_size<ArgsTuple>::value> args;

      for_constexpr<args.size()>([arguments, &args](auto i) {
        args[i.value] = arguments[i.value];
      });

      Class* obj = static_cast<Class*>(memory.allocate(sizeof(Class)));
      construct_from_any(obj, args);
      return obj;
    }
  };

  template<typename Class>
  class ClassMetaInfo : public BaseClassMetaInfo
  {
   public:
    explicit ClassMetaInfo(std::string_view name);

    void populateFields();
  };

  template<typename Class>
  class ArrayClassMetaInfo final : public BaseClassMetaInfo
  {
   private:
    ValMember<Array<Class>, std::size_t> m_Size;
    ValMember<Array<Class>, std::size_t> m_Capacity;

   public:
    explicit ArrayClassMetaInfo() :
      BaseClassMetaInfo("Array", sizeof(Class), alignof(Class)),
      m_Size{property("m_Size", &Array<Class>::size, &Array<Class>::resize)},
      m_Capacity{property("m_Capacity", &Array<Class>::capacity, &Array<Class>::reserve)}
    {
      gRegistry()[name()] = this;

      m_IsArray = true;

      m_Properties.push(gRttiMemory().allocateT<PropertyMetaInfo<decltype(m_Size)>>(m_Size));
      m_Properties.push(gRttiMemory().allocateT<PropertyMetaInfo<decltype(m_Capacity)>>(m_Capacity));
    }

    BaseClassMetaInfo* containedType() const override;

    std::size_t arraySize(Any& instance) override
    {
      Array<Class>* const arr = instance;
      return arr->size();
    }

    Any arrayGetElementAt(Any& instance, std::size_t index) override
    {
      Array<Class>* arr = instance;
      return (*arr)[index];
    }

    bool arraySetElementAt(Any& instance, std::size_t index, Any& value) override
    {
      Array<Class>* arr = instance;
      (*arr)[index]     = value;
      return true;
    }
  };

template<typename T>
struct TypeInfo;

#define TYPE_INFO_SPEC(T)                                                                                                        \
  template<>                                                                                                                     \
  struct TypeInfo<T>                                                                                                             \
  {                                                                                                                              \
    static BaseClassMetaInfo*& get()                                                                                             \
    {                                                                                                                            \
      static BaseClassMetaInfo* s_Info = gRttiMemory().allocateT<ClassMetaInfo<T>>(#T); /* NOLINT(bugprone-macro-parentheses) */ \
      return s_Info;                                                                                                             \
    }                                                                                                                            \
  }

  TYPE_INFO_SPEC(std::byte);
  TYPE_INFO_SPEC(char);
  TYPE_INFO_SPEC(std::int8_t);
  TYPE_INFO_SPEC(std::uint8_t);
  TYPE_INFO_SPEC(std::int16_t);
  TYPE_INFO_SPEC(std::uint16_t);
  TYPE_INFO_SPEC(std::int32_t);
  TYPE_INFO_SPEC(std::uint32_t);
  TYPE_INFO_SPEC(std::int64_t);
  TYPE_INFO_SPEC(std::uint64_t);
  TYPE_INFO_SPEC(float);
  TYPE_INFO_SPEC(double);
  TYPE_INFO_SPEC(long double);
  TYPE_INFO_SPEC(void*);
#undef TYPE_INFO_SPEC

  template<typename T>
  struct TypeInfo<const T> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<volatile T> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<const volatile T> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<T*> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<const T*> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<const volatile T*> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<Array<T>>
  {
    static BaseClassMetaInfo* get()
    {
      static BaseClassMetaInfo* s_Info = gRttiMemory().allocateT<ArrayClassMetaInfo<T>>();
      return s_Info;
    }
  };

template<typename T>
  struct TypeInfo
  {
    static BaseClassMetaInfo*& get()
    {
      //
      // NOTE(Shareef):
      //   Since this is a static variable for a templated function:
      //   Pointer Comparisons still work for type checking since each instanciation of this
      //   function gives a different address.
      //   Also the name "___NoTypeInfo___" is not registered to the global map
      //   since this uses the base class's constructor.
      //
      static BaseClassMetaInfo  s_NullType = {"___NoTypeInfo___", 0, 0};
      static BaseClassMetaInfo* s_Info     = nullptr;

      if (s_Info == nullptr)
      {
#define TYPE_INFO_SPEC(T) TypeInfo<T>::get()
        TYPE_INFO_SPEC(std::byte);
        TYPE_INFO_SPEC(char);
        TYPE_INFO_SPEC(std::int8_t);
        TYPE_INFO_SPEC(std::uint8_t);
        TYPE_INFO_SPEC(std::int16_t);
        TYPE_INFO_SPEC(std::uint16_t);
        TYPE_INFO_SPEC(std::int32_t);
        TYPE_INFO_SPEC(std::uint32_t);
        TYPE_INFO_SPEC(std::int64_t);
        TYPE_INFO_SPEC(std::uint64_t);
        TYPE_INFO_SPEC(float);
        TYPE_INFO_SPEC(double);
        TYPE_INFO_SPEC(long double);
        TYPE_INFO_SPEC(void*);
#undef TYPE_INFO_SPEC

        for_each(meta::membersOf<T>(), [](const auto& member) {
          if constexpr (meta::is_class_v<decltype(member)> || meta::is_enum_v<decltype(member)>)
          {
            if (!s_Info)
            {
              auto* info = gRttiMemory().allocateT<ClassMetaInfo<T>>(member.name());
              s_Info     = info;
              info->populateFields();
            }
          }
        });

        if (!s_Info)
        {
          s_Info = &s_NullType;
        }
      }

      return s_Info;
    }
  };

  template<typename MemberConcept>
  PropertyMetaInfo<MemberConcept>::PropertyMetaInfo(const MemberConcept& impl) :
    BasePropertyMetaInfo(impl.name(), TypeInfo<PropertyBaseT>::get(), !isFieldImpl()),
    m_Impl{impl}
  {
  }

  template<typename FunctionConcept>
  MethodMetaInfo<FunctionConcept>::MethodMetaInfo(const FunctionConcept& impl) :
    BaseMethodMetaInfo(impl.name(), FnTraits::arity, TypeInfo<ReturnType>::get()),
    m_Impl{impl}
  {
  }

  template<typename Class, typename CtorConcept>
  CtorMetaInfo<Class, CtorConcept>::CtorMetaInfo() :
    BaseCtorMetaInfo()
  {
    for_constexpr<std::tuple_size<ArgsTuple>::value>([this](auto i) {
      m_Parameters.push(TypeInfo<typename std::tuple_element<i.value, ArgsTuple>::type>::get());
    });
  }

  template<typename Class>
  ClassMetaInfo<Class>::ClassMetaInfo(std::string_view name) :
    BaseClassMetaInfo(name, sizeof(Class), alignof(Class))
  {
    gRegistry()[name] = this;
  }

  template<typename Class>
  void ClassMetaInfo<Class>::populateFields()
  {
    for_each(meta::membersOf<Class>(), [this](const auto& member) {
      using MemberT = member_t<decltype(member)>;

      if constexpr (meta::is_class_v<MemberT>)
      {
        using base_type = member_type_base<decltype(member)>;

        if constexpr (!std::is_same_v<base_type, void>)
        {
          m_BaseClasses.push(TypeInfo<base_type>::get());
        }
      }

      if constexpr (meta::is_enum_v<MemberT>)
      {
        m_IsEnum = true;
      }

      if constexpr (meta::is_ctor_v<MemberT>)
      {
        m_Ctors.push(gRttiMemory().allocateT<CtorMetaInfo<Class, MemberT>>());
      }

      if constexpr (meta::is_property_v<MemberT> || meta::is_field_v<MemberT>)
      {
        m_Properties.push(gRttiMemory().allocateT<PropertyMetaInfo<MemberT>>(member));
      }

      if constexpr (meta::is_function_v<MemberT>)
      {
        m_Methods.push(gRttiMemory().allocateT<MethodMetaInfo<MemberT>>(member));
      }
    });
  }

  template<typename Class>
  BaseClassMetaInfo* ArrayClassMetaInfo<Class>::containedType() const
  {
    return TypeInfo<Class>::get();
  }
}  // namespace bifrost::meta

#endif /* BIFROST_META_RUNTIME_IMPL_HPP */
