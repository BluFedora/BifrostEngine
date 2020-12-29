/******************************************************************************/
/*!
 * @file   bifrost_assets.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Asset / Resource manager for this engine.
 *
 *   References:
 *     [https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#short-vs-long-names]
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_ASSETS_HPP
#define BF_ASSETS_HPP

#include "bf/asset_io/bf_asset_map.hpp"          /* AssetMap                    */
#include "bf/asset_io/bf_base_asset.hpp"         /* AssetMetaInfo, IBaseAsset   */
#include "bf/bf_non_copy_move.hpp"               /* NonCopyMoveable<T>          */
#include "bf/meta/bifrost_meta_runtime_impl.hpp" /* BaseClassMetaInfo, TypeInfo */

namespace bf
{
  namespace json
  {
    class Value;
  }

  class JsonSerializerWriter;
  class JsonSerializerReader;
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

  template<typename AssetTInfo>
  struct AssetIndexResult
  {
    AssetTInfo* info;
    bool        is_new;
  };

  using AssetCreationFn = IBaseAsset* (*)(IMemoryManager& asset_memory, Engine& engine);

  template<typename T>
  IBaseAsset* defaultAssetCreate(IMemoryManager& asset_memory, Engine& engine)
  {
    (void)engine;

    return asset_memory.allocateT<T>();
  }

  // TODO(SR):
  //   Use of String is pretty heavy, a StringRange would be better since just about all
  //   file extensions registered are const char* hardcoded in the program.
  using FileExtenstionRegistry = HashTable<String, AssetCreationFn>;

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
    Engine&                m_Engine;       //!< The engine this asset system is attached to.
    IMemoryManager&        m_Memory;       //!< Where to grab memory for the asset info.
    BifrostString          m_RootPath;     //!< Base Path that all assets are relative to.
    AssetMap               m_AssetSet;     //!< Owns the memory for the associated 'IBaseAsset*'.
    FileExtenstionRegistry m_FileExtReg;   //!< Allows installing of handlers for certain file extensions.
    ListView<IBaseAsset>   m_DirtyAssets;  //!< Assets that have unsaved modifications.

   public:
    explicit Assets(Engine& engine, IMemoryManager& memory);

    void        registerFileExtensions(std::initializer_list<StringRange> exts, AssetCreationFn create_fn);
    IBaseAsset* findAsset(const bfUUIDNumber& uuid) const;
    IBaseAsset* findAsset(AbsPath abs_path, AssetFindOption load_option = AssetFindOption::TRY_LOAD_ASSET);
    IBaseAsset* findAsset(RelPath rel_path, AssetFindOption load_option = AssetFindOption::TRY_LOAD_ASSET);
    void        markDirty(IBaseAsset* asset);

    template<typename F>
    void forEachAssetOfType(const meta::BaseClassMetaInfo* type_info, F&& callback)
    {
      m_AssetSet.forEach([type_info, &callback](IBaseAsset* const asset) {
        if (asset->type() == type_info)
        {
          callback(asset);
        }
      });
    }

    template<typename T>
    T* findAssetOfType(AbsPath abs_path)
    {
      IBaseAsset* base_asset = findAsset(abs_path);

      // If the found asset does not match the correct type then return nullptr.
      if (base_asset && base_asset->type() != meta::typeInfoGet<T>())
      {
        base_asset = nullptr;
      }

      return static_cast<T*>(base_asset);
    }

    AssetMetaInfo* loadMetaInfo(LinearAllocator& temp_allocator, StringRange abs_path_to_meta_file);

    AssetError setRootPath(std::string_view path);  // TODO(SR): Use 'StringRange'.
    void       setRootPath(std::nullptr_t);
    bool       writeJsonToFile(const StringRange& path, const json::Value& value) const;
    void       saveAssets();
    void       saveAssetInfo(LinearAllocator& temp_alloc, IMemoryManager& temp_alloc_no_free, IBaseAsset* asset) const;
    void       saveAssetInfo(Engine& engine, IBaseAsset* asset) const;
    void       clearDirtyList();

    // Path Conversions

    String      relPathToAbsPath(const StringRange& rel_path) const;
    StringRange absPathToRelPath(const StringRange& abs_path) const;  // This function returns a StringRange that indexes into the passed in abs_path, so be careful about lifetimes.
    String      absPathToMetaPath(const StringRange& abs_path) const;
    String      resolvePath(const StringRange& abs_or_asset_path) const;

    // TODO: Remove These
    Engine&         engine() const { return m_Engine; }
    IMemoryManager& memory() const { return m_Memory; }

    ~Assets();

    IBaseAsset* loadAsset(const StringRange& abs_path);

   private:
    IBaseAsset* createAssetFromPath(StringRange path, const bfUUIDNumber& uuid);
    IBaseAsset* createAssetFromPath(StringRange path);  // Creates UUID for you.
    void        saveMetaInfo(LinearAllocator& temp_alloc, IMemoryManager& temp_alloc_no_free, IBaseAsset* asset) const;
  };
}  // namespace bf

#endif /* BF_ASSETS_HPP */
