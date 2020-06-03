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

#include "bifrost/asset_io/bifrost_asset_info.hpp"
#include "bifrost/asset_io/bifrost_file.hpp"
#include "bifrost/asset_io/bifrost_json_serializer.hpp"
#include "bifrost/core/bifrost_engine.hpp" /* Engine */
#include "bifrost/utility/bifrost_json.hpp"

namespace bifrost
{
  static constexpr int         s_MaxDigitsUInt64 = 20;
  static constexpr StringRange k_EnumValueKey    = "__EnumValue__";

  void ISerializer::serialize(StringRange key, Vec2f& value)
  {
    if (pushObject(key))
    {
      serialize("x", value.x);
      serialize("y", value.y);

      popObject();
    }
  }

  void ISerializer::serialize(StringRange key, Vec3f& value)
  {
    if (pushObject(key))
    {
      serialize("x", value.x);
      serialize("y", value.y);
      serialize("z", value.z);
      serialize("w", value.w);

      popObject();
    }
  }

  void ISerializer::serialize(StringRange key, Quaternionf& value)
  {
    if (pushObject(key))
    {
      serialize("x", value.x);
      serialize("y", value.y);
      serialize("z", value.z);
      serialize("w", value.w);

      popObject();
    }
  }

  void ISerializer::serialize(StringRange key, bfColor4f& value)
  {
    if (pushObject(key))
    {
      serialize("r", value.r);
      serialize("g", value.g);
      serialize("b", value.b);
      serialize("a", value.a);

      popObject();
    }
  }

  void ISerializer::serialize(StringRange key, bfColor4u& value)
  {
    if (pushObject(key))
    {
      serialize("r", value.r);
      serialize("g", value.g);
      serialize("b", value.b);
      serialize("a", value.a);

      popObject();
    }
  }

  void ISerializer::serialize(StringRange key, IBaseObject& value)
  {
    if (pushObject(key))
    {
      serialize(value);
      popObject();
    }
  }

  void ISerializer::serialize(StringRange key, meta::MetaObject& value)
  {
    if (pushObject(key))
    {
      if (value.type_info->isEnum())
      {
        serialize(k_EnumValueKey, value.enum_value);
      }
      else
      {
        auto v = meta::MetaVariant{value};
        serialize(v, value.type_info);
      }

      popObject();
    }
  }

  void ISerializer::serialize(IBaseObject& value)
  {
    auto value_variant = meta::makeVariant(&value);

    serialize(value_variant, value.type());
  }

  void ISerializer::serialize(StringRange key, meta::MetaVariant& value)
  {
    visit_all(
     meta::overloaded{[this, &key](auto& primitive_value) -> void { serialize(key, primitive_value); },
                      [this, &key](IBaseObject* base_obj) -> void { serialize(key, *base_obj); }},
     value);
  }

  void ISerializer::serialize(meta::MetaVariant& value, meta::BaseClassMetaInfo* type_info)
  {
    const bool is_array = type_info->isArray();

    for (auto& prop : type_info->properties())
    {
      auto field_value = prop->get(value);

      serialize(StringRange(prop->name().data(), prop->name().size()), field_value);

      prop->set(value, field_value);
    }

    if (is_array)
    {
      std::size_t array_size;
      if (pushArray("Elements", array_size))
      {
        const std::size_t size = type_info->numElements(value);
        char              label_buffer[s_MaxDigitsUInt64 + 1];
        LinearAllocator   label_alloc{label_buffer, sizeof(label_buffer)};

        for (std::size_t i = 0; i < size; ++i)
        {
          std::size_t idx_label_length;
          auto* const idx_label = string_utils::fmtAlloc(label_alloc, &idx_label_length, "%i", int(i));
          auto        element   = type_info->elementAt(value, i);

          serialize(StringRange(idx_label, idx_label_length), element);
          (void)type_info->setElementAt(value, i, element);

          label_alloc.clear();
        }

        popArray();
      }
    }
  }

  void ISerializer::serialize(StringRange key, Vector2f& value)
  {
    serialize(key, static_cast<Vec2f&>(value));
  }

  void ISerializer::serialize(StringRange key, Vector3f& value)
  {
    serialize(key, static_cast<Vec3f&>(value));
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
