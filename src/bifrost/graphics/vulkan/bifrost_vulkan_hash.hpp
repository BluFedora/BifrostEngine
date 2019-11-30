#ifndef BIFROST_VULKAN_HASH_HPP
#define BIFROST_VULKAN_HASH_HPP

#include "bifrost/graphics/bifrost_gfx_api.h"        /*                       */
#include "bifrost/utility/bifrost_non_copy_move.hpp" /* bfNonCopyMoveable     */
#include <cassert>                                   /* assert                */
#include <cstddef>                                   /* size_t                */
#include <cstdint>                                   /* uint64_t              */
#include <cstring>                                   /* memset                */
#include <type_traits>                               /* is_trivially_copyable */

namespace bifrost
{
  namespace vk
  {
    //
    // Non owning data structure. Just used for managing a hash-based cache of objects.
    //
    template<typename T>
    class ObjectHashCache final : private bfNonCopyMoveable<ObjectHashCache<T>>
    {
      struct Node final
      {
        T*            value = nullptr;
        std::uint64_t hash_code = 0x0;
      };

     private:
      Node*       m_Nodes;
      std::size_t m_NodesSize;
      std::size_t m_MaxLoad;

     public:
      explicit ObjectHashCache(std::size_t initial_size = 32) :
        m_Nodes{new Node[initial_size]()},
        m_NodesSize{initial_size},
        m_MaxLoad{3}
      {
        assert(initial_size && !(initial_size & initial_size - 1) && "Initial size of a ObjectHashCache must be a non 0 power of two.");
      }

      // This method will update an old slot
      // with the new value if a collision occurs.
      void insert(std::uint64_t key, T* value)
      {
        if (!internalInsert(key, value))
        {
          grow();
          insert(key, value);
        }
      }

      [[nodiscard]] T* find(std::uint64_t key) const
      {
        const std::size_t hash_mask = m_NodesSize - 1;
        std::size_t       idx       = key & hash_mask;

        for (std::size_t i = 0; i < m_MaxLoad; ++i)
        {
          const auto& node = m_Nodes[idx];

          if (node.value && node.hash_code == key)
          {
            return node.value;
          }

          idx = idx + 1 & hash_mask;
        }

        return nullptr;
      }

      bool remove(std::uint64_t key)
      {
        const std::size_t hash_mask = m_NodesSize - 1;
        std::size_t       idx       = key & hash_mask;

        for (std::size_t i = 0; i < m_MaxLoad; ++i)
        {
          auto& node = m_Nodes[idx];

          if (node.value && node.hash_code == key)
          {
            node.value     = nullptr;
            node.hash_code = key;
            return true;
          }

          idx = idx + 1 & hash_mask;
        }

        return false;
      }

      void clear()
      {
        static_assert(std::is_trivially_copyable<Node>::value, "To memset nodes I nee to make sure they are trivial types.");

        if (m_Nodes)
        {
          std::memset(m_Nodes, 0x0, sizeof(Node) * m_NodesSize);
        }
      }

      ~ObjectHashCache()
      {
        delete[] m_Nodes;
      }

     private:
      bool internalInsert(std::uint64_t key, T* value)
      {
        assert(value && "nullptr is used as an indicator of an empty slot.");

        const std::size_t hash_mask = m_NodesSize - 1;
        std::size_t       idx       = key & hash_mask;

        for (std::size_t i = 0; i < m_MaxLoad; ++i)
        {
          auto& node = m_Nodes[idx];

          if (!node.value || node.hash_code == key)
          {
            node.value     = value;
            node.hash_code = key;
            return true;
          }

          idx = idx + 1 & hash_mask;
        }

        return false;
      }

      void grow()
      {
        const std::size_t old_size  = m_NodesSize;
        Node* const       old_nodes = m_Nodes;
        bool              was_success;

        do
        {
          const std::size_t new_size  = m_NodesSize * 2;
          Node* const       new_nodes = new Node[new_size]();
          m_Nodes                     = new_nodes;
          m_NodesSize                 = new_size;
          ++m_MaxLoad;

          was_success = true;
          for (std::size_t i = 0; i < old_size; ++i)
          {
            const Node& old_node = old_nodes[i];

            if (old_node.value && !internalInsert(old_node.hash_code, old_node.value))
            {
              was_success = false;
              break;
            }
          }

          if (!was_success)
          {
            delete[] new_nodes;
          }

        } while (!was_success);

        delete[] old_nodes;
      }
    };

    std::uint64_t hash(std::uint64_t self, const bfPipelineCache* pipeline);
    std::uint64_t hash(std::uint64_t self, bfTextureHandle* attachments, std::size_t num_attachments);
    std::uint64_t hash(std::uint64_t self, const bfRenderpassInfo* renderpass_info);
    std::uint64_t hash(std::uint64_t self, const bfAttachmentInfo* attachment_info);
    std::uint64_t hash(std::uint64_t self, const bfSubpassCache* subpass_info);
    std::uint64_t hash(std::uint64_t self, const bfAttachmentRefCache* attachment_ref_info);
  }  // namespace vk
}  // namespace bifrost

#endif /* BIFROST_VULKAN_HASH_HPP */
