/******************************************************************************/
/*!
 * @file   bifrost_asset_handle.hpp
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
#ifndef BIFROST_ASSET_HANDLE_HPP
#define BIFROST_ASSET_HANDLE_HPP

#include "bifrost/core/bifrost_base_object.hpp"             /* BaseObject<T>        */
#include "bifrost/data_structures/bifrost_dynamic_string.h" /* BifrostString        */
#include "bifrost/data_structures/bifrost_string.hpp"       /* StringRange          */
#include "bifrost/utility/bifrost_non_copy_move.hpp"        /* bfNonCopyMoveable<T> */
#include "bifrost/utility/bifrost_uuid.h"                   /* BifrostUUID          */

#include <cstdint> /* uint16_t    */

class BifrostEngine;

namespace bifrost
{
  using Engine = ::BifrostEngine;

  // TODO(Shareef): Remove the extra data in 'BaseAssetHandle' for non editor builds.
  // TODO(Shareef): Make is so 'AssetTagList::m_Tags' does not own the string but rather just references a string pool or something.

  namespace string_utils
  {
    inline BifrostString fromStringview(const std::string_view source)
    {
      return String_newLen(source.data(), source.length());
    }
  }  // namespace string_utils

  class AssetTagList
  {
   private:
    BifrostString m_Tags[4];  //!< An asset can have up to 4 tags associated with it.

   public:
    explicit AssetTagList() :
      m_Tags{nullptr, nullptr, nullptr, nullptr}
    {
    }

    BifrostString* begin()
    {
      return m_Tags;
    }

    BifrostString* end()
    {
      BifrostString* i;

      for (i = m_Tags + 3; i != m_Tags; --i)
      {
        if (*i)
        {
          break;
        }
      }

      return i;
    }
  };

  class BaseAssetInfo;
  class BaseAssetHandle;

  class ISerializer
  {
    enum
    {
      IS_LOADING = bfBit(0),
      IS_SAVING  = bfBit(1),
    };

   public:
    virtual void pushObject(StringRange key)                        = 0;
    virtual void pushArray(StringRange key)                         = 0;
    virtual void serialize(StringRange key, std::int8_t value)      = 0;
    virtual void serialize(StringRange key, std::uint8_t value)     = 0;
    virtual void serialize(StringRange key, std::int16_t value)     = 0;
    virtual void serialize(StringRange key, std::uint16_t value)    = 0;
    virtual void serialize(StringRange key, std::int32_t value)     = 0;
    virtual void serialize(StringRange key, std::uint32_t value)    = 0;
    virtual void serialize(StringRange key, std::int64_t value)     = 0;
    virtual void serialize(StringRange key, std::uint64_t value)    = 0;
    virtual void serialize(StringRange key, float value)            = 0;
    virtual void serialize(StringRange key, double value)           = 0;
    virtual void serialize(StringRange key, long double value)      = 0;
    virtual void serialize(StringRange key, String& value)          = 0;
    virtual void serialize(StringRange key, BaseAssetInfo& value)   = 0;
    virtual void serialize(StringRange key, BaseAssetHandle& value) = 0;
    virtual void serialize(StringRange key, IBaseObject& value)     = 0;
    virtual void popObject(StringRange key)                         = 0;
    virtual void popArray(StringRange key)                          = 0;

    virtual ~ISerializer() = default;
  };

  class BaseAssetInfo : public BaseObject<BaseAssetInfo>
  {
    friend class BaseAssetHandle;

   protected:
    String        m_Path;      //!< A path relative to the project to the actual asset file.
    BifrostUUID   m_UUID;      //!< Uniquely identifies the asset.
    std::uint16_t m_RefCount;  //!< How many live references in the engine.
    AssetTagList  m_Tags;      //!< Tags associated with this asset.
    void*         m_Payload;

   protected:
    BaseAssetInfo(const StringRange path, BifrostUUID uuid) :
      m_Path{path},
      m_UUID{uuid},
      m_RefCount{0u},
      m_Tags{},
      m_Payload{nullptr}
    {
    }

   public:
    const String&      path() const { return m_Path; }
    const BifrostUUID& uuid() const { return m_UUID; }
    std::uint16_t      refCount() const { return m_RefCount; }

    virtual bool load(Engine& engine)
    {
      (void)engine;
      return false;
    }

    virtual void unload(Engine& engine)
    {
      (void)engine;
    }

    virtual void serialize(Engine& engine, ISerializer& serializer)
    {
      (void)engine;
      (void)serializer;
    }
  };

  // clang-format off
  template<typename TSelf>
  class AssetInfo : public BaseAssetInfo, private bfNonCopyMoveable<AssetInfo<TSelf>>
  // clang-format on
  {
   private:
    static meta::BaseClassMetaInfo* s_IsRegistered;

    static meta::BaseClassMetaInfo* registerImpl()
    {
      return meta::TypeInfo<TSelf>::get();
    }

   public:
    AssetInfo(const StringRange path, const BifrostUUID uuid) :
      BaseAssetInfo(path, uuid),
      bfNonCopyMoveable<AssetInfo<TSelf>>()
    {
      static_assert(std::is_base_of<AssetInfo<TSelf>, TSelf>::value, "This is the CRTP, so you must inherit from AssetHandle<TSelf>");

      // Force the Compiler to register the type.
      (void)s_IsRegistered;
    }

    meta::BaseClassMetaInfo* type() override
    {
      return s_IsRegistered;
    }
  };

