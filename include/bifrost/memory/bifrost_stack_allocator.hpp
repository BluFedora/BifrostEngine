/******************************************************************************/
/*!
  @file   tide_stack_allocator.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is a designed for allocations where you can guarantee
      deallocation is in a LIFO (Last in First Out) order.
*/
/******************************************************************************/
#ifndef TIDE_STACK_ALLOCATOR_HPP
#define TIDE_STACK_ALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp"  /* MemoryManager    */

#include <cstdint>              /* size_t, uint32_t */

namespace bifrost
{
  class StackAllocator : public MemoryManager
  {
    private:
      char*       m_StackPtr;
      std::size_t m_MemoryLeft;

    public:
      StackAllocator(char * const memory_block, const std::size_t memory_size);

      inline size_t usedMemory() const { return size() - m_MemoryLeft; }

    public:
      void* alloc(const std::size_t size, const std::size_t alignment)  override;
      void  dealloc(void* ptr)                                          override;

    private:
      class StackHeader
      {
        public:
          std::size_t block_size;
          std::size_t align_size;

      };

    public:
      static constexpr std::size_t header_size = sizeof(StackHeader);

  };
}

#endif // TIDE_STACK_ALLOCATOR_HPP
