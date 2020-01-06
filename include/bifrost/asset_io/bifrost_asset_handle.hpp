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

#include "bifrost/data_structures/bifrost_dynamic_string.h" /* BifrostString */
#include "bifrost/utility/bifrost_uuid.h"                   /* BifrostUUID   */

#include <cstdint>     /* uint16_t    */
#include <string_view> /* string_view */

namespace bifrost
{
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

  class BaseAssetHandle
  {
   protected:
    BifrostString m_Path;      //!< A path relative to the project to the actual asset file.
    BifrostUUID   m_UUID;      //!< Uniquely identifies the asset.
    std::uint16_t m_RefCount;  //!< How many live references in the engine.
    AssetTagList  m_Tags;      //!< Tags associated with this asset.

   protected:
    BaseAssetHandle(std::string_view path) :
      m_Path{string_utils::fromStringview(path)},
      m_UUID{bfUUID_makeEmpty()},
      m_RefCount{0u},
      m_Tags{}
    {
    }
  };

  template<typename TData, typename Dtor>
  class AssetHandle : public BaseAssetHandle
  {
   protected:
    TData m_Data;
    Dtor  m_Dtor;  // TODO(Shareef): DO something about this taking up space in each asset handle.

   public:
    AssetHandle(const std::string_view& path, const TData& data) :
      BaseAssetHandle(path),
      m_Data(data),
      m_Dtor{}
    {
    }
  };
}  // namespace bifrost

#endif /* BIFROST_ASSET_HANDLE_HPP */
