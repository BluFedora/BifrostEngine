/******************************************************************************/
/*!
 * @file   bifrost_assets.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Asset / Resource manager for this engine.
 *
 *   References:
 *     [https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#short-vs-long-names]
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_ASSETS_HPP
#define BF_ASSETS_HPP

#include "bf/asset_io/bf_asset_map.hpp"          /* AssetMap                    */
#include "bf/asset_io/bf_base_asset.hpp"         /* AssetMetaInfo, IBaseAsset   */
#include "bf/asset_io/bf_document.hpp"           /* Document                    */
#include "bf/asset_io/bf_iasset_importer.hpp"    /* ImportRegistry              */
#include "bf/bf_non_copy_move.hpp"               /* NonCopyMoveable<T>          */
#include "bf/meta/bifrost_meta_runtime_impl.hpp" /* BaseClassMetaInfo, TypeInfo */

#include <mutex> /* mutex */

namespace bf
{
  namespace json
  {
    class Value;
  }

  class Engine;

  enum class AssetError : std::uint8_t
  {
    NONE,
    UNKNOWN_STL_ERROR,
    PATH_DOES_NOT_EXIST,
  };

  namespace path
  {
    static constexpr std::size_t MAX_LENGTH = 512;  // TODO(SR): This needs be be deprecated.

    struct DirectoryEntry;

    bool            doesExist(const char* path);
    bool            createDirectory(const char* path);
    bool            renameDirectory(const char* full_path, const char* new_name);  // ex: renameDirectory("C:/my/path", "new_path_name") => "C:/my/new_path_name
    bool            moveDirectory(const char* dst_path, const char* src_path);     // ex: moveDirectory("C:/my/path", "C:/some/folder") => "C:/my/path/folder"
    bool            deleteDirectory(const char* path);
    DirectoryEntry* openDirectory(IMemoryManager& memory, const StringRange& path);
    bool            isDirectory(const DirectoryEntry* entry);
    bool            isFile(const DirectoryEntry* entry);
    const char*     entryFilename(const DirectoryEntry* entry);
    bool            readNextEntry(DirectoryEntry* entry);
    void            closeDirectory(DirectoryEntry* entry);

    bool renameFile(const StringRange& old_name, const StringRange& new_name);  // ex: renameFile("path/to/my/file.txt", "new_path/to/file2.txt")
  }                                                                             // namespace path

  // Strong Typing of Paths

  struct AbsPath
  {
    StringRange path;

    explicit AbsPath(StringRange path) :
      path{path}
    {
    }
  };

  struct RelPath
  {
    StringRange path;

    explicit RelPath(StringRange path) :
      path{path}
    {
    }
  };

  enum class AssetFindOption
  {
    TRY_LOAD_ASSET,
    DONT_LOAD_ASSET,
  };

  class Assets final : public NonCopyMoveable<Assets>
  {
    friend class IBaseAsset;

   public:
    inline static const StringRange k_MetaFileExtension = ".meta";

   private:
    Engine&             m_Engine;            //!< The engine this asset system is attached to (TODO(SR): remove since this is mostly used as scratch memory).
    IMemoryManager&     m_Memory;            //!< Where to grab memory for the asset info.
    BifrostString       m_RootPath;          //!< Base Path that all assets are relative to.
    AssetMap            m_AssetSet;          //!< Owns the memory for the associated 'Document*'s.
    ImportRegistry      m_Importers;         //!< Allows installing of handlers for certain file extensions.
    ListView<IDocument> m_DirtyAssets;       //!< Assets that have unsaved modifications.
    std::mutex          m_DirtyAssetsMutex;  //!< Protects concurrent access to `m_DirtyAssets`.

   public:
    explicit Assets(Engine& engine, IMemoryManager& memory);

    // Register some file extension handler.
    void registerFileExtensions(std::initializer_list<StringRange> exts, AssetImporterFn create_fn, void* user_data = nullptr);

    IDocument* findDocument(const bfUUIDNumber& uuid);
    IDocument* findDocument(AbsPath abs_path, AssetFindOption load_option = AssetFindOption::TRY_LOAD_ASSET);
    IDocument* findDocument(RelPath rel_path, AssetFindOption load_option = AssetFindOption::TRY_LOAD_ASSET);
    IDocument* loadDocument(const StringRange& abs_path);

    template<typename T, typename PathType>
    T* findAssetOfType(PathType path, AssetFindOption load_option = AssetFindOption::TRY_LOAD_ASSET)
    {
      IDocument* const document = findDocument(path, load_option);

      if (document)
      {
        return document->findAnyResourceOfType<T>();
      }

      return nullptr;
    }

    IBaseAsset* findAsset(const ResourceReference& ref_id)
    {
      IDocument* const document = findDocument(ref_id.doc_id);

      return document ? document->findResource(ref_id.file_id) : nullptr;
    }

    template<typename F>
    void forEachAssetOfType(meta::BaseClassMetaInfo* type, F&& callback)
    {
      m_AssetSet.forEach([type, &callback](IDocument* document) {
        for (IBaseAsset& asset : document->m_AssetList)
        {
          if (asset.type() == type)
          {
            callback(&asset);
          }
        }
      });
    }

    void markDirty(IBaseAsset* asset);
    void markDirty(IDocument* asset);

    AssetError setRootPath(std::string_view path);  // TODO(SR): Use 'StringRange'.
    void       setRootPath(std::nullptr_t);
    bool       writeJsonToFile(const StringRange& path, const json::Value& value) const;
    void       saveAssets();
    void       clearDirtyList();

    // Path Conversions

    String      relPathToAbsPath(const StringRange& rel_path) const;
    StringRange absPathToRelPath(const StringRange& abs_path) const;  // This function returns a StringRange that indexes into the passed in abs_path, so be careful about lifetimes.
    String      absPathToMetaPath(const StringRange& abs_path) const;
    String      resolvePath(const StringRange& abs_or_asset_path) const;

    // TODO: Remove These
    Engine&         engine() const { return m_Engine; }
    IMemoryManager& memory() const { return m_Memory; }

    IBaseAsset* loadAsset(const StringRange& abs_path);

    ~Assets();

   private:
    AssetImporter findAssetImporter(StringRange path) const;
    void          saveDocumentMetaInfo(LinearAllocator& temp_alloc, IDocument* document);
  };
}  // namespace bf

#endif /* BF_ASSETS_HPP */
