/******************************************************************************/
/*!
 * @file   bf_asset_map.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Maps a path and UUID to IBaseAsset pointers.
 *
 * @version 0.0.1
 * @date    2020-12-20
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_ASSET_MAP_HPP
#define BF_ASSET_MAP_HPP

#include "bf/bf_non_copy_move.hpp"                    // NonCopyMoveable<T>
#include "bf/data_structures/bifrost_hash_table.hpp"  // HashTable
#include "bf/utility/bifrost_uuid.hpp"                // bfUUIDNumber
#include "bf_base_asset.hpp"                          // IBaseAsset, StringRange, IMemoryManager

namespace bf
{
  // Forward Declarations

  class IBaseAsset;

  //
  // Although this is named `AssetMap` it is really a 'AssetSet'.
  // This class assumes you do not insert a repeated asset.
  //
  class AssetMap : private NonCopyMoveable<AssetMap>
  {
   public:
    static char s_TombstoneSentinel;  //!< Used to get a unique `IBaseAsset` pointer value so that Node does not need an extra member like a bool.

   private:
    static constexpr std::size_t k_InitialCapacity = 128;

   private:
    // Typedefs for clarity of these type's semantics.
    using StringRangeInAsset = StringRange;
    using HashIndex          = std::size_t;
    using PathToIndex        = HashTable<StringRangeInAsset, HashIndex>;

    struct Node
    {
      IBaseAsset* value = nullptr;

      bool is_active() const { return !is_inactive() && !is_tombstone(); }
      bool is_tombstone() const { return value == reinterpret_cast<IBaseAsset*>(&s_TombstoneSentinel); }
      bool is_inactive() const { return value == nullptr; }
    };

    struct FindResult
    {
      IBaseAsset* item;
      HashIndex   bucket_slot;
    };

   private:
    PathToIndex     m_PathToAssetIndex;  //!< Makes it faster to go from a path string to IBaseAsset*.
    Node*           m_Assets;            //!< Ordered / hashed based on bfUUID.
    std::size_t     m_NumAssets;         //!< The number of assets in the hash map.
    std::size_t     m_AssetsCapacity;    //!< Must always be a power of two.
    std::size_t     m_NumAssetsMask;     //!< Always equal to `m_AssetsCapacity - 1`.
    std::int32_t    m_MaxProbed;         //!< The max number of slots probed, only -1 when this table is empty.
    IMemoryManager& m_Memory;            //!< Where to grab memory from.

   public:
    explicit AssetMap(IMemoryManager& memory) :
      m_PathToAssetIndex{},
      m_Assets(memory.allocateArray<Node>(k_InitialCapacity)),
      m_NumAssets(0),
      m_AssetsCapacity(k_InitialCapacity),
      m_NumAssetsMask(k_InitialCapacity - 1),
      m_MaxProbed{-1},
      m_Memory{memory}
    {
    }

    ~AssetMap()
    {
      m_Memory.deallocateArray(m_Assets);
    }

    bool isEmpty() const
    {
      return m_NumAssets == 0;
    }

    void clear()
    {
      m_PathToAssetIndex.clear();
      std::fill_n(m_Assets, m_AssetsCapacity, Node());
      m_NumAssets = 0;
      m_MaxProbed = -1;
    }

    void insert(IBaseAsset* key)
    {
      reserveSpaceForNewElement();

      const std::size_t dst_slot_idx = findBucketFor(key->uuid());

      m_Assets[dst_slot_idx].value            = key;
      m_PathToAssetIndex[key->relativePath()] = dst_slot_idx;

      ++m_NumAssets;
    }

    IBaseAsset* find(StringRange path) const
    {
      const auto path_it = m_PathToAssetIndex.find(path);

      if (path_it != m_PathToAssetIndex.end())
      {
        return findImpl(
                path_it->value(),
                path,
                [](const StringRange key, IBaseAsset* asset) {
                  return key == asset->relativePath();
                })
         .item;
      }

      return nullptr;
    }

    IBaseAsset* find(const bfUUIDNumber& uuid) const
    {
      return findImpl(
              hashUUID(uuid),
              uuid,
              [](const bfUUIDNumber& key, IBaseAsset* asset) {
                return UUIDEqual()(key, asset->uuid());
              })
       .item;
    }

    template<typename F>
    void forEach(F&& callback) const
    {
      std::size_t num_evaluated_nodes = 0;

      for (std::size_t i = 0; i < m_AssetsCapacity; ++i)
      {
        if (m_Assets[i].is_active())
        {
          callback(m_Assets[i].value);

          if (++num_evaluated_nodes == m_NumAssets)
          {
            break;
          }
        }
      }
    }

    template<typename F, typename Callback>
    bool removeIf(F&& predicate, Callback&& on_removal)
    {
      bool removed_any = false;

      if (!isEmpty())
      {
        std::size_t num_evaluated_nodes = 0;

        for (std::size_t i = 0; i < m_AssetsCapacity; ++i)
        {
          if (m_Assets[i].is_active())
          {
            IBaseAsset* const asset = m_Assets[i].value;

            if (predicate(asset))
            {
              removeAt(i);
              on_removal(asset);

              removed_any = true;
            }

            if (++num_evaluated_nodes == m_NumAssets)
            {
              break;
            }
          }
        }
      }

      return removed_any;
    }

    bool remove(StringRange path)
    {
      const auto path_it = m_PathToAssetIndex.find(path);

      if (path_it != m_PathToAssetIndex.end())
      {
        return removeImpl(
         path_it->value(),
         path,
         [](const StringRange key, IBaseAsset* asset) {
           return key == asset->relativePath();
         });
      }

      return false;
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    bool remove(const bfUUIDNumber& uuid)
    {
      return removeImpl(
       hashUUID(uuid),
       uuid,
       [](const bfUUIDNumber& key, IBaseAsset* asset) {
         return UUIDEqual()(key, asset->uuid());
       });
    }

    bool remove(const IBaseAsset* key)
    {
      return remove(key->uuid());
    }

   private:
    template<typename TKey, typename FCmp>
    bool removeImpl(HashIndex start_index, TKey&& key, FCmp&& cmp)
    {
      const FindResult find_result = findImpl(start_index, std::forward<TKey>(key), std::forward<FCmp>(cmp));

      if (find_result.item)
      {
        removeAt(find_result.bucket_slot);

        return true;
      }

      return false;
    }

    void removeAt(HashIndex bucket_slot)
    {
      const IBaseAsset* const item = m_Assets[bucket_slot].value;

      m_PathToAssetIndex.remove(item->relativePath());
      m_Assets[bucket_slot].value = reinterpret_cast<IBaseAsset*>(&s_TombstoneSentinel);
      --m_NumAssets;
    }

    std::size_t findBucketFor(const bfUUIDNumber& uuid)
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

    void reserveSpaceForNewElement()
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

    template<typename TKey, typename FCmp>
    FindResult findImpl(HashIndex start_index, TKey&& key, FCmp&& cmp) const
    {
      const HashIndex end_index = start_index + m_MaxProbed + 1;

      while (start_index < end_index)
      {
        const HashIndex actual_index = start_index & m_NumAssetsMask;

        if (m_Assets[actual_index].is_inactive())
        {
          break;
        }

        if (m_Assets[actual_index].is_active() && cmp(key, m_Assets[actual_index].value))
        {
          return {
           m_Assets[actual_index].value,
           actual_index,
          };
        }

        ++start_index;
      }

      return {nullptr, static_cast<HashIndex>(-1)};
    }

    static std::size_t hashUUID(const bfUUIDNumber& uuid)
    {
      return UUIDHasher()(uuid);
    }
  };

}  // namespace bf

#endif /* BF_ASSET_MAP_HPP */

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
