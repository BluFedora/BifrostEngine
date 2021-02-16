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

#include "bf/asset_io/bf_iserializer.hpp"
#include "bf/asset_io/bf_path_manip.hpp"
#include "bf/asset_io/bifrost_assets.hpp"

#include <cassert> /* assert */

namespace bf
{
  void AssetMetaInfo::serialize(IMemoryManager& allocator, ISerializer& serializer)
  {
    const bool is_loading = serializer.mode() == SerializerMode::LOADING;

    String name_as_str = {name.buffer, name.length};

    serializer.serialize("name", name_as_str);

    if (is_loading)
    {
      name = string_utils::clone(allocator, name_as_str);
    }

    serializer.serialize("version", version);
    serializer.serialize("uuid", uuid);

    std::size_t num_children;

    if (serializer.pushArray("sub-assets", num_children))
    {
      if (is_loading)
      {
        for (std::size_t i = 0; i < num_children; ++i)
        {
          if (serializer.pushObject(nullptr))
          {
            AssetMetaInfo* const new_child = allocator.allocateT<AssetMetaInfo>();

            addChild(new_child);

            new_child->serialize(allocator, serializer);

            serializer.popObject();
          }
        }
      }
      else
      {
        AssetMetaInfo* child_iterator = first_child;

        while (child_iterator)
        {
          if (serializer.pushObject(nullptr))
          {
            child_iterator->serialize(allocator, serializer);

            serializer.popObject();
          }

          child_iterator = child_iterator->next_sibling;
        }
      }

      serializer.popArray();
    }
  }

  void AssetMetaInfo::addChild(AssetMetaInfo* child)
  {
    if (!first_child)
    {
      first_child = child;
    }

    if (last_child)
    {
      last_child->next_sibling = child;
    }

    last_child = child;
  }

  void AssetMetaInfo::freeResources(IMemoryManager& allocator)
  {
    AssetMetaInfo* child_iterator = first_child;

    while (child_iterator)
    {
      AssetMetaInfo* const next_child = child_iterator->next_sibling;

      child_iterator->freeResources(allocator);
      allocator.deallocateT(child_iterator);

      child_iterator = next_child;
    }

    if (name.buffer)
    {
      allocator.deallocate(name.buffer, name.length + 1);

      name = {nullptr, 0u};
    }
  }

  IBaseAsset::IBaseAsset() :
    m_UUID{},
    m_FilePathAbs{},
    m_FilePathRel{},
    m_SubAssets{&IBaseAsset::m_SubAssetListNode},
    m_SubAssetListNode{},
    m_ParentAsset{nullptr},
    m_DirtyListNode{},
    m_RefCount{0},
    m_Flags{AssetFlags::DEFAULT},
    m_Assets{nullptr}
  {
  }

  IBaseAsset::~IBaseAsset()
  {
    if (m_ParentAsset)
    {
      m_ParentAsset->m_SubAssets.erase(*this);
    }

    for (IBaseAsset& sub_asset : m_SubAssets)
    {
      sub_asset.m_ParentAsset = nullptr;
    }

    m_SubAssets.clear();
  }

  StringRange IBaseAsset::name() const
  {
    return path::nameWithoutExtension(relativePath());
  }

  StringRange IBaseAsset::nameWithExt() const
  {
    return path::name(relativePath());
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
    assert((m_RefCount + 1) != 0 && "Unsigned overflow check, I suspect that we will never have more than 0xFFFF live references but just to be safe.");

    const std::uint16_t old_ref_count = m_RefCount++;

    // Do not continuously try to load an asset that could not be loaded.
    if (!(m_Flags & AssetFlags::FAILED_TO_LOAD))
    {
      if (old_ref_count == 0)  // The ref count WAS zero.
      {
        assert((m_Flags & AssetFlags::IS_LOADED) == 0 && "If the ref count was zero then this asset should not have been loaded.");

        if (isSubAsset())
        {
          m_ParentAsset->acquire();
          m_Flags |= AssetFlags::IS_LOADED;
        }
        else if (m_Flags & AssetFlags::IS_IN_MEMORY)
        {
          markIsLoaded();
        }
        else
        {
          onLoad();
        }
      }
    }
  }

