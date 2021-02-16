/******************************************************************************/
/*!
 * @file   bf_base_asset.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Interface for creating asset types the engine can use.
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_BASE_ASSET_HPP
#define BF_BASE_ASSET_HPP

#include "bf/ListView.hpp"                  // ListView<T>
#include "bf/bf_core.h"                     // bfBit, bfPureInterface
#include "bf/bf_non_copy_move.hpp"          // NonCopyMoveable<T>
#include "bf/core/bifrost_base_object.hpp"  // IBaseObject

#include <atomic>  // atomic_uint16_t

namespace bf
{
  class Assets;
  class Engine;
  class ISerializer;

  enum class AssetStatus
  {
    UNLOADED,  //!< (RefCount == 0)                                : Asset is not loaded.
    FAILED,    //!< (RefCount == 0) && (Flags & FailedToLoad) != 0 : Asset tried to load but could not.
    LOADING,   //!< (RefCount != 0) && (Flags & IsLoaded) == 0     : Asset loading on background thread.
    LOADED,    //!< (RefCount != 0) && (Flags & IsLoaded) != 0     : Asset is fully loaded.
  };

  namespace AssetFlags
  {
    enum
    {
      DEFAULT            = 0x0,       //!< No flags are set by default.
      IS_LOADED          = bfBit(0),  //!< Marks that the asset has been successfully loaded.
      IS_SUBASSET        = bfBit(1),  //!< This asset only lives in memory.
      FAILED_TO_LOAD     = bfBit(2),  //!< Failed to load asset, this flag is set do that we do not continuously try to load it.
      IS_DIRTY           = bfBit(3),  //!< This asset wants to be saved.
      IS_ENGINE_ASSET    = bfBit(4),  //!< If the content saving routines should be called.
      IS_FREE_ON_RELEASE = bfBit(5),  //!< Asset's memory will not be managed by `Assets` class, so will be freed on release.
      IS_IN_MEMORY       = bfBit(6),  //!< If the asset should not attempt to load from a file.
    };
  }

  struct AssetMetaInfo
  {
    std::int32_t   version      = 1;
    BufferLen      name         = {nullptr, 0};
    bfUUIDNumber   uuid         = {};
    AssetMetaInfo* first_child  = nullptr;
    AssetMetaInfo* last_child   = nullptr;  // Last child is stored to make appending fast. (We want to append since serialization is an array, we do not want the data reversing on every save).
    AssetMetaInfo* next_sibling = nullptr;

    // Before calling this function it is expected that no resources are currently being owned.
    // (Eg: Either default constructed or after a call to `freeResources` is fine)
    void serialize(IMemoryManager& allocator, ISerializer& serializer);

    void addChild(AssetMetaInfo* child);
    void freeResources(IMemoryManager& allocator);
  };

  // clang-format off
  class bfPureInterface(IBaseAsset) : public IBaseObject
  // clang-format on
  {
    friend class Assets;

    // TODO(SR): Having data members in an interface probably breaks some of my codebase guidelines.
   private:
    bfUUIDNumber         m_UUID;
    String               m_FilePathAbs;       //!< The full path to an asset.
    StringRange          m_FilePathRel;       //!< Indexes into `IBaseAsset::m_FilePathAbs` for the relative path.
    ListView<IBaseAsset> m_SubAssets;         //!< Assets that only exists in memory spawned from this parent asset.
    ListNode<IBaseAsset> m_SubAssetListNode;  //!< Used with 'm_SubAssets' to make an intrusive non-owning linked list.
    IBaseAsset*          m_ParentAsset;       //!< The asset this subasset is associated with (assuming this is a subasset).
    ListNode<IBaseAsset> m_DirtyListNode;     //!< USed with `Assets` to keep track of which assets are dirty and should be saved.
    std::atomic_uint16_t m_RefCount;          //!< The number of live references there are to this asset.
    std::atomic_uint16_t m_Flags;             //!< Various flags about the current state of the asset.
    Assets*              m_Assets;            //!< Kind of a hack, it is useful to have this back pointer.

   public:
    IBaseAsset();
    ~IBaseAsset() override;

    // Accessors //

    [[nodiscard]] const bfUUIDNumber& uuid() const { return m_UUID; }
    [[nodiscard]] const String&       fullPath() const { return m_FilePathAbs; }
    [[nodiscard]] StringRange         relativePath() const { return m_FilePathRel; }
    [[nodiscard]] StringRange         name() const;
    [[nodiscard]] StringRange         nameWithExt() const;
    [[nodiscard]] std::uint16_t       refCount() const { return m_RefCount; }
    AssetStatus                       status() const;
    bool                              isSubAsset() const { return (m_Flags & AssetFlags::IS_SUBASSET) != 0; }
    bool                              hasSubAssets() const { return !m_SubAssets.isEmpty(); }
    const ListView<IBaseAsset>&       subAssets() const { return m_SubAssets; }

    // IO and Ref Count //

    void acquire();
    void reload();
    void release();
    void saveAssetContent(ISerializer & serializer);
    void saveAssetMeta(ISerializer & serializer);

    // Misc //

    AssetMetaInfo* generateMetaInfo(IMemoryManager & allocator) const;

    void setup(const String& full_path, Assets& assets);

   private:
    void setup(const String& full_path, std::size_t length_of_root_path, const bfUUIDNumber& uuid, Assets& assets);

    // Interface That Must Be Implemented By Subclasses. //

    virtual void onLoad()   = 0;  // Called when the asset should be loaded. (Will never be called if the asset is already loaded or the asset is a subasset)
    virtual void onUnload() = 0;  // Called when the asset should be unloaded from memory. (Will never be called if the asset is not loaded or the asset is a subasset)

    // These have default implementations but can be re-implemented by subclasses.

    virtual void onReload();                             // By Default calls "unload" then "load" but allows for subclasses to optimize the "reload" operation.
    virtual void onSaveAsset(ISerializer & serializer);  // Called when the asset should save to the source asset file, default calls `IBaseObject::reflect`.
    virtual void onSaveMeta(ISerializer & serializer);   // Called to save some extra information in the meta file, default does nothing.

   protected:
    // Helpers methods for sub classes //

    Assets& assets() const { return *m_Assets; }

    String createSubAssetPath(StringRange name_with_ext) const;

    IBaseAsset* findOrCreateSubAsset(StringRange name_with_ext);
    IBaseAsset* findOrCreateSubAsset(StringRange name_with_ext, bfUUIDNumber uuid);

    void markFailedToLoad()
    {
      m_Flags |= AssetFlags::FAILED_TO_LOAD;
    }

    void markIsLoaded()
    {
      m_Flags |= AssetFlags::IS_LOADED;
    }

    void markAsEngineAsset()
    {
      m_Flags |= AssetFlags::IS_ENGINE_ASSET;
    }
  };

  namespace detail
  {
    // clang-format off
    class BaseAssetT : public IBaseAsset, public meta::AutoRegisterType<BaseAssetT>, private NonCopyMoveable<BaseAssetT>
    {
     protected:
      explicit BaseAssetT(PrivateCtorTag) :
        IBaseAsset(),
        AutoRegisterType<BaseAssetT>(),
        NonCopyMoveable<BaseAssetT>()
      {}
    };
    // clang-format on
  }  // namespace detail

  // clang-format off
  template<typename T>
  class BaseAsset : public detail::BaseAssetT::Base<T>
  // clang-format on
  {
   public:
    using Base = BaseAsset<T>;

    BaseAsset() = default;

    meta::BaseClassMetaInfo* type() const final override
    {
      return meta::typeInfoGet<T>();
    }
  };

  //
  // ARC = Automatic Reference Count
  //

  // clang-format off
  
  //
  // This interface exists so that you can manipulate an
  // `ARC` handle generically particularly in serialization
  // and editor code.
  //
  class bfPureInterface(IARCHandle)
  // clang-format on
  {
   public:
    virtual bool                     isValid() const noexcept   = 0;
    virtual meta::BaseClassMetaInfo* typeInfo() const noexcept  = 0;
    virtual void                     assign(IBaseAsset * asset) = 0;
    virtual IBaseAsset*              handle() const noexcept    = 0;
    virtual ~IARCHandle()                                       = default;
  };

  //
  // Automatically handles calling acquire and release on the associated IBaseAsset pointer.
  //

  template<typename T>
  class ARC final : public IARCHandle
  {
    // using T = IBaseAsset;

    static_assert(!std::is_pointer_v<T>, "T must not be a pointer.");
    static_assert(std::is_convertible_v<T*, IBaseAsset*>, "The type must implement the IBaseAsset interface.");

   private:
    T* m_Handle;

   public:
    ARC(T* handle = nullptr) :
      IARCHandle(),
      m_Handle{handle}
    {
      acquire();
    }

    // NOTE(Shareef): Useful to set a handle to nullptr to represent null.
    ARC(std::nullptr_t) :
      IARCHandle(),
      m_Handle{nullptr}
    {
    }

    ARC(const ARC& rhs) :
      IARCHandle(),
      m_Handle{rhs.m_Handle}
    {
      acquire();
    }

    // clang-format off

    ARC(ARC&& rhs) noexcept :
      IARCHandle(),
      m_Handle{std::exchange(rhs.m_Handle, nullptr)}
    {
    }

    // clang-format on

    ARC& operator=(const ARC& rhs)  // NOLINT(bugprone-unhandled-self-assignment)
    {
      if (this != &rhs)
      {
        reassign(rhs.m_Handle);
      }

      return *this;
    }

    ARC& operator=(ARC&& rhs) noexcept
    {
      m_Handle = std::exchange(rhs.m_Handle, m_Handle);

      return *this;
    }

    ARC& operator=(std::nullptr_t)
    {
      release();
      m_Handle = nullptr;

      return *this;
    }

    operator bool() const noexcept { return isValid(); }
    T&   operator*() const noexcept { return *m_Handle; }
    T*   operator->() const noexcept { return m_Handle; }
    bool operator==(const ARC& rhs) const noexcept { return m_Handle == rhs.m_Handle; }
    bool operator!=(const ARC& rhs) const noexcept { return m_Handle != rhs.m_Handle; }

    T* typedHandle() const noexcept
    {
      return m_Handle;
    }

    ~ARC()
    {
      release();
    }

    // IARCHandle Interface

    bool                     isValid() const noexcept override { return m_Handle != nullptr; }
    meta::BaseClassMetaInfo* typeInfo() const noexcept override { return meta::typeInfoGet<T>(); }
    void                     assign(IBaseAsset* asset) override { reassign(asset); }
    IBaseAsset*              handle() const noexcept override { return m_Handle; }

   private:
    void reassign(IBaseAsset* asset)
    {
      if (m_Handle != asset)
      {
        assert(!asset || asset->type() == typeInfo() && "Either must be assigning nullptr or the types must match.");

        release();
        m_Handle = static_cast<T*>(asset);
        acquire();
      }
    }

    void acquire() const
    {
      if (m_Handle)
      {
        m_Handle->acquire();
      }
    }

    // NOTE(SR):
    //   This function does not set 'm_Handle' to nullptr.
    //   That is because it is a redundant assignment to 'm_Handle', ex: copy assignment.
    void release() const
    {
      if (m_Handle)
      {
        m_Handle->release();
      }
    }
  };
}  // namespace bf

#endif /* BF_BASE_ASSET_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
