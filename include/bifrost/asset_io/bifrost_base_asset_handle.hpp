/******************************************************************************/
/*!
 * @file   bifrost_base_asset_handle.hpp
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
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BIFROST_BASE_ASSET_HANDLE_HPP
#define BIFROST_BASE_ASSET_HANDLE_HPP

class BifrostEngine;

// TODO(SR): Make a header for math forward decls
typedef struct Vec2f_t       Vec2f;
typedef struct Vec3f_t       Vec3f;
typedef struct Quaternionf_t Quaternionf;
typedef struct bfColor4f_t   bfColor4f;
typedef struct bfColor4u_t   bfColor4u;

namespace bifrost
{
  class IBaseObject;
  class BaseObjectT;
  class BaseAssetInfo;

  namespace meta
  {
    class BaseClassMetaInfo;
  }

  using Engine = ::BifrostEngine;

  //
  // This class MUST not have any virtual functions. (or anything else that violates this being standard layout)
  // All subclasses much not add any data members (AssetHandle<T> is the only canon subclass).
  //
  class BaseAssetHandle
  {
    friend class Assets;

   protected:
    Engine*                  m_Engine;
    BaseAssetInfo*           m_Info;
    meta::BaseClassMetaInfo* m_TypeInfo;

   protected:
    explicit BaseAssetHandle(Engine& engine, BaseAssetInfo* info, meta::BaseClassMetaInfo* type_info);
    explicit BaseAssetHandle(meta::BaseClassMetaInfo* type_info) noexcept;

   public:
    BaseAssetHandle(const BaseAssetHandle& rhs) noexcept;
    BaseAssetHandle(BaseAssetHandle&& rhs) noexcept;
    BaseAssetHandle& operator=(const BaseAssetHandle& rhs) noexcept;
    BaseAssetHandle& operator=(BaseAssetHandle&& rhs) noexcept;

    operator bool() const { return isValid(); }
    bool                     isValid() const;
    void                     release();
    BaseAssetInfo*           info() const { return m_Info; }
    IBaseObject*             payload() const;
    meta::BaseClassMetaInfo* typeInfo() const { return m_TypeInfo; }

    bool operator==(const BaseAssetHandle& rhs) const { return m_Info == rhs.m_Info; }
    bool operator!=(const BaseAssetHandle& rhs) const { return m_Info != rhs.m_Info; }

    ~BaseAssetHandle();

   protected:
    void acquire();
  };
}  // namespace bifrost

#endif /* BIFROST_BASE_ASSET_HANDLE_HPP */
