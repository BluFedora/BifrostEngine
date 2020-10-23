/******************************************************************************/
/*!
 * @file   bifrost_asset_handle.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  Convienience wrapper for `BaseAssetHandle` reference counted asset 
 *  handle for more type safety.
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
#ifndef BIFROST_ASSET_HANDLE_HPP
#define BIFROST_ASSET_HANDLE_HPP

#include "bifrost/meta/bifrost_meta_runtime_impl.hpp" /* meta::TypeInfo<T> */
#include "bifrost_base_asset_handle.hpp"              /* BaseAssetHandle   */

namespace bf
{
  //
  // This class MUST not have a size different from 'BaseAssetHandle'.
  // This is merely a convienience wrapper.
  //
  template<typename T>
  class AssetHandle final : public BaseAssetHandle
  {
    friend class Assets;

   public:
    // NOTE(Shareef): Only Invalid 'AssetHandle's may be constructed from external sources.
    AssetHandle() :
      BaseAssetHandle(meta::TypeInfo<T>::get())
    {
    }

    // NOTE(Shareef): Useful to set a handle to nullptr to represent null.
    AssetHandle(std::nullptr_t) :
      BaseAssetHandle(meta::TypeInfo<T>::get())
    {
    }

    // Implemented in this subclass since this assignment
    // operator needs to return a 'AssetHandle' specifically.
    AssetHandle& operator=(std::nullptr_t)
    {
      release();
      return *this;
    }

    AssetHandle(const AssetHandle& rhs) = default;
    AssetHandle(AssetHandle&& rhs)      = default;
    AssetHandle& operator=(const AssetHandle& rhs) = default;
    AssetHandle& operator=(AssetHandle&& rhs) = default;

    operator T*() const { return static_cast<T*>(payload()); }
    T* operator->() const { return static_cast<T*>(payload()); }
    T& operator*() const { return *static_cast<T*>(payload()); }

    ~AssetHandle() = default;
  };
}  // namespace bifrost

#endif /* BIFROST_ASSET_HANDLE_HPP */