  template<typename TSelf>
  meta::BaseClassMetaInfo* AssetInfo<TSelf>::s_IsRegistered = registerImpl();

  class BaseAssetHandle
  {
   protected:
    Engine*        m_Engine;
    BaseAssetInfo* m_Info;

   protected:
    explicit BaseAssetHandle(Engine& engine, BaseAssetInfo* info);

   public:
    // NOTE(Shareef): Only Invalid 'AssetHandle's may be constructed from external sources.
    explicit BaseAssetHandle() noexcept;
    BaseAssetHandle(const BaseAssetHandle& rhs) noexcept;
    BaseAssetHandle(BaseAssetHandle&& rhs) noexcept;
    BaseAssetHandle& operator=(const BaseAssetHandle& rhs) noexcept;
    BaseAssetHandle& operator=(BaseAssetHandle&& rhs) noexcept;

    operator bool() const;
    bool isValid() const;
    void release();

    bool operator==(const BaseAssetHandle& rhs) const { return m_Info == rhs.m_Info; }
    bool operator!=(const BaseAssetHandle& rhs) const { return m_Info != rhs.m_Info; }

    ~BaseAssetHandle();

   protected:
    void  acquire() const;
    void* payload() const;
  };

  template<typename T, typename TInfo>
  class AssetHandle : public BaseAssetHandle
  {
    friend class Assets;

   private:
    explicit AssetHandle(Engine& engine, BaseAssetInfo* info) :
      BaseAssetHandle(engine, info)
    {
    }

   public:
    // NOTE(Shareef): Only Invalid 'AssetHandle's may be constructed from external sources.
    AssetHandle() :
      BaseAssetHandle()
    {
    }

    // NOTE(Shareef): Useful to set a handle to nullptr to represent null.
    AssetHandle(std::nullptr_t) :
      BaseAssetHandle()
    {
    }

    AssetHandle& operator=(std::nullptr_t)
    {
      release();
      return *this;
    }

    AssetHandle(const AssetHandle& rhs) = default;
    AssetHandle(AssetHandle&& rhs)      = default;
    AssetHandle& operator=(const AssetHandle& rhs) = default;
    AssetHandle& operator=(AssetHandle&& rhs) = default;

    TInfo* info() const { return static_cast<TInfo*>(m_Info); }

    operator T*() const { return static_cast<T*>(payload()); }
    T* operator->() const { return static_cast<T*>(payload()); }
    T& operator*() const { return *static_cast<T*>(payload()); }

    ~AssetHandle() = default;
  };

}  // namespace bifrost

// TODO(Shareef): Make this Another Header?
#include "bifrost/graphics/bifrost_gfx_api.h"

namespace bifrost
{
  namespace detail
  {
    void releaseGfxHandle(Engine& engine, bfGfxBaseHandle handle);

    template<typename T>
    class GfxHandle : public AssetInfo<GfxHandle<T>>
    {
      BIFROST_META_FRIEND;

     protected:
      T m_Handle;

     public:
      GfxHandle(const StringRange path, const BifrostUUID uuid) :
        AssetInfo<GfxHandle<T>>(path, uuid),
        m_Handle{nullptr}
      {
        this->m_Payload = &m_Handle;
      }

      T handle() const { return m_Handle; }

      void unload(Engine& engine) override final
      {
        releaseGfxHandle(engine, m_Handle);
      }
    };
  }  // namespace detail

  // Asset Infos

  class AssetTextureInfo final : public detail::GfxHandle<bfTextureHandle>
  {
    using BaseT = GfxHandle<bfTextureHandle>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
  };

  using AssetShaderModuleInfo = detail::GfxHandle<bfShaderModuleHandle>;
  using AssetShadeProgramInfo = detail::GfxHandle<bfShaderProgramHandle>;

  // Asset Handles

  using AssetTextureHandle       = AssetHandle<bfTextureHandle, AssetTextureInfo>;
  using AssetShaderModuleHandle  = AssetHandle<bfShaderModuleHandle, AssetShaderModuleInfo>;
  using AssetShaderProgramHandle = AssetHandle<bfShaderProgramHandle, AssetShadeProgramInfo>;
}  // namespace bifrost

#define BIFROST_META_REGISTER_GFX(n)           \
  BIFROST_META_REGISTER(bifrost::n)            \
  {                                            \
    BIFROST_META_BEGIN()                       \
      BIFROST_META_MEMBERS(                    \
       meta::class_info<n>(#n),                \
       meta::ctor<StringRange, BifrostUUID>()) \
    BIFROST_META_END()                         \
  }

BIFROST_META_REGISTER_GFX(AssetTextureInfo)
BIFROST_META_REGISTER_GFX(AssetShaderModuleInfo)
BIFROST_META_REGISTER_GFX(AssetShadeProgramInfo)

#undef BIFROST_META_REGISTER_GFX

#endif /* BIFROST_ASSET_HANDLE_HPP */
