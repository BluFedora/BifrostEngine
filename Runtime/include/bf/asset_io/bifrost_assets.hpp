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

#include "bf/asset_io/bf_asset_map.hpp"
#include "bf/bf_non_copy_move.hpp"                   /* NonCopyMoveable<T>                           */
#include "bf/data_structures/bifrost_hash_table.hpp" /* HashTable<K, V>                              */
#include "bf/data_structures/bifrost_string.hpp"     /* BifrostStringHasher, BifrostStringComparator */
#include "bf/meta/bifrost_meta_runtime_impl.hpp"     /* BaseClassMetaInfo, TypeInfo                  */
#include "bf/utility/bifrost_uuid.hpp"               /* BifrostUUID                                  */

namespace bf
{
  namespace json
  {
    class Value;
  }

  class BaseAssetInfo;
  class BaseAssetHandle;
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

  using PathToUUIDTable = HashTable<String, bfUUID, 64>;

  namespace detail
  {
    using AssetMap = HashTable<bfUUID, BaseAssetInfo*, 64, UUIDHasher, UUIDEqual>;
  }  // namespace detail

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
  //   Use of string is pretty heavy, a StringRange would be better since just about all
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

  class Assets final : public NonCopyMoveable<Assets>
  {
   public:
    inline static const char META_PATH_NAME[]      = "_meta";
    inline static const char META_FILE_EXTENSION[] = ".meta";

   public:
    static bool isHandleCompatible(const BaseAssetHandle& handle, const BaseAssetInfo* info);

   private:
    Engine&                m_Engine;          //!< The engine this asset system is attached to.
    IMemoryManager&        m_Memory;          //!< Where to grab memory for the asset info.
    PathToUUIDTable        m_NameToGUID;      //!< Allows loading assets from path rather than having to know the UUID.
    detail::AssetMap       m_AssetMap;        //!< Owns the memory for the associated 'BaseAssetHandle*'.
    BifrostString          m_RootPath;        //!< Base Path that all assets are relative to.
    String                 m_MetaPath;        //!< Path that all assets meta files are located in.
    Array<BaseAssetHandle> m_DirtyAssetList;  //!< Assets that have unsaved modifications.
    AssetMap               m_AssetSet;        //!< Owns the memory for the associated 'IBaseAsset*'.
    FileExtenstionRegistry m_FileExtReg;      //!< Allows installing of handlers for certain file extensions.

   public:
    explicit Assets(Engine& engine, IMemoryManager& memory);

    // Startup

    template<typename T>
    void registerFileExtensions(std::initializer_list<StringRange> exts, AssetCreationFn create_fn = &defaultAssetCreate<T>)
    {
      static_assert(std::is_convertible_v<T*, IBaseAsset*>, "T must implement the IBaseAsset interface.");

      for (const StringRange& ext : exts)
      {
        m_FileExtReg.emplace(ext, create_fn);
      }
    }

    // Main API

    IBaseAsset* findAsset(const bfUUIDNumber& uuid) const
    {
      return m_AssetSet.find(uuid);
    }

    IBaseAsset* findAsset(AbsPath abs_path)
    {
      const StringRange rel_path = absPathToRelPath(abs_path.path);
      IBaseAsset*       result   = m_AssetSet.find(rel_path);

      if (!result)
      {
        result = loadAsset(abs_path.path);
      }

      return result;
    }

    IBaseAsset* findAsset(RelPath rel_path)
    {
      IBaseAsset* result = m_AssetSet.find(rel_path.path);

      if (!result)
      {
        const String abs_path = relPathToAbsPath(rel_path.path);

        result = loadAsset(abs_path);
      }

      return result;
    }

    IBaseAsset* loadAsset(const StringRange& abs_path);

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

      // If the found asset did not match the correct type.
      if (base_asset && base_asset->type() != meta::typeInfoGet<T>())
      {
        base_asset = nullptr;
      }

      return static_cast<T*>(base_asset);
    }

    // Old garbage

    template<typename AssetTInfo>
    AssetIndexResult<AssetTInfo> indexAsset(StringRange abs_path)
    {
      AssetIndexResult<AssetTInfo> result = {};
      BaseAssetInfo*               info   = nullptr;
      bfUUID                       uuid;

      std::tie(uuid, result.is_new, info) = indexAssetImpl(abs_path);

      if (result.is_new)
      {
        result.info = createAssetInfo<AssetTInfo>(abs_path, String_length(m_RootPath), uuid);

        saveAssetInfo(m_Engine, result.info);
      }
      else
      {
        result.info = static_cast<AssetTInfo*>(info);
      }

      return result;
    }

