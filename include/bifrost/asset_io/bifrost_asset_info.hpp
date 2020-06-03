/******************************************************************************/
/*!
 * @file   bifrost_asset_info.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  // TODO: Make this header leaner
 *
 * @version 0.0.1
 * @date    2020-05-31
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BIFROST_ASSET_INFO_HPP
#define BIFROST_ASSET_INFO_HPP

#include "bifrost/data_structures/bifrost_dynamic_string.h" /* BifrostString        */
#include "bifrost/data_structures/bifrost_string.hpp"       /* StringRange          */
#include "bifrost/data_structures/bifrost_variant.hpp"      /* Variant<Ts...>       */
#include "bifrost/math/bifrost_rect2.hpp"                   // TODO: Find a way to get this to be fwd declared.
#include "bifrost/meta/bifrost_meta_runtime_impl.hpp"       /* meta::TypeInfo       */
#include "bifrost/utility/bifrost_non_copy_move.hpp"        /* bfNonCopyMoveable<T> */
#include "bifrost/utility/bifrost_uuid.h"                   /* BifrostUUID          */

#include <cstdint> /* uint16_t    */

class BifrostEngine;

typedef struct Vec2f_t       Vec2f;
typedef struct Vec3f_t       Vec3f;
typedef struct Quaternionf_t Quaternionf;
typedef struct bfColor4f_t   bfColor4f;
typedef struct bfColor4u_t   bfColor4u;

namespace bifrost
{
  class Any;
  class IBaseObject;
  class BaseObjectT;

  namespace meta
  {
    class BaseClassMetaInfo;
  }

  using Engine = ::BifrostEngine;

  // TODO(Shareef): Remove the extra data in 'BaseAssetHandle' for non editor builds.
  // TODO(Shareef): Make is so 'AssetTagList::m_Tags' does not own the string but rather just references a string pool or something.

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

    ////////////////////////////////////////////////////////////////////////////////
    //
    // API / Implementation Notes:
    //   * If you are within an Array all 'StringRange key' paramaters are ignored,
    //     as a result of this condition you may pass in nullptr.
    //       > An implemetation is allowed to do something special with the key
    //         if it is not nullptr though.
    //
    //   * The return value in pushArray's "std::size_t& size" is only useful for
    //     SerializerMode::LOADING. Otherise you are going to receive 0.
    //
    //   * Scopes for 'pushObject' and 'pushArray' are only valid if they return true.
    //     Only class 'popObject' and 'popArray' respectively only if 'pushXXX' returned true.
    //
    //   * Only begin reading the document if 'beginDocument' returned true.
    //
    ////////////////////////////////////////////////////////////////////////////////

    virtual bool beginDocument(bool is_array = false) = 0;
    virtual bool hasKey(StringRange key)
    {
      (void)key;
      return false;
    }
    virtual bool pushObject(StringRange key)                   = 0;
    virtual bool pushArray(StringRange key, std::size_t& size) = 0;
    virtual void serialize(StringRange key, std::byte& value) { serialize(key, *reinterpret_cast<std::uint8_t*>(&value)); }
    virtual void serialize(StringRange key, bool& value)          = 0;
    virtual void serialize(StringRange key, std::int8_t& value)   = 0;
    virtual void serialize(StringRange key, std::uint8_t& value)  = 0;
    virtual void serialize(StringRange key, std::int16_t& value)  = 0;
    virtual void serialize(StringRange key, std::uint16_t& value) = 0;
    virtual void serialize(StringRange key, std::int32_t& value)  = 0;
    virtual void serialize(StringRange key, std::uint32_t& value) = 0;
    virtual void serialize(StringRange key, std::int64_t& value)  = 0;
    virtual void serialize(StringRange key, std::uint64_t& value) = 0;
    virtual void serialize(StringRange key, float& value)         = 0;
    virtual void serialize(StringRange key, double& value)        = 0;
    virtual void serialize(StringRange key, long double& value)   = 0;
    virtual void serialize(StringRange key, Vec2f& value);
    virtual void serialize(StringRange key, Vec3f& value);
    virtual void serialize(StringRange key, Quaternionf& value);
    virtual void serialize(StringRange key, bfColor4f& value);
    virtual void serialize(StringRange key, bfColor4u& value);
    virtual void serialize(StringRange key, String& value)          = 0;
    virtual void serialize(StringRange key, BifrostUUID& value)     = 0;
    virtual void serialize(StringRange key, BaseAssetHandle& value) = 0;
    virtual void serialize(StringRange key, IBaseObject& value);
    virtual void serialize(StringRange key, meta::MetaObject& value);
    virtual void serialize(IBaseObject& value);
    virtual void serialize(StringRange key, meta::MetaVariant& value);
    virtual void serialize(meta::MetaVariant& value, meta::BaseClassMetaInfo* type_info);
    virtual void popObject()   = 0;
    virtual void popArray()    = 0;
    virtual void endDocument() = 0;

