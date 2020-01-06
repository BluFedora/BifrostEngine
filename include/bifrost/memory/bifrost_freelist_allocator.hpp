/******************************************************************************/
/*!
  @file   bifrost_freelist_allocator.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      This allocator is a the most generic custom allocator with the heaviest
      overhead. This has the largest header size of any allocator but can be
      used as a direct replacement for "malloc/new" and "free/delete".
*/
/******************************************************************************/
#ifndef TIDE_FREE_LIST_ALLOCATOR_HPP
#define TIDE_FREE_LIST_ALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp" /* MemoryManager */

namespace bifrost
{
  class FreeListAllocator : public MemoryManager
  {
   private:
    struct FreeListNode;
    FreeListNode* m_FreeList;
    std::size_t   m_UsedBytes;

   public:
    FreeListAllocator(char* memory_block, const std::size_t memory_block_size);

    inline std::size_t usedMemory() const { return m_UsedBytes; }

   public:
    void* alloc(const std::size_t size, const std::size_t alignment) override;
    void  dealloc(void* ptr) override;

   private:
    struct AllocationHeader
    {
      std::size_t size;
      std::size_t PADD;
    };

    struct FreeListNode
    {
      FreeListNode* next;
      std::size_t   size;

     public:
      inline unsigned char* begin() const { return (unsigned char*)this; }
      inline unsigned char* end() const { return begin() + size + header_size; }
    };

    static_assert(sizeof(FreeListNode) == sizeof(AllocationHeader), "FreeListNode needs to the same size as AllocationHeader");

   public:
    static constexpr std::size_t header_size = sizeof(AllocationHeader);
  };
}  // namespace tide

#endif  /* TIDE_FREE_LIST_ALLOCATOR_HPP */
