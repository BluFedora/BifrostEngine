/******************************************************************************/
/*!
* @file   bifrost_asset_handle.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*  Asset handle definitions.
*
*  Types of Assets:
*    > Shader Module
*    > Shader Program
*    > Texture
*    > Material
*    > Spritesheet Animations
*    > Audio Source
*    > Scene
*    > Font
*    > Script
*    > Models (Meshes)
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019
*/
/******************************************************************************/
#include "bifrost/asset_io/bifrost_asset_handle.hpp"

#include "bifrost/asset_io/bifrost_file.hpp"
#include "bifrost/asset_io/bifrost_json_serializer.hpp"
#include "bifrost/core/bifrost_engine.hpp" /* Engine */
#include "bifrost/utility/bifrost_json.hpp"

namespace bifrost
{
  static constexpr int s_MaxDigitsUInt64 = 20;

  void ISerializer::serialize(StringRange key, Vec2f& value)
  {
    pushObject(key);
    serialize("x", value.x);
    serialize("y", value.y);
    popObject();
  }

  void ISerializer::serialize(StringRange key, Vec3f& value)
  {
    pushObject(key);
    serialize("x", value.x);
    serialize("y", value.y);
    serialize("z", value.z);
    serialize("w", value.w);
    popObject();
  }

  void ISerializer::serialize(StringRange key, IBaseObject& value)
  {
    if (pushObject(key))
    {
      serialize(value);
      popObject();
    }
  }

  void ISerializer::serialize(IBaseObject& value)
  {
    meta::BaseClassMetaInfo* const type_info = value.type();

    // TODO: This should never be null but ok.
    if (type_info)
    {
      Any object = &value;
      serialize(object, type_info);
    }
  }

  void ISerializer::serialize(StringRange key, Any& value, meta::BaseClassMetaInfo* type_info)
  {
    bool is_primitive = false;

    meta::for_each_template_and_pointer_and_const<
     // std::byte,
     std::int8_t,
     std::uint8_t,
     std::int16_t,
     std::uint16_t,
     std::int32_t,
     std::uint32_t,
     std::int64_t,
     std::uint64_t,
     float,
     double,
     long double,
     Vec2f,
     Vec3f,
     String,
     BifrostUUID,
     BaseAssetHandle>([this, &key, &value, &is_primitive](auto t) {
      using T = bfForEachTemplateT(t);

      if (!is_primitive && value.is<T>())
      {
        if constexpr (std::is_pointer_v<T>)
        {
          if constexpr (std::is_const_v<std::remove_pointer_t<T>>)
          {
            auto& v = const_cast<std::remove_const_t<std::remove_pointer_t<T>>&>(*value.as<T>());
            serialize(key, v);
          }
          else
          {
            auto& v = *value.as<T>();
            serialize(key, v);
          }
        }
        else
        {
          T value_copy = value.as<T>();

          serialize(key, value_copy);
          value.assign<T>(value_copy);
        }

        is_primitive = true;
      }
    });

    if (!is_primitive)
    {
      if (type_info->isEnum())
      {
        auto& v = *value.as<std::uint64_t*>();
        serialize(key, v, type_info);
      }
      else
      {
        const bool is_open = pushObject(key);

        if (is_open)
        {
          serialize(value, type_info);
          popObject();
        }
      }
    }
  }

  void ISerializer::serialize(Any& value, meta::BaseClassMetaInfo* type_info)
  {
    const bool is_array = type_info->isArray();

    for (auto& prop : type_info->properties())
    {
      auto field_value = prop->get(value);

      serialize(StringRange(prop->name().data(), prop->name().size()), field_value, prop->type());
      prop->set(value, field_value);
    }

    if (is_array)
    {
      std::size_t array_size;
      if (pushArray("Elements", array_size))
      {
        const std::size_t size = type_info->arraySize(value);
        char              label_buffer[s_MaxDigitsUInt64 + 1];
        LinearAllocator   label_alloc{label_buffer, sizeof(label_buffer)};

        for (std::size_t i = 0; i < size; ++i)
        {
          std::size_t idx_label_length;
          auto* const idx_label = string_utils::fmtAlloc(label_alloc, &idx_label_length, "%i", int(i));
          auto        element   = type_info->arrayGetElementAt(value, i);

          serialize(StringRange(idx_label, idx_label_length), element, type_info->containedType());
          type_info->arraySetElementAt(value, i, element);

          label_alloc.clear();
        }

        popArray();
      }
    }
  }