    // Helpers
    void serialize(StringRange key, Vector2f& value);
    void serialize(StringRange key, Vector3f& value);

    template<typename T>
    void serializeT(StringRange key, T* value)
    {
      if (pushObject(key))
      {
        serializeT(value);
        popObject();
      }
    }

    template<typename T>
    void serializeT(T* value)
    {
      auto v = meta::makeVariant(value);
      serialize(v, meta::TypeInfo<T>::get());
    }

    virtual ~ISerializer() = default;
  };

  class BaseAssetInfo : private bfNonCopyMoveable<BaseAssetInfo>
  {
    friend class Assets;
    friend class BaseAssetHandle;

   protected:
    String                   m_Path;      //!< A path relative to the project to the actual asset file.
    BifrostUUID              m_UUID;      //!< Uniquely identifies the asset.
    std::uint16_t            m_RefCount;  //!< How many live references in the engine.
    AssetTagList             m_Tags;      //!< Tags associated with this asset.
    bool                     m_IsDirty;   //!< This asset wants to be saved.
    meta::BaseClassMetaInfo* m_TypeInfo;  //!< The type info for the subclasses.

   protected:
    BaseAssetInfo(const StringRange path, BifrostUUID uuid) :
      bfNonCopyMoveable<BaseAssetInfo>(),
      m_Path{path},
      m_UUID{uuid},
      m_RefCount{0u},
      m_Tags{},
      m_IsDirty{false},
      m_TypeInfo{nullptr}
    {
    }

   public:
    const String&      path() const { return m_Path; }
    const BifrostUUID& uuid() const { return m_UUID; }
    std::uint16_t      refCount() const { return m_RefCount; }

    // Implemented by AssetInfo<T, TPayload> //

    virtual IBaseObject*             payload()           = 0;
    virtual meta::BaseClassMetaInfo* payloadType() const = 0;
    virtual void                     unload()            = 0;

    // Implemented by children of AssetInfo<T, TPayload> //

    // Called when the asset should be loaded.
    virtual bool load(Engine& engine)
    {
      (void)engine;
      return false;
    }

    // Serializing of Asset Content
    virtual bool save(Engine& engine, ISerializer& serializer)
    {
      (void)engine;
      (void)serializer;
      return false;
    }

    // Serializing of Meta Content
    virtual void serialize(Engine& engine, ISerializer& serializer)
    {
      (void)engine;
      (void)serializer;
    }

    // Helpers //
    bool defaultLoad(Engine& engine);

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
      static_assert(std::is_base_of<BaseObjectT, TPayload>::value, "Only reflect-able types should be used as a payload.");

      // Force the Compiler to register the type.
      (void)s_IsRegistered;
    }

    IBaseObject* payload() override final
    {
      return m_Payload.template is<TPayload>() ? &m_Payload.template as<TPayload>() : nullptr;
    }

    meta::BaseClassMetaInfo* payloadType() const override final
    {
      return s_IsRegistered;
    }

    void unload() override final
    {
      m_Payload.destroy();
    }
  };

  template<typename TPayload, typename TInfo>
  meta::BaseClassMetaInfo* AssetInfo<TPayload, TInfo>::s_IsRegistered = registerImpl();
}  // namespace bifrost

#endif /* BIFROST_ASSET_INFO_HPP */
