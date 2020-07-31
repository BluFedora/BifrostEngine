/******************************************************************************/
/*!
* @file   bifrost_linear_allocator.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   This allocator is very good for temporary memory allocations throughout
*   the frame. There is no individual deallocation but a whole clear operation
*   that should happen at the beginning (or end) of each frame.
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_linear_allocator.hpp"

// #include <new>    /* bad_alloc  */

namespace bifrost
{
  const char* linear_allocator_free::what() const noexcept
  {
    return "LinearAllocator::dealloc was called but that may never happen all data must be free at once with LinearAllocator::clear.";
  }

  LinearAllocator::LinearAllocator(char* memory_block, const std::size_t memory_block_size) :
    MemoryManager(memory_block, memory_block_size),
    m_MemoryOffset(0)
  {
  }

  void LinearAllocator::clear()
  {
#ifdef BIFROST_MEMORY_DEBUG_WIPE_MEMORY
    if (m_MemoryOffset)
    {
      std::memset(begin(), BIFROST_MEMORY_DEBUG_SIGNATURE, m_MemoryOffset);
    }
#endif
    m_MemoryOffset = 0;
  }

  void* LinearAllocator::allocate(const std::size_t size)
  {
    if (m_MemoryOffset < this->size())
    {
      void* ptr = currentBlock();

      m_MemoryOffset += size;
      return ptr;
    }

     throw std::bad_alloc();
    return nullptr;
  }

  void LinearAllocator::deallocate(void*)
  {
    throw linear_allocator_free();
  }

  char* LinearAllocator::currentBlock() const
  {
    return begin() + m_MemoryOffset;
  }

  LinearAllocatorScope::LinearAllocatorScope(LinearAllocator& allocator) :
    m_Allocator(allocator),
    m_OldOffset(allocator.m_MemoryOffset)
  {
  }

  LinearAllocatorScope::~LinearAllocatorScope()
  {
    m_Allocator.m_MemoryOffset = m_OldOffset;
  }
}  // namespace bifrost
