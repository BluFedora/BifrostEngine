/******************************************************************************/
/*!
 * @file   bf_asset_map.cpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Provides a mapping of UUID and Paths to Documents for fast / easy lookup.
 *
 * @version 0.0.1
 * @date    2020-12-20
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#include "bf/asset_io/bf_asset_map.hpp"

#include "bf/asset_io/bf_document.hpp"  // IMemoryManager, IDocument

namespace bf
{
  char AssetMap::s_TombstoneSentinel = 0;

  AssetMap::AssetMap(IMemoryManager& memory) :
    m_PathToAssetIndex{},
    m_Assets(memory.allocateArray<Node>(k_InitialCapacity)),
    m_NumAssets(0),
    m_AssetsCapacity(k_InitialCapacity),
    m_NumAssetsMask(k_InitialCapacity - 1),
    m_MaxProbed{-1},
    m_Memory{memory}
  {
  }

  AssetMap::~AssetMap()
  {
    m_Memory.deallocateArray(m_Assets);
  }

  bool AssetMap::isEmpty() const
  {
    return m_NumAssets == 0;
  }

  void AssetMap::clear()
  {
    m_PathToAssetIndex.clear();
    std::fill_n(m_Assets, m_AssetsCapacity, Node());
    m_NumAssets = 0;
    m_MaxProbed = -1;
  }

  void AssetMap::insert(IDocument* key)
  {
    reserveSpaceForNewElement();

    const std::size_t dst_slot_idx = findBucketFor(key->uuid());

    m_Assets[dst_slot_idx].value            = key;
    m_PathToAssetIndex[key->relativePath()] = dst_slot_idx;

    ++m_NumAssets;
  }

  IDocument* AssetMap::find(StringRange path) const
  {
    const auto path_it = m_PathToAssetIndex.find(path);

    if (path_it != m_PathToAssetIndex.end())
    {
      return findImpl(
              path_it->value(),
              path,
              [](const StringRange key, IDocument* asset) {
                return key == asset->relativePath();
              })
       .item;
    }

    return nullptr;
  }

  IDocument* AssetMap::find(const bfUUIDNumber& uuid) const
  {
    return findImpl(
            hashUUID(uuid),
            uuid,
            [](const bfUUIDNumber& key, IDocument* asset) {
              return UUIDEqual()(key, asset->uuid());
            })
     .item;
  }

  bool AssetMap::remove(StringRange path)
  {
    const auto path_it = m_PathToAssetIndex.find(path);

    if (path_it != m_PathToAssetIndex.end())
    {
      return removeImpl(
       path_it->value(),
       path,
       [](const StringRange key, IDocument* asset) {
         return key == asset->relativePath();
       });
    }

    return false;
  }

  bool AssetMap::remove(const bfUUIDNumber& uuid)
  {
    return removeImpl(
     hashUUID(uuid),
     uuid,
     [](const bfUUIDNumber& key, IDocument* asset) {
       return UUIDEqual()(key, asset->uuid());
     });
  }

  bool AssetMap::remove(const IDocument* key)
  {
    return remove(key->uuid());
  }

  void AssetMap::removeAt(HashIndex bucket_slot)
  {
    const IDocument* const item = m_Assets[bucket_slot].value;

    m_PathToAssetIndex.remove(item->relativePath());
    m_Assets[bucket_slot].value = reinterpret_cast<IDocument*>(&s_TombstoneSentinel);
    --m_NumAssets;
  }

  std::size_t AssetMap::findBucketFor(const bfUUIDNumber& uuid)
  {
    const std::size_t base_hash = hashUUID(uuid);
    std::int32_t      offset    = 0;

    while (true)
    {
      const std::size_t slot = (base_hash + offset) & m_NumAssetsMask;

      if (!m_Assets[slot].is_active())
      {
        if (offset > m_MaxProbed)
        {
          m_MaxProbed = offset;
        }

        return slot;
      }

      ++offset;
    }
  }

  void AssetMap::reserveSpaceForNewElement()
  {
    const std::size_t requested_num_elements = (m_NumAssets + 1);
    const std::size_t required_size          = requested_num_elements + requested_num_elements / 2 + 1;

    // If we do not have the capacity to satisfy optimal number of free spaces.
    if (required_size > m_AssetsCapacity)
    {
      std::size_t required_size_po2 = 4;

      while (required_size_po2 < required_size)
      {
        required_size_po2 *= 2;
      }

      Node* const       old_assets   = m_Assets;
      const std::size_t old_capacity = m_AssetsCapacity;

      m_PathToAssetIndex.clear();
      m_Assets         = m_Memory.allocateArray<Node>(required_size_po2);
      m_NumAssets      = 0u;
      m_AssetsCapacity = required_size_po2;
      m_NumAssetsMask  = required_size_po2 - 1;

      for (std::size_t src_slot_idx = 0; src_slot_idx < old_capacity; ++src_slot_idx)
      {
        const Node& src_slot = old_assets[src_slot_idx];

        if (src_slot.is_active())
        {
          const std::size_t dst_slot_idx = findBucketFor(src_slot.value->uuid());

          m_Assets[dst_slot_idx]                             = src_slot;
          m_PathToAssetIndex[src_slot.value->relativePath()] = dst_slot_idx;
          ++m_NumAssets;
        }
      }

      m_Memory.deallocateArray(old_assets);
    }
  }
}  // namespace bf

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

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
