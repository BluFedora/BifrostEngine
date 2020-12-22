/******************************************************************************/
/*!
 * @file   bf_base_asset.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Interface for creating asset type the engine can use.
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#include "bf/asset_io/bf_base_asset.hpp"

#include <cassert> /* assert */

#include "bf/asset_io/bf_asset_map.hpp"

namespace bf
{
  char AssetMap::s_TombstoneSentinel = 0;

  IBaseAsset::IBaseAsset() :
    m_UUID{},
    m_FilePathAbs{},
    m_FilePathRel{},
    m_RefCount{0},
    m_Flags{AssetFlags::DEFAULT}
  {
  }

  AssetStatus IBaseAsset::status() const
  {
    const std::uint16_t current_flags = m_Flags;

    if (m_RefCount == 0)
    {
      assert((m_Flags & AssetFlags::IS_LOADED) == 0 && "This flag should not be set if the ref count is 0.");

      if (current_flags & AssetFlags::FAILED_TO_LOAD)
      {
        return AssetStatus::FAILED;
      }

      return AssetStatus::UNLOADED;
    }

    // The Ref Count is NOT zero.

    if (current_flags & AssetFlags::IS_LOADED)
    {
      return AssetStatus::LOADED;
    }

    return AssetStatus::LOADING;
  }

  void IBaseAsset::acquire()
  {
    assert((m_Flags + 1) != 0 && "Unsigned overflow check, I suspect that we will never have more than 0xFFFF live references but just to be safe.");

    // Do not continuously try to load an asset that could not be loaded.
    if (!(m_Flags & AssetFlags::FAILED_TO_LOAD))
    {
      if (m_RefCount++ == 0)  // The ref count WAS zero.
      {
        assert((m_Flags & AssetFlags::IS_LOADED) == 0 && "If the ref count was zero then this asset should not have been loaded.");

        onLoad();
      }
    }
  }

  void IBaseAsset::reload()
  {
    onReload();
  }

  void IBaseAsset::release()
  {
    assert(m_RefCount > 0 && "To many release calls for the number of successful acquire calls.");

    if (--m_RefCount)  // This is the last use of this asset.
    {
      assert((m_Flags & AssetFlags::IS_LOADED) != 0 && "The asset should have been loaded if we are now unloading it.");

      onUnload();

      m_Flags &= ~(AssetFlags::IS_LOADED | AssetFlags::FAILED_TO_LOAD);
    }
  }

  void IBaseAsset::saveAssetContent(ISerializer& serializer)
  {
    onSaveAsset(serializer);
  }

  void IBaseAsset::saveAssetMeta(ISerializer& serializer)
  {
    onSaveMeta(serializer);
  }

  void IBaseAsset::setup(const String& full_path, std::size_t length_of_root_path, const bfUUIDNumber& uuid)
  {
    m_UUID        = uuid;
    m_FilePathAbs = full_path;
    m_FilePathRel = {
     m_FilePathAbs.begin() + (length_of_root_path ? length_of_root_path + 1u : 0u), // The plus one accounts for the '/'
     m_FilePathAbs.end(),
    };  
  }

  void IBaseAsset::onReload()
  {
    const AssetStatus current_status = status();

    if (current_status != AssetStatus::LOADING)
    {
      if (current_status == AssetStatus::LOADED)
      {
        onUnload();
      }

      onLoad();
    }
  }

  void IBaseAsset::onSaveAsset(ISerializer& serializer)
  {
    (void)serializer; /* NO-OP */
  }

  void IBaseAsset::onSaveMeta(ISerializer& serializer)
  {
    (void)serializer; /* NO-OP */
  }
}  // namespace bf

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
