/******************************************************************************/
/*!
* @file   bifrost_linear_allocator.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   This allocator is very good for temporary memory allocations throughout
*   the frame. There is no individual deallocation but a whole clear operation
*   that should happen at the beginning (or end) of each frame.
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019-2020
*/
/******************************************************************************/
#ifndef BIFROST_LINEAR_ALLOCATOR_HPP
#define BIFROST_LINEAR_ALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp" /* MemoryManager */

#include <exception> /* exception */

namespace bf
{
  class LinearAllocatorScope;

  class linear_allocator_free : public std::exception
  {
   public:
    const char* what() const noexcept(true) override;
  };

  class LinearAllocator final : public MemoryManager
  {
    friend class LinearAllocatorScope;

   public:
    static constexpr std::size_t header_size = 0u;

   private:
    std::size_t m_MemoryOffset;

   public:
    LinearAllocator(char* memory_block, std::size_t memory_block_size);

    size_t usedMemory() const { return m_MemoryOffset; }

    void  clear(void);
    void* allocate(std::size_t size) override;
    void  deallocate(void* ptr) override;

   private:
    char* currentBlock() const;
  };

  class LinearAllocatorScope final
  {
   private:
    LinearAllocator& m_Allocator;
    std::size_t      m_OldOffset;

   public:
    LinearAllocatorScope(LinearAllocator& allocator);

    LinearAllocatorScope(const LinearAllocatorScope& rhs) = delete;
    LinearAllocatorScope(LinearAllocatorScope&& rhs)      = delete;
    LinearAllocatorScope& operator=(const LinearAllocatorScope& rhs) = delete;
    LinearAllocatorScope& operator=(LinearAllocatorScope&& rhs) = delete;

    ~LinearAllocatorScope();
  };
}  // namespace bifrost

#endif /* BIFROST_LINEAR_ALLOCATOR_HPP */
