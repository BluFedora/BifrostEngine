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
 * @copyright Copyright (c) 2019
 */
/******************************************************************************/
#ifndef BIFROST_ASSETS_HPP
#define BIFROST_ASSETS_HPP

#include "bifrost/data_structures/bifrost_hash_table.hpp" /* HashTable<K, V>                              */
#include "bifrost/data_structures/bifrost_string.hpp"     /* BifrostStringHasher, BifrostStringComparator */
#include "bifrost/meta/bifrost_meta_runtime_impl.hpp"     /* BaseClassMetaInfo, TypeInfo                  */
#include "bifrost/utility/bifrost_non_copy_move.hpp"      /* bfNonCopyMoveable<T>                         */
#include "bifrost/utility/bifrost_uuid.h"                 /* BifrostUUID                                  */

class BifrostEngine;

namespace bifrost
{
  using Engine = ::BifrostEngine;

  class BaseAssetInfo;
  class BaseAssetHandle;

  using PathToUUIDTable = HashTable<String, BifrostUUID, 64>;

  enum class AssetError : std::uint8_t
  {
    NONE,
    UNKNOWN_STL_ERROR,
    PATH_DOES_NOT_EXIST,
  };

  /*!
   * @brief
   *  Basic abstraction over a path.
   *
   *  Glorified string utilities with some extra utilities to
   *  make working with paths cross-platform and less painful.
   */
  namespace path
  {
    static constexpr std::size_t MAX_LENGTH = 512;

    struct DirectoryEntry;

    bool            doesExist(const char* path);
    bool            createDirectory(const char* path);
    bool            deleteDirectory(const char* path);
    DirectoryEntry* openDirectory(IMemoryManager& memory, const StringRange& path);
    bool            isDirectory(const DirectoryEntry* entry);
    bool            isFile(const DirectoryEntry* entry);
    const char*     entryFilename(const DirectoryEntry* entry);
    bool            readNextEntry(DirectoryEntry* entry);
    void            closeDirectory(DirectoryEntry* entry);
  }  // namespace path

  namespace detail
  {
    struct UUIDHasher final
    {
      std::size_t operator()(const BifrostUUID& uuid) const
      {
        // ReSharper disable CppUnreachableCode
        if constexpr (sizeof(std::size_t) == 4)
        {
          return bfString_hashN(uuid.as_number, sizeof(uuid.as_number));
        }
        else
        {
          return bfString_hashN64(uuid.as_number, sizeof(uuid.as_number));
        }
        // ReSharper restore CppUnreachableCode
      }
    };

    struct UUIDEqual final
    {
      bool operator()(const BifrostUUID& lhs, const BifrostUUID& rhs) const
      {
        return bfUUID_isEqual(&lhs, &rhs) != 0;
      }
    };

    using AssetMap = HashTable<BifrostUUID, BaseAssetInfo*, 64, UUIDHasher, UUIDEqual>;
  }  // namespace detail

  class Assets final : public bfNonCopyMoveable<Assets>
  {
   public:
    inline static const char META_PATH_NAME[] = "_meta";

   public:
    static bool isHandleCompatible(const BaseAssetHandle& handle, const BaseAssetInfo* info);

   private:
    Engine&          m_Engine;      //!< The engine this asset system is attached to.
    IMemoryManager&  m_Memory;      //!< Where to grab memory for the asset info.
    PathToUUIDTable  m_NameToGUID;  //!< Allows loading assets from path rather than having to know the UUID.
    detail::AssetMap m_AssetMap;    //!< Owns the memory for the associated 'BaseAssetHandle*'.
    BifrostString    m_RootPath;    //!< Base Path that all assets are relative to.
    String           m_MetaPath;    //!< Path that all assets meta files located in.

   public:
    explicit Assets(Engine& engine, IMemoryManager& memory);

    template<typename T>
    BifrostUUID indexAsset(StringRange relative_path, StringRange meta_file_name)
    {
      bool              create_new;
      const BifrostUUID uuid = indexAssetImpl(relative_path, meta_file_name, create_new, meta::TypeInfo<T>::get());

      if (create_new)
      {
        T* const asset_handle = m_Memory.allocateT<T>(relative_path, uuid);
        m_AssetMap.emplace(uuid, asset_handle);
      }

      return uuid;
    }

    BaseAssetInfo* findAssetInfo(const BifrostUUID& uuid);
    bool           tryAssignHandle(BaseAssetHandle& handle, BaseAssetInfo* info) const;
    String         fullPath(const BaseAssetInfo& info) const;
    void           loadMeta(StringRange meta_file_name);
    AssetError     setRootPath(std::string_view path);  // TODO(Shareef): Use 'StringRange'.
    void           setRootPath(std::nullptr_t);

    ~Assets();

    // TODO: Remove this
    detail::AssetMap& assetMap() { return m_AssetMap; }

   private:
    BifrostUUID indexAssetImpl(StringRange path, StringRange meta_file_name, bool& create_new, meta::BaseClassMetaInfo* type_info);  // Relative Path to Asset in reference to the Root path. Ex: "Textures/whatever.png"
  };
}  // namespace bifrost

#endif /* BIFROST_ASSETS_HPP */