  void IBaseAsset::reload()
  {
    if (isSubAsset())
    {
      m_ParentAsset->reload();
    }
    else
    {
      onReload();
    }
  }

  void IBaseAsset::release()
  {
    assert(m_RefCount > 0 && "To many release calls for the number of successful acquire calls.");

    if (--m_RefCount == 0)  // This is the last use of this asset.
    {
      assert((m_Flags & (AssetFlags::IS_LOADED | AssetFlags::FAILED_TO_LOAD)) != 0 && "The asset should have been loaded (or atleast attempted) if we are now unloading it.");

      if (isSubAsset())
      {
        m_ParentAsset->release();
      }
      else if (!(m_Flags & AssetFlags::IS_IN_MEMORY))
      {
        onUnload();

        // An unloaded non-sub asset implies non of the sub assets are still referenced.

        for (IBaseAsset& sub_asset : m_SubAssets)
        {
          sub_asset.onUnload();
        }
      }

      m_Flags &= ~(AssetFlags::IS_LOADED | AssetFlags::FAILED_TO_LOAD);

      if (m_Flags & AssetFlags::IS_FREE_ON_RELEASE)
      {
        IMemoryManager& asset_memory = assets().memory();

        asset_memory.deallocateT(this);
      }
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

  AssetMetaInfo* IBaseAsset::generateMetaInfo(IMemoryManager& allocator) const
  {
    AssetMetaInfo* const result      = allocator.allocateT<AssetMetaInfo>();
    const StringRange    result_name = nameWithExt();

    result->name = string_utils::clone(allocator, result_name);
    result->uuid = m_UUID;

    for (const IBaseAsset& sub_asset : m_SubAssets)
    {
      result->addChild(sub_asset.generateMetaInfo(allocator));
    }

    return result;
  }

  void IBaseAsset::setup(const String& full_path, std::size_t length_of_root_path, const bfUUIDNumber& uuid, Assets& assets)
  {
    m_UUID        = uuid;
    m_FilePathAbs = full_path;
    m_FilePathRel = {
     m_FilePathAbs.begin() + (length_of_root_path ? length_of_root_path + 1u : 0u),  // The plus one accounts for the '/'
     m_FilePathAbs.end(),
    };
    m_Assets = &assets;
  }

  void IBaseAsset::setup(const String& full_path, Assets& assets)
  {
    setup(full_path, 0u, bfUUID_makeEmpty().as_number, assets);
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
    reflect(serializer);
  }

  void IBaseAsset::onSaveMeta(ISerializer& serializer)
  {
    (void)serializer; /* NO-OP */
  }

  String IBaseAsset::createSubAssetPath(StringRange name_with_ext) const
  {
    char uuid_as_str[k_bfUUIDStringCapacity];
    bfUUID_numberToString(m_UUID.data, uuid_as_str);

    String result = path::k_SubAssetsRoot;
    result.append(StringRange{uuid_as_str, k_bfUUIDStringLength});

    result = path::append(result, name_with_ext);

    return result;
  }

  IBaseAsset* IBaseAsset::findOrCreateSubAsset(StringRange name_with_ext)
  {
    for (IBaseAsset& item : m_SubAssets)
    {
      if (item.nameWithExt() == name_with_ext)
      {
        return &item;
      }
    }

    const String sub_asset_path = createSubAssetPath(name_with_ext);

    IBaseAsset* const result = assets().createAssetFromPath(sub_asset_path);

    if (result)
    {
      result->m_Flags |= AssetFlags::IS_SUBASSET;
      result->m_ParentAsset = this;

      m_SubAssets.pushBack(*result);
    }

    return result;
  }

  IBaseAsset* IBaseAsset::findOrCreateSubAsset(StringRange name_with_ext, bfUUIDNumber uuid)
  {
    for (IBaseAsset& item : m_SubAssets)
    {
      if (item.nameWithExt() == name_with_ext)
      {
        return &item;
      }
    }

    const String sub_asset_path = createSubAssetPath(name_with_ext);

    IBaseAsset* const result = assets().createAssetFromPath(sub_asset_path, uuid);

    if (result)
    {
      result->m_Flags |= AssetFlags::IS_SUBASSET;
      result->m_ParentAsset = this;

      m_SubAssets.pushBack(*result);
    }

    return result;
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
