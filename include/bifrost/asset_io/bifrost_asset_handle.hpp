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
#include "bifrost/data_structures/bifrost_variant.hpp"      /* Variant<Ts...>       */
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

  enum class SerializerMode
  {
    LOADING,
    SAVING,
    INSPECTING,
  };

  class ISerializer
  {
   protected:
    SerializerMode m_Mode;

   protected:
    explicit ISerializer(SerializerMode mode) :
      m_Mode{mode}
    {
    }

   public:
    SerializerMode mode() const { return m_Mode; }

    virtual void beginDocument(bool is_array = false)               = 0;
    virtual void pushObject(StringRange key)                        = 0;
    virtual void pushArray(StringRange key)                         = 0;
    virtual void serialize(StringRange key, std::int8_t& value)     = 0;
    virtual void serialize(StringRange key, std::uint8_t& value)    = 0;
    virtual void serialize(StringRange key, std::int16_t& value)    = 0;
    virtual void serialize(StringRange key, std::uint16_t& value)   = 0;
    virtual void serialize(StringRange key, std::int32_t& value)    = 0;
    virtual void serialize(StringRange key, std::uint32_t& value)   = 0;
    virtual void serialize(StringRange key, std::int64_t& value)    = 0;
    virtual void serialize(StringRange key, std::uint64_t& value)   = 0;
    virtual void serialize(StringRange key, float& value)           = 0;
    virtual void serialize(StringRange key, double& value)          = 0;
    virtual void serialize(StringRange key, long double& value)     = 0;
    virtual void serialize(StringRange key, String& value)          = 0;
    virtual void serialize(StringRange key, BaseAssetInfo& value)   = 0;
    virtual void serialize(StringRange key, BaseAssetHandle& value) = 0;
    virtual void serialize(StringRange key, IBaseObject& value)
    {
      pushObject(key);
      serialize(value);
      popObject();
    }
    virtual void serialize(IBaseObject& value) = 0;
    virtual void popObject()                   = 0;
    virtual void popArray()                    = 0;
    virtual void endDocument()                 = 0;

    virtual ~ISerializer() = default;
  };

  class BaseAssetInfo : private bfNonCopyMoveable<BaseAssetInfo>
  {
    friend class BaseAssetHandle;

   protected:
    String        m_Path;      //!< A path relative to the project to the actual asset file.
    BifrostUUID   m_UUID;      //!< Uniquely identifies the asset.
    std::uint16_t m_RefCount;  //!< How many live references in the engine.
    AssetTagList  m_Tags;      //!< Tags associated with this asset.

   protected:
    BaseAssetInfo(const StringRange path, BifrostUUID uuid) :
      bfNonCopyMoveable<BaseAssetInfo>(),
      m_Path{path},
      m_UUID{uuid},
      m_RefCount{0u},
      m_Tags{}
    {
    }

   public:
    const String&      path() const { return m_Path; }
    const BifrostUUID& uuid() const { return m_UUID; }
    std::uint16_t      refCount() const { return m_RefCount; }

    // Implemented by AssetInfo<T, TPayload>
    virtual void*                    payload()           = 0;
    virtual meta::BaseClassMetaInfo* payloadType() const = 0;
    virtual void                     destroy()           = 0;

    // Implemented by children of AssetInfo<T, TPayload>
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

    virtual ~BaseAssetInfo() = default;
  };

  template<typename TPayload, typename TInfo>
  class AssetInfo : public BaseAssetInfo
  {
   private:
    static meta::BaseClassMetaInfo* s_IsRegistered;

    static meta::BaseClassMetaInfo* registerImpl()
    {
      meta::TypeInfo<TInfo>::get();
      return meta::TypeInfo<TPayload>::get();
    }

   protected:
    Optional<TPayload> m_Payload;

   public:
    AssetInfo(const StringRange path, const BifrostUUID uuid) :
      BaseAssetInfo(path, uuid)
    {
      // static_assert(std::is_base_of<BaseObjectT, TPayload>::value, "Only reflect-able types should be used as a payload.");

      // Force the Compiler to register the type.
      (void)s_IsRegistered;
    }

    void* payload() override final
    {
      return m_Payload.template is<TPayload>() ? &m_Payload.template as<TPayload>() : nullptr;
    }

    meta::BaseClassMetaInfo* payloadType() const override final
    {
      return s_IsRegistered;
    }

    void destroy() override final
    {
      m_Payload.destroy();
    }

    void serialize(Engine& engine, ISerializer& serializer) override
    {
      (void)engine;

      if constexpr (std::is_base_of_v<IBaseObject, TPayload>)
      {
        if (payload())
        {
          serializer.beginDocument(false);
          serializer.serialize(*static_cast<TPayload*>(payload()));
          serializer.endDocument();
        }
      }
    }
  };

  template<typename TPayload, typename TInfo>
  meta::BaseClassMetaInfo* AssetInfo<TPayload, TInfo>::s_IsRegistered = registerImpl();

  //
  // This class MUST not have any virtual members.
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

   public:
    // NOTE(Shareef): Only Invalid 'AssetHandle's may be constructed from external sources.
    explicit BaseAssetHandle(meta::BaseClassMetaInfo* type_info) noexcept;
    BaseAssetHandle(const BaseAssetHandle& rhs) noexcept;
    BaseAssetHandle(BaseAssetHandle&& rhs) noexcept;
    BaseAssetHandle& operator=(const BaseAssetHandle& rhs) noexcept;
    BaseAssetHandle& operator=(BaseAssetHandle&& rhs) noexcept;

    operator bool() const { return isValid(); }
    bool                     isValid() const;
    void                     release();
    BaseAssetInfo*           info() const { return m_Info; }
    meta::BaseClassMetaInfo* typeInfo() const { return m_TypeInfo; }

    bool operator==(const BaseAssetHandle& rhs) const { return m_Info == rhs.m_Info; }
    bool operator!=(const BaseAssetHandle& rhs) const { return m_Info != rhs.m_Info; }

    ~BaseAssetHandle();

   protected:
    void  acquire() const;
    void* payload() const;
  };

  //
  // This call MUST not have a size different from 'BaseAssetHandle'.
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

