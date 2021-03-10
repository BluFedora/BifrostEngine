/******************************************************************************/
/*!
 * @file   bf_document.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Abstraction over a file with a set of resources to allow for easily
 *   refering to multiple assets that may be stored in a single file.
 *
 * @version 0.0.1
 * @date    2021-03-02
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef BF_DOCUMENT_HPP
#define BF_DOCUMENT_HPP

#include "bf/ListView.hpp"                  // ListView<T>
#include "bf/core/bifrost_base_object.hpp"  // IBaseObject

#include <atomic>  // atomic_uint16_t

namespace bf
{
  static constexpr std::uint32_t k_DocumentFileFormatVersion = 1u;

  class Assets;
  class IBaseAsset;
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
      FAILED_TO_LOAD     = bfBit(1),  //!< Failed to load asset, this flag is set do that we do not continuously try to load it.
      IS_DIRTY           = bfBit(2),  //!< This asset wants to be saved.
      DESTROY_ON_RELEASE = bfBit(4),  //!< Asset's memory will not be managed by `Assets` class, so will be freed on release.
      IS_MAIN_ASSET      = bfBit(5),  //!<
    };
  }

  struct FileSaveCtx
  {
    LinearAllocatorSavePoint memory_scope;
    ISerializer*             serializer;
    bool                     has_document_began;
  };

  class IDocument
  {
    friend class Assets;

    // TODO(SR): Having data members in an interface probably breaks some of my codebase guidelines.
   private:
    std::uint32_t        m_Version;         //!< The version the loaded file contained in it's header.
    bfUUIDNumber         m_UUID;            //!< Unique id for this document allows assets to be moved and still having referencial integrity
    String               m_FilePathAbs;     //!< The full path to an document in the filesystem.
    StringRange          m_FilePathRel;     //!< Indexes into `IDocument::m_FilePathAbs` for the relative path.
    ListView<IBaseAsset> m_AssetList;       //!< The list of assets this document contains.
    std::uint32_t        m_AssetListCount;  //!< Number of assets stored in this document.
    Assets*              m_AssetManager;    //!< Backpointer to the owning asset manager.
    ListNode<IDocument>  m_DirtyListNode;   //!< Used with `Assets` to keep track of which assets are dirty and should be saved.
    std::atomic_uint16_t m_Flags;           //!< Various flags about the current state of the document.
    std::atomic_uint16_t m_RefCount;        //!< The number of live references there are to this document.

   protected:
    IDocument();

   public:
    virtual ~IDocument();

    // Accessors //

    [[nodiscard]] const bfUUIDNumber& uuid() const { return m_UUID; }
    [[nodiscard]] std::uint16_t       refCount() const { return m_RefCount; }
    [[nodiscard]] const String&       fullPath() const { return m_FilePathAbs; }
    [[nodiscard]] StringRange         relativePath() const { return m_FilePathRel; }
    [[nodiscard]] StringRange         name() const;
    [[nodiscard]] StringRange         nameWithExt() const;
    [[nodiscard]] AssetStatus         status() const;
    [[nodiscard]] std::uint32_t       numAssets() const { return m_AssetListCount; }

    // IO and Ref Count //

    void acquire();
    void release();
    void reload();
    void save();
    void serializeMetaInfo(ISerializer& serializer);

    // Resource Query //

    template<typename T>
    T* findResourceOfType(ResourceID id)
    {
      IBaseAsset* asset = findResource(id);

      // Type mismatch, just return nullptr.
      if (asset && asset->type() != meta::typeInfoGet<T>())
      {
        asset = nullptr;
      }

      return static_cast<T*>(asset);
    }

    template<typename T>
    T* findAnyResourceOfType()
    {
      for (IBaseAsset& asset : m_AssetList)
      {
        if (asset.type() == meta::typeInfoGet<T>())
        {
          return static_cast<T*>(&asset);
        }
      }

      return nullptr;
    }

   private:
    // Interface That Must Be Implemented By Subclasses. //

    virtual AssetStatus onLoad()   = 0;
    virtual void        onUnload() = 0;

    // These have default implementations but can be re-implemented by subclasses. //

    virtual IBaseAsset* findResource(ResourceID file_id);     // By default does a dumb linear search
    virtual void        onSaveAsset();                        // By default does nothing since most of the time you do not want to mess with the source assets.
    virtual void        onSaveMeta(ISerializer& serializer);  // By default writes out nothing.
    virtual void        onReload();                           // By Default calls "onUnload" then "onLoad" but allows for subclasses to optimize the "reload" operation.

   protected:
    // Helper methods for sub classes //

    Assets&         assets() const;
    IMemoryManager& assetMemory() const;

    template<typename T, typename... Args>
    T* addAsset(ResourceID id, StringRange name, Args&&... args)
    {
      T* result = findResourceOfType<T>(id);

      if (!result)
      {
        result = assetMemory().allocateT<T>(std::forward<Args>(args)...);

        addAssetImpl(result, id, name);
      }

      // Update the name regardless.
      result->m_Name = name;

      return result;
    }

    void addAssetImpl(IBaseAsset* asset, ResourceID id, StringRange name);

    FileSaveCtx defaultSaveBegin();
    void        defaultSaveEnd(FileSaveCtx& ctx);

    template<typename F>
    void defaultSave(F&& callback)
    {
      FileSaveCtx ctx = defaultSaveBegin();

      if (ctx.has_document_began)
      {
        callback(*ctx.serializer);
      }

      defaultSaveEnd(ctx);
    }

    void setPath(const String& full_path, std::size_t length_of_root_path)
    {
      m_FilePathAbs = full_path;
      m_FilePathRel = {
       m_FilePathAbs.begin() + length_of_root_path + 1u,  // The plus one accounts for the '/'
       m_FilePathAbs.end(),
      };
    }
  };

}  // namespace bf

#endif /* BF_DOCUMENT_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

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
