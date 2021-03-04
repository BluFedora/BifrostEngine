/******************************************************************************/
/*!
 * @file   bf_asset_map.hpp
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
#ifndef BF_ASSET_MAP_HPP
#define BF_ASSET_MAP_HPP

#include "bf/StringRange.hpp"                         // StringRange
#include "bf/bf_non_copy_move.hpp"                    // NonCopyMoveable<T>
#include "bf/data_structures/bifrost_hash_table.hpp"  // HashTable<K, V>
#include "bf/utility/bifrost_uuid.hpp"                // bfUUIDNumber

namespace bf
{
  // Forward Declarations

  class IDocument;

  //
  // Although this is named `AssetMap` it is really a 'AssetSet'.
  // This class assumes you do not insert a repeated asset.
  // Also this class does not assume memory ownership over the `IBaseAsset` pointers.
  //
  class AssetMap final : private NonCopyMoveable<AssetMap>
  {
   public:
    static char s_TombstoneSentinel;  //!< Used to get a unique `IBaseAsset` pointer value so that Node does not need an extra member for marking as a tombstone.

   private:
    static constexpr std::size_t k_InitialCapacity = 128;

   private:
    // Typedefs for clarity of these type's semantics.
    using StringRangeInAsset = StringRange;
    using HashIndex          = std::size_t;
    using PathToIndex        = HashTable<StringRangeInAsset, HashIndex>;

    struct Node
    {
      IDocument* value = nullptr;

      bool is_active() const { return !is_inactive() && !is_tombstone(); }
      bool is_tombstone() const { return value == reinterpret_cast<IDocument*>(&s_TombstoneSentinel); }
      bool is_inactive() const { return value == nullptr; }
    };

    struct FindResult
    {
      IDocument* item;
      HashIndex  bucket_slot;
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
    explicit AssetMap(IMemoryManager& memory);

    ~AssetMap();

    bool       isEmpty() const;
    void       clear();
    void       insert(IDocument* key);
    IDocument* find(StringRange path) const;
    IDocument* find(const bfUUIDNumber& uuid) const;

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
            IDocument* const asset = m_Assets[i].value;

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

    bool remove(StringRange path);
    bool remove(const bfUUIDNumber& uuid);
    bool remove(const IDocument* key);

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

    void        removeAt(HashIndex bucket_slot);
    std::size_t findBucketFor(const bfUUIDNumber& uuid);
    void        reserveSpaceForNewElement();

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