// TODO(Shareef): Make this Another Header?
#include "bifrost/graphics/bifrost_gfx_api.h"

namespace bifrost
{
  namespace detail
  {
    void releaseGfxHandle(Engine& engine, bfGfxBaseHandle handle);

    template<typename TSelf, typename T>
    class GfxHandle : public AssetInfo<T, TSelf>
    {
      BIFROST_META_FRIEND;

     public:
      GfxHandle(const StringRange path, const BifrostUUID uuid) :
        AssetInfo<T, TSelf>(path, uuid)
      {
      }

      void unload(Engine& engine) override final
      {
        releaseGfxHandle(engine, *reinterpret_cast<T*>(this->payload()));
      }
    };
  }  // namespace detail

  // Asset Infos

  class AssetTextureInfo final : public detail::GfxHandle<AssetTextureInfo, bfTextureHandle>
  {
    using BaseT = GfxHandle<AssetTextureInfo, bfTextureHandle>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
  };

  class AssetShaderModuleInfo final : public detail::GfxHandle<AssetShaderModuleInfo, bfShaderModuleHandle>
  {
    using BaseT = GfxHandle<AssetShaderModuleInfo, bfShaderModuleHandle>;

   public:
    using BaseT::BaseT;
  };

  class AssetShaderProgramInfo final : public detail::GfxHandle<AssetShaderProgramInfo, bfShaderProgramHandle>
  {
    using BaseT = GfxHandle<AssetShaderProgramInfo, bfShaderProgramHandle>;

   public:
    using BaseT::BaseT;
  };

  // Asset Handles

  using AssetTextureHandle       = AssetHandle<bfTextureHandle>;
  using AssetShaderModuleHandle  = AssetHandle<bfShaderModuleHandle>;
  using AssetShaderProgramHandle = AssetHandle<bfShaderProgramHandle>;
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
BIFROST_META_REGISTER_GFX(AssetShaderProgramInfo)

#undef BIFROST_META_REGISTER_GFX

#endif /* BIFROST_ASSET_HANDLE_HPP */