  bool BaseAssetInfo::defaultLoad(Engine& engine)
  {
    const String full_path = engine.assets().fullPath(*this);

    File file{full_path, file::FILE_MODE_READ};

    if (file)
    {
      std::size_t buffer_size;
      char*       buffer = file.readAll(engine.tempMemoryNoFree(), buffer_size);

      json::Value json_value = json::fromString(buffer, buffer_size - 1);

      JsonSerializerReader reader{engine.assets(), engine.tempMemoryNoFree(), json_value};

      ISerializer& serializer = reader;

      serializer.beginDocument(false);
      serializer.serialize(*payload());
      serializer.endDocument();

      file.close();

      return true;
    }

    return false;
  }

  BaseAssetHandle::BaseAssetHandle(Engine& engine, BaseAssetInfo* info, meta::BaseClassMetaInfo* type_info) :
    m_Engine{&engine},
    m_Info{info},
    m_TypeInfo{type_info}
  {
    acquire();
  }

  BaseAssetHandle::BaseAssetHandle(meta::BaseClassMetaInfo* type_info) noexcept :
    m_Engine{nullptr},
    m_Info{nullptr},
    m_TypeInfo{type_info}
  {
  }

  BaseAssetHandle::BaseAssetHandle(const BaseAssetHandle& rhs) noexcept :
    m_Engine{rhs.m_Engine},
    m_Info(rhs.m_Info),
    m_TypeInfo{rhs.m_TypeInfo}
  {
    acquire();
  }

  BaseAssetHandle::BaseAssetHandle(BaseAssetHandle&& rhs) noexcept :
    m_Engine{rhs.m_Engine},
    m_Info{rhs.m_Info},
    m_TypeInfo{rhs.m_TypeInfo}
  {
    rhs.m_Engine = nullptr;
    rhs.m_Info   = nullptr;
  }

  BaseAssetHandle& BaseAssetHandle::operator=(const BaseAssetHandle& rhs) noexcept
  {
    if (this != &rhs)
    {
      release();
      m_Engine = rhs.m_Engine;
      m_Info   = rhs.m_Info;
      acquire();
    }

    return *this;
  }

  BaseAssetHandle& BaseAssetHandle::operator=(BaseAssetHandle&& rhs) noexcept
  {
    if (this != &rhs)
    {
      release();
      m_Engine     = rhs.m_Engine;
      m_Info       = rhs.m_Info;
      rhs.m_Engine = nullptr;
      rhs.m_Info   = nullptr;
    }

    return *this;
  }

  bool BaseAssetHandle::isValid() const
  {
    return m_Info != nullptr;
  }

  void BaseAssetHandle::release()
  {
    if (m_Info)
    {
      if (--m_Info->m_RefCount == 0)
      {
        m_Info->unload();
      }

      m_Engine = nullptr;
      m_Info   = nullptr;
    }
  }

  BaseAssetHandle::~BaseAssetHandle()
  {
    release();
  }

  void BaseAssetHandle::acquire()
  {
    if (m_Info)
    {
      if (m_Info->m_RefCount == 0)
      {
        if (!m_Info->load(*m_Engine))
        {
          m_Engine = nullptr;
          m_Info   = nullptr;
          return;
        }
      }

      ++m_Info->m_RefCount;
    }
  }

  IBaseObject* BaseAssetHandle::payload() const
  {
    return m_Info ? m_Info->payload() : nullptr;
  }
}  // namespace bifrost
