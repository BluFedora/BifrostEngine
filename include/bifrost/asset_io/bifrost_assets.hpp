/*!
 * @file    bifrost_assets.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Asset / Resource manager for this engine.
 *
 * @version 0.0.1
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_ASSETS_HPP
#define BIFROST_ASSETS_HPP

#include "bifrost/data_structures/bifrost_any.hpp"        /* Any                                          */
#include "bifrost/data_structures/bifrost_hash_table.hpp" /* HashTable<K, V>                              */
#include "bifrost/data_structures/bifrost_string.hpp"     /* BifrostStringHasher, BifrostStringComparator */
#include "bifrost/utility/bifrost_non_copy_move.hpp"      /* bfNonCopyMoveable<T>                         */
#include "bifrost/utility/bifrost_uuid.h"                 /* BifrostUUID                                  */

#include <string_view> /* string_view */

namespace bifrost
{
  class BaseAssetHandle;

  // TODO: Move to string.hpp???
  template<typename T, std::size_t initial_size = 128>
  using StringTable = HashTable<BifrostString, T, initial_size, string_utils::BifrostStringHasher, string_utils::BifrostStringComparator>;

  class Project final
  {
   private:
    BifrostString    m_Name;
    BifrostString    m_PathRoot;
    StringTable<Any> m_Options;

   public:
  };

  using PathToUUIDTable = StringTable<BifrostUUID, 64>;

  /*!
   * @brief
   *  All native asset types supported by the engine.
   */
  enum class AssetType : std::uint8_t
  {
    SHADER_MODULE,
    SHADER_PROGRAM,
    TEXTURE,
    MATERIAL,
    SPRITESHEET,
    AUDIO_SOURCE,
    BITMAP_FONT,
    BIFROST_SCRIPT,
    MODEL,

    ASSET_TYPE_MAX
  };

  /*!
   * @brief
   *  Basic abstraction over a path.
   *
   *  Glorified string with some extra utilities to
   *  make working with paths cross-platform and less
   *  painful.
   */
  class Path : public bfNonCopyMoveable<Path>
  {
   private:
    BifrostString m_Impl;  //!< The Glorified string.

   public:
    Path(std::string_view path) :
      m_Impl{String_newLen(path.data(), path.length())}
    {
    }
  };

  class Assets final : public bfNonCopyMoveable<Assets>
  {
   private:
    PathToUUIDTable                          m_NameToGUID;  //!< Allows loading assets from path rather than having to know the UUID.
    HashTable<BifrostUUID, BaseAssetHandle*> m_Asset;       //!< Owns the memory for the associated 'BaseAssetHandle*'.

   public:
    Assets();
  };
}  // namespace bifrost

#endif /* BIFROST_ASSETS_HPP */
