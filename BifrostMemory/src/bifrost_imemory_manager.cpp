/******************************************************************************/
/*!
* @file   bifrost_imemory_manager.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*  Outlines a basic interface for the various types of memory managers.
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_imemory_manager.hpp"

#include "bifrost/memory/bifrost_memory_utils.h" /* bfAlignUpPointer */

#include <cstdint>

namespace bifrost
{
#define bfCastByte(arr) ((unsigned char*)(arr))

  static std::uint8_t grabOffset(const void* self)
  {
    return static_cast<const std::uint8_t*>(self)[-1];
  }

  void* IMemoryManager::allocateAligned(std::size_t size, std::size_t alignment)
  {
    return allocateAligned(0, size, alignment);
  }

  void IMemoryManager::deallocateAligned(void* ptr)
  {
    deallocateAligned(0, ptr);
  }

  void* IMemoryManager::allocateAligned(std::size_t header_size, std::size_t size, std::size_t alignment)
  {
    if (size == 0)
    {
      return nullptr;
    }

    const std::size_t allocation_size = header_size + sizeof(std::uint8_t) + size + (alignment - 1);
    void* const       allocation      = allocate(allocation_size);

    if (allocation)
    {
      void*              data_start = bfAlignUpPointer(bfCastByte(allocation) + header_size, alignment);
      const std::uint8_t new_offset = std::uint8_t(std::uintptr_t(data_start) - std::uintptr_t(allocation) - header_size);

#if BIFROST_MEMORY_DEBUG_WIPE_MEMORY
      std::memset(
       static_cast<std::uint8_t*>(allocation) + header_size,
       BIFROST_MEMORY_DEBUG_SIGNATURE,
       new_offset);
#endif

      static_cast<std::uint8_t*>(data_start)[-1] = new_offset;

      return data_start;
    }

    return nullptr;
  }

  void* IMemoryManager::grabHeader(std::size_t header_size, void* ptr)
  {
    return bfCastByte(ptr) - grabOffset(ptr) - header_size;
  }

  void IMemoryManager::deallocateAligned(std::size_t header_size, void* ptr)
  {
    deallocate(grabHeader(header_size, ptr));
  }

  MemoryManager::MemoryManager(char* memory_block, const std::size_t memory_block_size) :
    IMemoryManager(),
    m_MemoryBlockBegin(memory_block),
    m_MemoryBlockEnd(memory_block + memory_block_size)
  {
#if BIFROST_MEMORY_DEBUG_WIPE_MEMORY
    std::memset(begin(), BIFROST_MEMORY_DEBUG_SIGNATURE, memory_block_size);
#endif
  }

  void MemoryManager::checkPointer(const void* ptr) const
  {
    if (ptr < begin() || ptr > end())
    {
      throw std::exception();
    }
  }
}  // namespace bifrost
