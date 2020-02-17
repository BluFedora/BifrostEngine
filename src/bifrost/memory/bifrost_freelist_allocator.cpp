/******************************************************************************/
/*!
  @file   bifrost_freelist_allocator.cpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is a the most generic custom allocator with the heaviest
      overhead. This has the largest header size of any allocator but can be
      used as a direct replacement for "malloc/new" and "free/delete".
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_freelist_allocator.hpp"

#include <memory> /* memset */

#define cast(o, T) reinterpret_cast<T>((o))

namespace bifrost
{
  FreeListAllocator::FreeListAllocator(char* memory_block, const std::size_t memory_block_size) :
    MemoryManager(memory_block, memory_block_size),
    m_FreeList(cast(memory_block, FreeListNode*)),
    m_UsedBytes(0)
  {
    this->m_FreeList->next = nullptr;
    this->m_FreeList->size = memory_block_size - FreeListAllocator::header_size;
  }

  void* FreeListAllocator::allocate(std::size_t size)
  {
    FreeListNode* curr_node = this->m_FreeList;
    FreeListNode* prev_node = nullptr;

    while (curr_node)
    {
      const std::size_t potential_space = (size + header_size);

      if (curr_node->size <= potential_space)
      {
        prev_node = curr_node;
        curr_node = curr_node->next;
        continue;
      }

      void* data = cast(curr_node, void*);

      const std::size_t free_space = curr_node->size;

      const std::size_t moved_fwd      = (curr_node->size - free_space);
      const std::size_t new_block_size = header_size + size;

      FreeListNode* new_node = cast(reinterpret_cast<char*>(data) + new_block_size, FreeListNode*);
      new_node->size         = curr_node->size - new_block_size - moved_fwd;
      new_node->next         = curr_node->next;

      if (prev_node)
      {
        prev_node->next = new_node;
      }
      else
      {
        this->m_FreeList = new_node;
      }

      AllocationHeader* header = reinterpret_cast<AllocationHeader*>(data);

      header->size = size;

      m_UsedBytes += size + moved_fwd;
      return reinterpret_cast<char*>(data) + header_size;
    }

    return nullptr;
  }

  void FreeListAllocator::deallocate(void* ptr)
  {
    AllocationHeader* const header = reinterpret_cast<AllocationHeader*>(reinterpret_cast<char*>(ptr) - header_size);
    FreeListNode* const     node   = reinterpret_cast<FreeListNode*>(reinterpret_cast<char*>(header));

    const std::size_t block_size = header->size;

#if DEBUG_MEMORY
    std::memset(ptr, DEBUG_MEMORY_SIGNATURE, block_size);
#endif

    node->size = block_size;

    m_UsedBytes -= node->size;

    FreeListNode* current    = m_FreeList;
    FreeListNode* previous   = nullptr;
    void* const   node_begin = node->begin();
    void* const   node_end   = node->end();

    while (current)
    {
      if (current->begin() >= node_end)
      {
        break;
      }

      if (previous)
      {
        if (previous->end() == node_begin)
        {
          break;
        }
      }

      previous = current;
      current  = current->next;
    }

    if (current && current->begin() == node_end)
    {
      if (previous)
        previous->next = node;
      else
        m_FreeList = node;

      node->size += (current->size + header_size);
      node->next = current->next;
    }
    else if (previous)
    {
      if (previous->end() == node_begin)
      {
        previous->size += (node->size + header_size);
      }
      else
      {
        node->next     = previous->next;
        previous->next = node;
      }
    }
    else
    {
      node->next = m_FreeList;
      m_FreeList = node;
    }
  }
}  // namespace bifrost

#undef cast
