/******************************************************************************/
/*!
  @file   bifrost_stack_allocator.cpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is a designed for allocations wher you can guarantee
      deallocation is in a LIFO (Last in First Out) order.
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_stack_allocator.hpp"

#include <algorithm> /* max    */
#include <cassert>   /* assert */
#include <memory>    /* align  */

namespace bifrost
{
  StackAllocator::StackAllocator(char* const memory_block, const std::size_t memory_size) :
    MemoryManager(memory_block, memory_size),
    m_StackPtr(memory_block),
    m_MemoryLeft(memory_size)
  {
  }

  void* StackAllocator::alloc(const std::size_t size, const std::size_t alignment)
  {
    const auto        real_alignment   = std::max(alignof(StackHeader), alignment);
    const std::size_t requested_memory = (size + sizeof(StackHeader));
    std::size_t       block_size       = m_MemoryLeft;
    void*             memory_start     = m_StackPtr;

    if (requested_memory <= m_MemoryLeft && std::align(real_alignment, requested_memory, memory_start, block_size))
    {
      StackHeader* header = reinterpret_cast<StackHeader*>(memory_start);
      header->block_size  = requested_memory;
      header->align_size  = (m_MemoryLeft - block_size);

      const auto full_size = (header->block_size + header->align_size);

      m_MemoryLeft -= full_size;
      m_StackPtr += full_size;

      return reinterpret_cast<char*>(header) + sizeof(StackHeader);
    }

    // __debugbreak();
    return nullptr;
  }

  void StackAllocator::dealloc(void* ptr)
  {
    StackHeader* header      = reinterpret_cast<StackHeader*>(static_cast<char*>(ptr) - sizeof(StackHeader));
    const auto   full_size   = (header->block_size + header->align_size);
    auto         block_start = reinterpret_cast<char*>(header) - header->align_size;

    assert((block_start + full_size) == m_StackPtr &&
           "StackAllocator::dealloc : For this type of allocator you MUST deallocate in the reverse order of allocation.");

    this->m_StackPtr -= full_size;
    this->m_MemoryLeft += full_size;

#if DEBUG_MEMORY
    std::memset(block_start, DEBUG_MEMORY_SIGNATURE, full_size);
#endif
  }
}  // namespace tide
