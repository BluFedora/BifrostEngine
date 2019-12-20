#include "bifrost/meta/bifrost_meta_runtime.hpp"

#include "bifrost/memory/bifrost_linear_allocator.hpp"
#include "bifrost/meta/bifrost_meta_runtime_impl.hpp"

namespace
{
  char s_Storage[8192];
}  // namespace

namespace bifrost::meta
{
  NoFreeAllocator& gRttiMemory()
  {
    static LinearAllocator s_BackingAllocator{s_Storage, sizeof(s_Storage)};
    static NoFreeAllocator s_RttiMemory{s_BackingAllocator};

    return s_RttiMemory;
  }

  HashTable<std::string_view, BaseClassMetaInfo*>& gRegistry()
  {
    static HashTable<std::string_view, BaseClassMetaInfo*> s_Registry;
    return s_Registry;
  }

  BaseCtorMetaInfo::BaseCtorMetaInfo() :
    m_Parameters{gRttiMemory()}
  {
  }

  BaseMetaInfo::BaseMetaInfo(std::string_view name) :
    m_Name{name}
  {
  }

  BasePropertyMetaInfo::BasePropertyMetaInfo(const std::string_view name, BaseClassMetaInfo* type) :
    BaseMetaInfo(name),
    m_Type{type}
  {
  }

  BaseMemberMetaInfo::BaseMemberMetaInfo(const std::string_view name, BaseClassMetaInfo* type, std::ptrdiff_t offset) :
    BasePropertyMetaInfo(name, type),
    m_Offset{offset}
  {
  }

  BaseMethodMetaInfo::BaseMethodMetaInfo(std::string_view name, std::size_t arity, BaseClassMetaInfo* return_type) :
    BaseMetaInfo(name),
    m_Parameters{gRttiMemory()},
    m_ReturnType{return_type}
  {
    m_Parameters.resize(arity);
  }

  BaseEnumMetaInfo::BaseEnumMetaInfo(std::string_view name, std::size_t num_values) :
    BaseMetaInfo(name),
    m_Elements{gRttiMemory()}
  {
    m_Elements.resize(num_values);
  }

  BaseClassMetaInfo::BaseClassMetaInfo(std::string_view name, std::size_t size, std::size_t alignment) :
    BaseMetaInfo{name},
    m_BaseClasses{gRttiMemory()},
    m_Ctors{gRttiMemory()},
    m_Members{gRttiMemory()},
    m_Properties{gRttiMemory()},
    m_Methods{gRttiMemory()},
    m_Size{size},
    m_Alignment{alignment}
  {
  }

  BaseMemberMetaInfo* BaseClassMetaInfo::findMember(std::string_view name) const
  {
    for (auto& member : m_Members)
    {
      if (member->name() == name)
      {
        return member;
      }
    }

    return nullptr;
  }

  BasePropertyMetaInfo* BaseClassMetaInfo::findProperty(std::string_view name) const
  {
    for (auto& property : m_Properties)
    {
      if (property->name() == name)
      {
        return property;
      }
    }

    return nullptr;
  }

  BaseMethodMetaInfo* BaseClassMetaInfo::findMethod(std::string_view name) const
  {
    for (auto& method : m_Methods)
    {
      if (method->name() == name)
      {
        return method;
      }
    }

    return nullptr;
  }

  // clang-format off                                   
  template<> BaseClassMetaInfo* g_TypeInfo<std::byte>   = gRttiMemory().alloc_t<ClassMetaInfo<std::byte>>("std::byte");
  template<> BaseClassMetaInfo* g_TypeInfo<bool>        = gRttiMemory().alloc_t<ClassMetaInfo<bool>>("bool");
  template<> BaseClassMetaInfo* g_TypeInfo<char>        = gRttiMemory().alloc_t<ClassMetaInfo<char>>("char");
  template<> BaseClassMetaInfo* g_TypeInfo<int8_t>      = gRttiMemory().alloc_t<ClassMetaInfo<int8_t>>("int8_t");
  template<> BaseClassMetaInfo* g_TypeInfo<uint8_t>     = gRttiMemory().alloc_t<ClassMetaInfo<uint8_t>>("uint8_t");
  template<> BaseClassMetaInfo* g_TypeInfo<int16_t>     = gRttiMemory().alloc_t<ClassMetaInfo<int16_t>>("int16_t");
  template<> BaseClassMetaInfo* g_TypeInfo<uint16_t>    = gRttiMemory().alloc_t<ClassMetaInfo<uint16_t>>("uint16_t");
  template<> BaseClassMetaInfo* g_TypeInfo<int32_t>     = gRttiMemory().alloc_t<ClassMetaInfo<int32_t>>("int32_t");
  template<> BaseClassMetaInfo* g_TypeInfo<uint32_t>    = gRttiMemory().alloc_t<ClassMetaInfo<uint32_t>>("uint32_t");
  template<> BaseClassMetaInfo* g_TypeInfo<int64_t>     = gRttiMemory().alloc_t<ClassMetaInfo<int64_t>>("int64_t");
  template<> BaseClassMetaInfo* g_TypeInfo<uint64_t>    = gRttiMemory().alloc_t<ClassMetaInfo<uint64_t>>("uint64_t");
  template<> BaseClassMetaInfo* g_TypeInfo<float>       = gRttiMemory().alloc_t<ClassMetaInfo<float>>("float");
  template<> BaseClassMetaInfo* g_TypeInfo<double>      = gRttiMemory().alloc_t<ClassMetaInfo<double>>("double");
  template<> BaseClassMetaInfo* g_TypeInfo<long double> = gRttiMemory().alloc_t<ClassMetaInfo<long double>>("long double");
  template<> BaseClassMetaInfo* g_TypeInfo<void*>       = gRttiMemory().alloc_t<ClassMetaInfo<void*>>("void*");
  // clang-format on

  BaseClassMetaInfo* TypeInfoFromName(std::string_view name)
  {
    BaseClassMetaInfo** clz_info = gRegistry().at(name);
    return clz_info ? *clz_info : nullptr;
  }
}  // namespace bifrost::meta
