#ifndef BIFROST_META_RUNTIME_HPP
#define BIFROST_META_RUNTIME_HPP

#define BIFROST_META_USE_FREELIST 0

#include "bifrost/data_structures/bifrost_array.hpp"
#include "bifrost/data_structures/bifrost_hash_table.hpp"
#if BIFROST_META_USE_FREELIST
#include "bifrost/memory/bifrost_freelist_allocator.hpp"
#else
#include "bifrost/memory/bifrost_linear_allocator.hpp"
#endif
#include "bifrost/memory/bifrost_proxy_allocator.hpp"

#include "bifrost_meta_variant.hpp"

#include <string_view> /* string_view */

namespace bifrost::meta
{
  class BaseClassMetaInfo;

#if BIFROST_META_USE_FREELIST
  using RttiAllocatorBackingType = FreeListAllocator;
  using RttiAllocatorType        = ProxyAllocator;
#else
  using RttiAllocatorBackingType = LinearAllocator;
  using RttiAllocatorType        = NoFreeAllocator;
#endif

  RttiAllocatorBackingType&                        gRttiMemoryBacking();
  RttiAllocatorType&                               gRttiMemory();
  HashTable<std::string_view, BaseClassMetaInfo*>& gRegistry();

  class BaseCtorMetaInfo
  {
    friend class BaseClassMetaInfo;

   protected:
    Array<BaseClassMetaInfo*> m_Parameters;

   public:
    const Array<BaseClassMetaInfo*>& parameters() const { return m_Parameters; }

    virtual ~BaseCtorMetaInfo() = default;

   protected:
    BaseCtorMetaInfo();

    virtual bool        isCompatible(const MetaVariant* arguments)                            = 0;
    virtual MetaVariant instantiateImpl(IMemoryManager& memory, const MetaVariant* arguments) = 0;
  };

  class BaseMetaInfo
  {
   protected:
    std::string_view m_Name;

   public:
    explicit BaseMetaInfo(std::string_view name);

    [[nodiscard]] std::string_view name() const { return m_Name; }

    virtual ~BaseMetaInfo() = default;
  };

  class BasePropertyMetaInfo : public BaseMetaInfo
  {
   private:
    BaseClassMetaInfo* m_Type;        //!< Type info for this property.
    bool               m_IsProperty;  //!< Since this base class is shared by both Class::m_Members and Class::getter / Class::setter pairs it is useful to know which type it is.

   protected:
    BasePropertyMetaInfo(std::string_view name, BaseClassMetaInfo* type, bool is_property);

   public:
    [[nodiscard]] BaseClassMetaInfo* type() const { return m_Type; }
    [[nodiscard]] bool               isProperty() const { return m_IsProperty; }
    virtual MetaVariant              get(const MetaVariant& self)                           = 0;
    virtual bool                     set(const MetaVariant& self, const MetaVariant& value) = 0;
  };

  class BaseMethodMetaInfo : public BaseMetaInfo
  {
   protected:
    Array<BaseClassMetaInfo*> m_Parameters;
    BaseClassMetaInfo*        m_ReturnType;

   public:
    [[nodiscard]] const Array<BaseClassMetaInfo*>& parameters() const { return m_Parameters; }
    [[nodiscard]] BaseClassMetaInfo*               returnType() const { return m_ReturnType; }

    template<typename... Args>
    MetaVariant invoke(Args&&... args)
    {
      if (sizeof...(Args) != m_Parameters.size())
      {
        throw "Arity Mismatch";
      }

      const MetaVariant variant_args[] = {makeVariant(args)...};
      return invokeImpl(variant_args);
    }

   protected:
    BaseMethodMetaInfo(std::string_view name, std::size_t arity, BaseClassMetaInfo* return_type);

   private:
    virtual MetaVariant invokeImpl(const MetaVariant* arguments) const = 0;
  };

  using BaseClassMetaInfoPtr = BaseClassMetaInfo*;

