#include "bifrost/meta/bifrost_meta_runtime.hpp"

#include "bifrost/bifrost_std.h" /* bfInvalidDefaultCase */
#include "bifrost/core/bifrost_base_object.hpp"

namespace
{
  char s_Storage[32768];
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

  BaseClassMetaInfo* TypeInfoFromName(std::string_view name)
  {
    BaseClassMetaInfo** clz_info = gRegistry().at(name);

    return clz_info ? *clz_info : nullptr;
  }

  MetaVariant detail::make(void* ptr, BaseClassMetaInfo* class_info)
  {
    // ReSharper disable CppSomeObjectMembersMightNotBeInitialized
    MetaObject ret{class_info, {nullptr}};

    if (class_info->isEnum())
    {
      // ret.enum_value = *static_cast<std::uint64_t*>(ptr);
      ret.enum_value = 0x0; // Clear it out.
      std::memcpy(&ret.enum_value, ptr, class_info->size());
    }
    else
    {
      ret.object_ref = ptr;
    }

    return ret;
    // ReSharper restore CppSomeObjectMembersMightNotBeInitialized
  }

  void* detail::doBaseObjStuff(IBaseObject* base_obj, BaseClassMetaInfo* type_info)
  {
    return base_obj->type() == type_info ? base_obj : nullptr;
  }

  bool detail::isEnum(const MetaObject& obj)
  {
    return obj.type_info->isEnum();
  }
}  // namespace bifrost::meta


