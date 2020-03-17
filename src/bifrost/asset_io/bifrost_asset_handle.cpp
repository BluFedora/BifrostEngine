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
