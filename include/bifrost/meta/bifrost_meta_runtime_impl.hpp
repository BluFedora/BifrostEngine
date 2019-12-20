#ifndef BIFROST_META_RUNTIME_IMPL_HPP
#define BIFROST_META_RUNTIME_IMPL_HPP

#include "bifrost_meta_function_traits.hpp"
#include "bifrost_meta_member.hpp"
#include "bifrost_meta_runtime.hpp"
#include "bifrost_meta_utils.hpp"

#include <array> /* array<T> */

namespace bifrost::meta
{
  class InvalidMethodCall
  {
  };

  template<typename Base, typename MemberConcept>
  class PropertyMetaInfoCRTP : public Base
  {
    using Class     = typename MemberConcept::class_t;
    using PropertyT = typename MemberConcept::type;

   private:
    const MemberConcept& m_Impl;

   public:
    template<typename... Args>
    PropertyMetaInfoCRTP(const MemberConcept& impl, Args&&... args) :
      Base(impl.name(), TypeInfo<PropertyT>(), std::forward<Args>(args)...),
      m_Impl{impl}
    {
    }

    Any get(Any& instance) override final
    {
      return &m_Impl.get(*instance.as<Class*>());
    }

    void set(Any& instance, const Any& value) override final
    {
      if constexpr (MemberConcept::is_writable)
      {
        m_Impl.set(instance.as<Class>(), value.as<PropertyT>());
      }
    }
  };

  template<typename MemberConcept>
  class PropertyMetaInfo final : public PropertyMetaInfoCRTP<BasePropertyMetaInfo, MemberConcept>
  {
    using Class     = typename MemberConcept::class_t;
    using PropertyT = typename MemberConcept::type;

   public:
    PropertyMetaInfo(const MemberConcept& impl) :
      PropertyMetaInfoCRTP<BasePropertyMetaInfo, MemberConcept>(impl)
    {
    }
  };

  template<typename MemberConcept>
  class MemberMetaInfo final : public PropertyMetaInfoCRTP<BaseMemberMetaInfo, MemberConcept>
  {
    using Class     = typename MemberConcept::class_t;
    using PropertyT = typename MemberConcept::type;

   public:
    MemberMetaInfo(const MemberConcept& impl) :
      PropertyMetaInfoCRTP<BaseMemberMetaInfo, MemberConcept>(impl, impl.offset())
    {
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
    MethodMetaInfo(const FunctionConcept& impl) :
      BaseMethodMetaInfo(impl.name(), FnTraits::arity, TypeInfo<ReturnType>()),
      m_Impl{impl}
    {
    }

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
    CtorMetaInfo() :
      BaseCtorMetaInfo()
    {
    }

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

      Class* obj = static_cast<Class*>(memory.alloc(sizeof(Class), alignof(Class)));
      construct_from_any(obj, args);
      return obj;
    }
  };

  template<typename Class>
  class ClassMetaInfo : public BaseClassMetaInfo
  {
   public:
    explicit ClassMetaInfo(std::string_view name) :
      BaseClassMetaInfo(name, sizeof(Class), alignof(Class))
    {
      gRegistry()[name] = this;

      for_each(meta::membersOf<Class>(), [this](const auto& member) {
        using MemberT = member_t<decltype(member)>;

        if constexpr (meta::is_class_v<MemberT>)
        {
          m_BaseClasses.push(TypeInfo<typename MemberT::type_base>());
        }

        if constexpr (meta::is_ctor_v<MemberT>)
        {
          m_Ctors.push(gRttiMemory().alloc_t<CtorMetaInfo<Class, MemberT>>());
        }

        if constexpr (meta::is_field_v<MemberT>)
        {
          m_Members.push(gRttiMemory().alloc_t<MemberMetaInfo<MemberT>>(member));
        }

        if constexpr (meta::is_property_v<MemberT>)
        {
          m_Properties.push(gRttiMemory().alloc_t<PropertyMetaInfo<MemberT>>(member));
        }

        if constexpr (meta::is_function_v<MemberT>)
        {
          m_Methods.push(gRttiMemory().alloc_t<MethodMetaInfo<MemberT>>(member));
        }
      });
    }
  };
}  // namespace bifrost::meta

#endif /* BIFROST_META_RUNTIME_IMPL_HPP */
