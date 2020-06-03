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

    MetaVariant get(const MetaVariant& self) override
    {
      if constexpr (std::is_enum_v<Class>)
      {
        Class const instance_as_enum = meta::variantToCompatibleT<Class>(self);
        return makeVariant(m_Impl.get(instance_as_enum));
      }
      else
      {
        Class* const instance_as_class = meta::variantToCompatibleT<Class*>(self);

        if (instance_as_class)
        {
          const auto& wtf = m_Impl.get(*instance_as_class);

          return makeVariant(wtf);
        }
        else
        {
          meta::variantToCompatibleT<Class*>(self);
        }
      }

      return {};
    }

    bool set(const MetaVariant& self, const MetaVariant& value) override
    {
      if constexpr (MemberConcept::is_writable)
      {
        if constexpr (std::is_enum_v<Class>)
        {
          const_cast<MetaVariant&>(self) = value;
        }
        else
        {
          Class* const instance_as_class = meta::variantToCompatibleT<Class*>(self);

          if (instance_as_class)
          {
            std::aligned_storage_t<sizeof(PropertyBaseT), alignof(PropertyBaseT)> value_storage;

            if (meta::variantToCompatibleT2<PropertyBaseT>(&value_storage, value))
            {
              m_Impl.set(*instance_as_class, std::move(*reinterpret_cast<PropertyBaseT*>(&value_storage)));
              reinterpret_cast<PropertyBaseT*>(&value_storage)->~PropertyBaseT();
              return true;
            }
          }
        }
      }

      return false;
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
    explicit MethodMetaInfo(const FunctionConcept& impl);

   private:
    MetaVariant invokeImpl(const MetaVariant* arguments) const override
    {
      using ArgsType = typename FnTraits::tuple_type;

      ArgsType args;

      bool is_compatible = true;

      for_constexpr<std::tuple_size_v<ArgsType>>([arguments, &args, &is_compatible](auto i) {
        using T = typename FnTraits::template argument<i.value>::type;

        if (is_compatible)
        {
          const auto& args_i = arguments[i.value];

          if (isVariantCompatible<T>(args_i))
          {
            std::get<i.value>(args) = variantToCompatibleT<T>(args_i);
          }
          else
          {
            is_compatible = false;
          }
        }
      });

      if (is_compatible)
      {
        const auto tuple_args = std::tuple_cat(std::make_tuple<const FunctionConcept*>(&m_Impl), args);

        if constexpr (std::is_same_v<ReturnType, void>)
        {
          meta::apply(&FunctionConcept::call, tuple_args);
        }
        else
        {
          return makeVariant(meta::apply(&FunctionConcept::call, tuple_args));
        }
      }

      return {};
    }
  };

  template<typename Class, typename CtorConcept>
  class CtorMetaInfo final : public BaseCtorMetaInfo
  {
    using ArgsTuple = typename CtorConcept::type;

   public:
    CtorMetaInfo();

   protected:
    bool isCompatible_OLD(const Any* arguments) override
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

    Any instantiateImpl_OLD(IMemoryManager& memory, const Any* arguments) override
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

    BaseClassMetaInfoPtr keyType() const override;
    BaseClassMetaInfoPtr valueType() const override;
    std::size_t          numElements(const MetaVariant& self) const override;
    MetaVariant          elementAt(const MetaVariant& self, std::size_t index) const override;
    MetaVariant          elementAt(const MetaVariant& self, const MetaVariant& key) const override;
    bool                 setElementAt(const MetaVariant& self, std::size_t index, const MetaVariant& value) const override;
    bool                 setElementAt(const MetaVariant& self, const MetaVariant& key, const MetaVariant& value) const override;
  };

  // TypeInfo<T>

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
  struct TypeInfo<const T&> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<volatile T&> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<const volatile T&> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<T&> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<T*&> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<const T*&> : public TypeInfo<std::decay_t<T>>
  {
  };

  template<typename T>
  struct TypeInfo<const volatile T*&> : public TypeInfo<std::decay_t<T>>
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
      //   Also note the name "___NoTypeInfo___" is not registered to the global map
      //   since this uses the base class's constructor.
      //
      static BaseClassMetaInfo    s_NullType = {"___NoTypeInfo___", 0, 0};
      static BaseClassMetaInfoPtr s_Info     = nullptr;

      if (s_Info == nullptr)
      {
        // TODO(Shareef): Investigate if this is strictly required...
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
              auto* const info = gRttiMemory().allocateT<ClassMetaInfo<T>>(member.name());
              s_Info           = info;
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

  // CtorMetaInfo<Class, CtorConcept>

  template<typename Class, typename CtorConcept>
  CtorMetaInfo<Class, CtorConcept>::CtorMetaInfo() :
    BaseCtorMetaInfo()
  {
    for_constexpr<std::tuple_size<ArgsTuple>::value>([this](auto i) {
      m_Parameters.push(TypeInfo<typename std::tuple_element<i.value, ArgsTuple>::type>::get());
    });
  }

  // ClassMetaInfo<Class>

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

  // ArrayClassMetaInfo<Class>

  template<typename Class>
  BaseClassMetaInfoPtr ArrayClassMetaInfo<Class>::keyType() const
  {
    return TypeInfo<std::size_t>::get();
  }

  template<typename Class>
  BaseClassMetaInfoPtr ArrayClassMetaInfo<Class>::valueType() const
  {
    return TypeInfo<Class>::get();
  }

  template<typename Class>
  std::size_t ArrayClassMetaInfo<Class>::numElements(const MetaVariant& self) const
  {
    Array<Class>* const arr = meta::variantToCompatibleT<Array<Class>*>(self);

    if (arr)
    {
      return arr->size();
    }

    return 0u;
  }

  template<typename Class>
  MetaVariant ArrayClassMetaInfo<Class>::elementAt(const MetaVariant& self, std::size_t index) const
  {
    Array<Class>* const arr = meta::variantToCompatibleT<Array<Class>*>(self);

    if (arr)
    {
      return makeVariant(arr->at(index));
    }

    return {};
  }

  template<typename Class>
  MetaVariant ArrayClassMetaInfo<Class>::elementAt(const MetaVariant& self, const MetaVariant& key) const
  {
    if (isVariantCompatible<std::size_t>(key))
    {
      return elementAt(self, variantToCompatibleT<std::size_t>(key));
    }

    return {};
  }

  template<typename Class>
  bool ArrayClassMetaInfo<Class>::setElementAt(const MetaVariant& self, std::size_t index, const MetaVariant& value) const
  {
    Array<Class>* const arr = meta::variantToCompatibleT<Array<Class>*>(self);

    if (arr)
    {
      arr->at(index) = variantToCompatibleT<Class>(value);
    }

    return false;
  }

  template<typename Class>
  bool ArrayClassMetaInfo<Class>::setElementAt(const MetaVariant& self, const MetaVariant& key, const MetaVariant& value) const
  {
    if (isVariantCompatible<std::size_t>(key))
    {
      return setElementAt(self, variantToCompatibleT<std::size_t>(key), value);
    }

    return false;
  }

  // Misc

  template<typename T>
  BaseClassMetaInfo* typeInfoGet()
  {
    return TypeInfo<T>::get();
  }
}  // namespace bifrost::meta

#endif /* BIFROST_META_RUNTIME_IMPL_HPP */