    template<typename AssetTInfo>
    AssetIndexResult<AssetTInfo> indexAsset(BaseAssetInfo* parent_asset, StringRange sub_asset_name_path)
    {
      AssetIndexResult<AssetTInfo> result = {};
      BaseAssetInfo* const         info   = findSubAssetFrom(parent_asset, sub_asset_name_path);

      result.is_new = info == nullptr;

      if (result.is_new)
      {
        result.info = createAssetInfo<AssetTInfo>(sub_asset_name_path, 0u, bfUUID_generate());

        addSubAssetTo(parent_asset, result.info);
        saveAssetInfo(m_Engine, parent_asset);
      }
      else
      {
        result.info = static_cast<AssetTInfo*>(info);
      }

      return result;
    }

    BaseAssetInfo*  findAssetInfo(const bfUUID& uuid);
    bool            tryAssignHandle(BaseAssetHandle& handle, BaseAssetInfo* info) const;
    BaseAssetHandle makeHandle(BaseAssetInfo& info) const;

    template<typename T>
    bool tryLoadAsset(BaseAssetHandle& handle, const StringRange& abs_path)
    {
      return tryAssignHandle(handle, indexAsset<T>(abs_path).info);
    }

    template<typename TAssetHandle>
    TAssetHandle makeHandleT(BaseAssetInfo& info) const
    {
      static_assert(std::is_base_of_v<BaseAssetHandle, TAssetHandle>, "The type specified must derive from BaseAssetHandle.");

      TAssetHandle handle = nullptr;
      tryAssignHandle(handle, &info);
      return handle;
    }

    char*      metaFileName(IMemoryManager& allocator, StringRange relative_path, std::size_t& out_string_length) const;  // Free the buffer with string_utils::fmtFree
    TempBuffer metaFullPath(IMemoryManager& allocator, StringRange meta_file_name) const;
    void       loadMeta(StringRange meta_file_name);
    AssetError setRootPath(std::string_view path);  // TODO(Shareef): Use 'StringRange'.
    void       setRootPath(std::nullptr_t);
    void       markDirty(const BaseAssetHandle& asset);
    bool       writeJsonToFile(const StringRange& path, const json::Value& value) const;
    void       saveAssets();
    void       saveAssetInfo(LinearAllocator& temp_alloc, IMemoryManager& temp_alloc_no_free, BaseAssetInfo* info);
    void       saveAssetInfo(Engine& engine, BaseAssetInfo* info);
    void       clearDirtyList();

    // Path Conversions

    String      relPathToAbsPath(const StringRange& rel_path) const;
    StringRange absPathToRelPath(const StringRange& abs_path) const;  // This function indexes into the passed in abs_path, so be careful about lifetimes.

    ~Assets();

    // TODO: Remove These
    detail::AssetMap& assetMap() { return m_AssetMap; }
    Engine&           engine() const { return m_Engine; }
    IMemoryManager&   memory() const { return m_Memory; }

   private:
    template<typename AssetTInfo>
    AssetTInfo* createAssetInfo(StringRange abs_path, std::size_t root_length, const bfUUID& uuid)
    {
      AssetTInfo* const asset_info = m_Memory.allocateT<AssetTInfo>(abs_path, root_length, uuid);
      asset_info->m_TypeInfo       = meta::TypeInfo<AssetTInfo>::get();

      m_AssetMap.emplace(uuid, asset_info);

      return asset_info;
    }

    std::tuple<bfUUID, bool, BaseAssetInfo*> indexAssetImpl(StringRange abs_path);
    static BaseAssetInfo*                    findSubAssetFrom(BaseAssetInfo* parent_asset, StringRange sub_asset_name_path);
    static void                              addSubAssetTo(BaseAssetInfo* parent_asset, BaseAssetInfo* child_asset);
    void                                     writeMetaInfo(JsonSerializerWriter& json_writer, BaseAssetInfo* info);
    BaseAssetInfo*                           readMetaInfo(JsonSerializerReader& reader, bool is_sub_asset = false);
  };
}  // namespace bf

#endif /* BF_ASSETS_HPP */
