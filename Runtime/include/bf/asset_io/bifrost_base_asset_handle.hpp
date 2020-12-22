/******************************************************************************/
/*!
 * @file   bifrost_base_asset_handle.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  A reference counted handle to an asset.
 *  This base class must not be used directly rather use the 'AssetHandle<T>'
 *  subclass.
 *
 * @version 0.0.1
 * @date    2019-12-26
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_BASE_ASSET_HANDLE_HPP
#define BF_BASE_ASSET_HANDLE_HPP

namespace bf
{
  class IBaseObject;
  class BaseAssetInfo;
  class Engine;

  namespace meta
  {
    class BaseClassMetaInfo;
  }

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

    operator bool() const noexcept { return isValid(); }
    bool                     isValid() const noexcept { return m_Info != nullptr; }
    void                     release();
    BaseAssetInfo*           info() const noexcept { return m_Info; }
    IBaseObject*             payload() const;
    meta::BaseClassMetaInfo* typeInfo() const noexcept { return m_TypeInfo; }

    bool operator==(const BaseAssetHandle& rhs) const noexcept { return m_Info == rhs.m_Info; }
    bool operator!=(const BaseAssetHandle& rhs) const noexcept { return m_Info != rhs.m_Info; }

    ~BaseAssetHandle();

   protected:
    void acquire();
  };
}  // namespace bf

#endif /* BF_BASE_ASSET_HANDLE_HPP */
