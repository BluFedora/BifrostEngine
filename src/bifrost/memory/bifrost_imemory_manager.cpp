/******************************************************************************/
/*!
  @file   bifrost_imemory_manager.cpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      Implements a basic interface for the various types of memory managers.
*/
/******************************************************************************/
#include "bifrost/memory/bifrost_imemory_manager.hpp"

#include <cstdint>

namespace bifrost
{
#define bfCastByte(arr) ((unsigned char*)(arr))

  static void* alignUpPtr(uintptr_t element_ptr, uintptr_t element_alignment)
  {
    return reinterpret_cast<void*>(element_ptr + (element_alignment - 1) & ~(element_alignment - 1));
  }

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
    const std::size_t allocation_size = header_size + sizeof(std::uint8_t) + size + (alignment - 1);
    void* const       allocation      = allocate(allocation_size);

    if (allocation)
    {
      void* data_start = alignUpPtr(std::uintptr_t(allocation) + header_size, alignment);

      const std::uint8_t new_offset = std::uint8_t(std::uintptr_t(data_start) - std::uintptr_t(allocation) - header_size);

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
#if DEBUG_MEMORY
    //std::memset(begin(), DEBUG_MEMORY_SIGNATURE, memory_block_size);
#endif
  }
}  // namespace bifrost
