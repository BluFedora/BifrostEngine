/******************************************************************************/
/*!
* @file   bifrost_linear_allocator.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   This allocator is a designed for static (known at compile time)
*   pools of objects. Features O(1) allocation and O(1) deletion.
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019
*/
/******************************************************************************/
#include "bf/memory/bf_pool_allocator.hpp"

#include "bf/memory/bf_memory_utils.h" /* bfAlignUpSize */

#include <algorithm> /* max     */
#include <cassert>   /* assert  */
#include <cstdint>   /* uint8_t */

namespace bf
{
  PoolAllocatorImpl::PoolAllocatorImpl(char* memory_block, std::size_t memory_block_size, std::size_t sizeof_block, std::size_t alignof_block) :
    MemoryManager(memory_block, memory_block_size),
    m_PoolStart{nullptr},
    m_BlockSize{bfAlignUpSize(std::max(sizeof_block, sizeof(PoolHeader)), std::max(alignof_block, alignof(PoolHeader)))}
  {
    reset();
  }

  void* PoolAllocatorImpl::allocate(std::size_t size)
  {
    assert(size <= m_BlockSize && "This Allocator is made for Objects of a certain size!");

    PoolHeader* const header = m_PoolStart;

    if (header)
    {
      m_PoolStart = header->next;

      checkPointer(reinterpret_cast<void*>(header));

      return reinterpret_cast<void*>(header);
    }

    throw std::bad_alloc();
    return nullptr;
  }

  void PoolAllocatorImpl::deallocate(void* ptr, std::size_t num_bytes)
  {
    checkPointer(ptr);

    PoolHeader* const header = reinterpret_cast<PoolHeader*>(ptr);

    assert(num_bytes == m_BlockSize);

#ifdef BIFROST_MEMORY_DEBUG_WIPE_MEMORY
    std::memset(ptr, BIFROST_MEMORY_DEBUG_SIGNATURE, m_BlockSize);
#endif

    header->next = m_PoolStart;
    m_PoolStart  = header;
  }

  std::size_t PoolAllocatorImpl::indexOf(const void* ptr)
  {
    const auto index = (std::size_t(ptr) - std::size_t(begin())) / m_BlockSize;

    return index;
  }

  void* PoolAllocatorImpl::fromIndex(std::size_t index)
  {
    void* ptr = begin() + m_BlockSize * index;

    checkPointer(ptr);

    return ptr;
  }

  void PoolAllocatorImpl::reset()
  {
    m_PoolStart = reinterpret_cast<PoolHeader*>(begin());

    const std::size_t num_elements = capacity();
    PoolHeader*       header       = m_PoolStart;

    for (std::size_t i = 0; i < num_elements - 1; ++i)
    {
      header->next = reinterpret_cast<PoolHeader*>(reinterpret_cast<std::uint8_t*>(header) + m_BlockSize);
      header       = header->next;
    }

    header->next = nullptr;
  }
}  // namespace bifrost
