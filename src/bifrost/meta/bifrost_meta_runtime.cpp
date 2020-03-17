#include "bifrost/meta/bifrost_meta_runtime.hpp"

#include "bifrost/meta/bifrost_meta_runtime_impl.hpp"

namespace
{
  char s_Storage[16384];
}  // namespace

namespace bifrost::meta
{
  RttiAllocatorBackingType& gRttiMemoryBacking()
  {
    static RttiAllocatorBackingType s_BackingAllocator{s_Storage, sizeof(s_Storage)};
    return s_BackingAllocator;
  }

  RttiAllocatorType& gRttiMemory()
  {
    static RttiAllocatorType s_RttiMemory{gRttiMemoryBacking()};
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
    m_Properties{gRttiMemory()},
    m_Methods{gRttiMemory()},
    m_Size{size},
    m_Alignment{alignment},
    m_IsArray{false},
    m_IsMap{false}
  {
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

  BaseClassMetaInfo* TypeInfoFromName(std::string_view name)
  {
    BaseClassMetaInfo** clz_info = gRegistry().at(name);
    return clz_info ? *clz_info : nullptr;
  }
}  // namespace bifrost::meta
