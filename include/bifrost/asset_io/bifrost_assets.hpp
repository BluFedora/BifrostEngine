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
#ifndef BIFROST_ASSETS_HPP
#define BIFROST_ASSETS_HPP

#include "bifrost/data_structures/bifrost_hash_table.hpp" /* HashTable<K, V>                              */
#include "bifrost/data_structures/bifrost_string.hpp"     /* BifrostStringHasher, BifrostStringComparator */
#include "bifrost/utility/bifrost_non_copy_move.hpp"      /* bfNonCopyMoveable<T>                         */
#include "bifrost/utility/bifrost_uuid.h"                 /* BifrostUUID                                  */

#include <string_view> /* string_view */

namespace bifrost
{
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
    struct DirectoryEntry;

    bool            doesExist(const char* path);
    bool            createDirectory(const char* path);
    DirectoryEntry* openDirectory(IMemoryManager& memory, const StringRange& path);
    bool            isDirectory(const DirectoryEntry* entry);
    bool            isFile(const DirectoryEntry* entry);
    const char*     entryFilename(const DirectoryEntry* entry);
    bool            readNextEntry(DirectoryEntry* entry);
    void            closeDirectory(DirectoryEntry* entry);
  }  // namespace path

  class Assets final : public bfNonCopyMoveable<Assets>
  {
   private:
    PathToUUIDTable                          m_NameToGUID;  //!< Allows loading assets from path rather than having to know the UUID.
    HashTable<BifrostUUID, BaseAssetHandle*> m_AssetMap;    //!< Owns the memory for the associated 'BaseAssetHandle*'.
    BifrostString                            m_RootPath;    //!< Base Path that all assets are relative to.

   public:
    Assets();

    AssetError setRootPath(std::string_view path);
    void       setRootPath(std::nullptr_t);

    ~Assets();
  };
}  // namespace bifrost

#endif /* BIFROST_ASSETS_HPP */