  // TODO(Shareef): Make this class more const correct.
  class BaseClassMetaInfo : public BaseMetaInfo
  {
    template<typename T>
    friend struct TypeInfo;

   protected:
    inline static constexpr std::uint8_t k_IsEnumBit  = bfBit(0);
    inline static constexpr std::uint8_t k_IsArrayBit = bfBit(1);
    inline static constexpr std::uint8_t k_IsMapBit   = bfBit(2);

   protected:
    Array<BaseClassMetaInfo*>    m_BaseClasses;
    Array<BaseCtorMetaInfo*>     m_Ctors;
    Array<BasePropertyMetaInfo*> m_Properties;
    Array<BaseMethodMetaInfo*>   m_Methods;
    std::size_t                  m_Size;
    std::size_t                  m_Alignment;
    std::uint8_t                 m_Flags;

   protected:
    BaseClassMetaInfo(std::string_view name, std::size_t size, std::size_t alignment);

   public:
    [[nodiscard]] std::size_t           size() const { return m_Size; }
    [[nodiscard]] std::size_t           alignment() const { return m_Alignment; }
    [[nodiscard]] bool                  isEnum() const { return m_Flags & k_IsEnumBit; }
    [[nodiscard]] bool                  isArray() const { return m_Flags & k_IsArrayBit; }
    [[nodiscard]] bool                  isMap() const { return m_Flags & k_IsMapBit; }
    const Array<BaseClassMetaInfo*>&    baseClasses() const { return m_BaseClasses; }
    const Array<BaseCtorMetaInfo*>&     ctors() const { return m_Ctors; }
    const Array<BasePropertyMetaInfo*>& properties() const { return m_Properties; }
    const Array<BaseMethodMetaInfo*>&   methods() const { return m_Methods; }

    template<typename TEnum>
    std::enable_if_t<std::is_enum_v<TEnum>, StringRange> enumToString(TEnum enum_value)
    {
      return enumToString(std::uint64_t(enum_value));
    }

    StringRange enumToString(std::uint64_t enum_value) const
    {
      for (BasePropertyMetaInfo* const property : properties())
      {
        const auto v = meta::variantToCompatibleT<std::uint64_t>(property->get(nullptr));

        if (v == enum_value)
        {
          return {
           property->name().data(),
           property->name().length(),
          };
        }
      }

      return {};
    }

    MetaVariant instantiate(IMemoryManager& memory);

    template<typename... Args>
    MetaVariant instantiate(IMemoryManager& memory, Args&&... args)
    {
      const MetaVariant variant_args[] = {makeVariant(args)...};

      return instantiateImpl(memory, variant_args, sizeof...(Args));
    }

    BasePropertyMetaInfo*        findProperty(std::string_view name) const;
    BaseMethodMetaInfo*          findMethod(std::string_view name) const;
    virtual BaseClassMetaInfoPtr keyType() const { return nullptr; }
    virtual BaseClassMetaInfoPtr valueType() const { return nullptr; }
    virtual std::size_t          numElements(const MetaVariant& self) const { return 0u; }
    virtual MetaVariant          elementAt(const MetaVariant& self, std::size_t index) const { return {}; }
    virtual MetaVariant          elementAt(const MetaVariant& self, const MetaVariant& key) const { return {}; }
    virtual bool                 setElementAt(const MetaVariant& self, std::size_t index, const MetaVariant& value) const { return false; }
    virtual bool                 setElementAt(const MetaVariant& self, const MetaVariant& key, const MetaVariant& value) const { return false; }

   private:
    MetaVariant instantiateImpl(IMemoryManager& memory, const MetaVariant* args, std::size_t num_args);
  };

  using BaseUnionMetaInfo  = BaseClassMetaInfo;
  using BaseStructMetaInfo = BaseClassMetaInfo;

  BaseClassMetaInfo* TypeInfoFromName(std::string_view name);
}  // namespace bifrost::meta

#endif /* BIFROST_META_RUNTIME_HPP */
