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

#include "bifrost/core/bifrost_engine.hpp" /* Engine */

namespace bifrost
{
  static constexpr int s_MaxDigitsUInt64 = 20;

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

    meta::for_each_template_and_pointer<
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
     String>([this, &key, &value, &is_primitive](auto t) {
      using T = bfForEachTemplateT(t);

      if (!is_primitive && value.is<T>())
      {
        if constexpr (std::is_pointer_v<T>)
        {
          auto& v = *value.as<T>();
          serialize(key, v);
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
      bool is_open;
      if (type_info->isArray())
      {
        is_open = pushArray(key);
      }
      else
      {
        is_open = pushObject(key);
      }

      if (is_open)
      {
        serialize(value, type_info);

        if (type_info->isArray())
        {
          popArray();
        }
        else
        {
          popObject();
        }
      }
    }
  }

  void ISerializer::serialize(Any& value, meta::BaseClassMetaInfo* type_info)
  {
    for (auto& prop : type_info->properties())
    {
      auto field_value = prop->get(value);

      serialize(StringRange(prop->name().data(), prop->name().size()), field_value, prop->type());
      prop->set(value, field_value);
    }

    if (type_info->isArray())
    {
      const std::size_t size = type_info->arraySize(value);
      char              label_buffer[s_MaxDigitsUInt64 + 1];
      LinearAllocator   label_alloc{label_buffer, sizeof(label_buffer)};

      for (std::size_t i = 0; i < size; ++i)
      {
        std::size_t idx_label_length;
        auto* const idx_label = string_utils::alloc_fmt(label_alloc, &idx_label_length, "%i", int(i));
        auto        element   = type_info->arrayGetElementAt(value, i);

        serialize(StringRange(idx_label, idx_label_length), element, type_info->containedType());
        type_info->arraySetElementAt(value, i, element);

        label_alloc.clear();
      }
    }
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
        m_Info->unload(*m_Engine);
        m_Info->destroy();
      }

      m_Engine = nullptr;
      m_Info   = nullptr;
    }
  }

  BaseAssetHandle::~BaseAssetHandle()
  {
    release();
  }

  void BaseAssetHandle::acquire() const
  {
    if (m_Info)
    {
      if (m_Info->m_RefCount == 0)
      {
        if (!m_Info->load(*m_Engine))
        {
          // TODO(Shareef): Handle this better.
          //   Failed to load asset.
          __debugbreak();
        }
      }

      ++m_Info->m_RefCount;
    }
  }

  void* BaseAssetHandle::payload() const
  {
    return m_Info ? m_Info->payload() : nullptr;
  }

  void detail::releaseGfxHandle(Engine& engine, bfGfxBaseHandle handle)
  {
    const bfGfxDeviceHandle device = bfGfxContext_device(engine.renderer().context());

    bfGfxDevice_release(device, handle);
  }

  bool AssetTextureInfo::load(Engine& engine)
  {
    const bfGfxDeviceHandle device = bfGfxContext_device(engine.renderer().context());

    const bfTextureCreateParams params = bfTextureCreateParams_init2D(
     BIFROST_TEXTURE_UNKNOWN_SIZE,
     BIFROST_TEXTURE_UNKNOWN_SIZE,
     BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM);

    m_Payload.set<bfTextureHandle>(bfGfxDevice_newTexture(device, &params));

    const String full_path = engine.assets().fullPath(*this);

    return bfTexture_loadFile(m_Payload.as<bfTextureHandle>(), full_path.cstr());
  }
}  // namespace bifrost
