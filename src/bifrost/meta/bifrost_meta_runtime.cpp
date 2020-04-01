#include "bifrost/meta/bifrost_meta_runtime.hpp"

#include "bifrost/meta/bifrost_meta_runtime_impl.hpp"

namespace
{
  char s_Storage[16384 * 2];
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

  BasePropertyMetaInfo::BasePropertyMetaInfo(std::string_view name, BaseClassMetaInfo* type, bool is_property) :
    BaseMetaInfo(name),
    m_Type{type},
    m_IsProperty{is_property}
  {
  }

  BaseMethodMetaInfo::BaseMethodMetaInfo(std::string_view name, std::size_t arity, BaseClassMetaInfo* return_type) :
    BaseMetaInfo(name),
    m_Parameters{gRttiMemory()},
    m_ReturnType{return_type}
  {
    m_Parameters.resize(arity);
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
    m_IsMap{false},
    m_IsEnum{false}
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

  std::uint64_t BaseClassMetaInfo::enumValueMask() const
  {
    std::uint64_t mask;

    switch (size())
    {
      case 1:
        mask = 0xFF;
        break;
      case 2:
        mask = 0xFFFF;
        break;
      case 4:
        mask = 0xFFFFFFFF;
        break;
      case 8:
        mask = 0xFFFFFFFFFFFFFFFF;
        break;
      default:
        mask = 0x0;
        break;
    }

    return mask;
  }

  std::uint64_t BaseClassMetaInfo::enumValueRead(std::uint64_t& enum_object) const
  {
    switch (size())
    {
      case 1: return *reinterpret_cast<std::uint8_t*>(&enum_object);
      case 2: return *reinterpret_cast<std::uint16_t*>(&enum_object);
      case 4: return *reinterpret_cast<std::uint32_t*>(&enum_object);
      case 8: return *reinterpret_cast<std::uint64_t*>(&enum_object);
      default: return 0;
    }
  }

  void BaseClassMetaInfo::enumValueWrite(std::uint64_t& enum_object, std::uint64_t new_value) const
  {
    switch (size())
    {
      case 1:
        *reinterpret_cast<std::uint8_t*>(&enum_object) = std::uint8_t(new_value);
        break;
      case 2:
        *reinterpret_cast<std::uint16_t*>(&enum_object) = std::uint16_t(new_value);
        break;
      case 4:
        *reinterpret_cast<std::uint32_t*>(&enum_object) = std::uint32_t(new_value);
        break;
      case 8:
        *reinterpret_cast<std::uint64_t*>(&enum_object) = std::uint64_t(new_value);
        break;
      default:
        break;
    }
  }

  BaseClassMetaInfo* TypeInfoFromName(std::string_view name)
  {
    BaseClassMetaInfo** clz_info = gRegistry().at(name);
    return clz_info ? *clz_info : nullptr;
  }
}  // namespace bifrost::meta
