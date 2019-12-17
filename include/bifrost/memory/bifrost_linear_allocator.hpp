/******************************************************************************/
/*!
  @file   tide_linear_allocator.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is very good for temporary memory allocations throughout
      the frame. There is no individual deallocation but a whole clear operation
      that should happen at the beginning (or end) of each frame.
*/
/******************************************************************************/
#ifndef TIDE_LINEAR_ALLOCATOR_HPP
#define TIDE_LINEAR_ALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp" /* MemoryManager */
#include <exception>                /* exception     */

namespace bifrost
{
  class LinearAllocatorScope;

  class linear_allocator_free : public std::exception
  {
    public:
      virtual const char* what() const noexcept(true);

  };

  class LinearAllocator : public MemoryManager
  {
      friend class LinearAllocatorScope;

    public:
      static constexpr std::size_t header_size = 0u;

    private:
      std::size_t m_MemoryOffset;

    public:
      LinearAllocator(char* memory_block, const std::size_t memory_block_size);

      inline size_t usedMemory() const { return m_MemoryOffset; }

      void  clear(void);
      void* alloc(const std::size_t size, const std::size_t alignment)  override;
      void  dealloc(void*)                                              override;

    private:
      char* currentBlock() const;

  };

  class LinearAllocatorScope
  {
    private:
      LinearAllocator& m_Allocator;
      std::size_t      m_OldOffset;

    public:
      LinearAllocatorScope(LinearAllocator& allocator);

      LinearAllocatorScope(const LinearAllocatorScope& rhs)            = delete;
      LinearAllocatorScope(LinearAllocatorScope&& rhs)                 = delete;
      LinearAllocatorScope& operator=(const LinearAllocatorScope& rhs) = delete;
      LinearAllocatorScope& operator=(LinearAllocatorScope&& rhs)      = delete;

      ~LinearAllocatorScope();

  };
}

#endif /* TIDE_LINEAR_ALLOCATOR_HPP */
