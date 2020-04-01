/******************************************************************************/
/*!
* @file   bifrost_freelist_allocator.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   This allocator is a the most generic custom allocator with the heaviest
*   overhead. This has the largest header size of any allocator but can be
*   used as a direct replacement for "malloc/new" and "free/delete".
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019-2020
*/
/******************************************************************************/
#ifndef BIFROST_FREELIST_ALLOCATOR_HPP
#define BIFROST_FREELIST_ALLOCATOR_HPP

#include "bifrost_imemory_manager.hpp" /* MemoryManager */

namespace bifrost
{
  class FreeListAllocator final : public MemoryManager
  {
   private:
    struct FreeListNode;
    FreeListNode* m_FreeList;
    std::size_t   m_UsedBytes;

   public:
    FreeListAllocator(char* memory_block, std::size_t memory_block_size);

    std::size_t usedMemory() const { return m_UsedBytes; }

   public:
    void* allocate(std::size_t size) override;
    void  deallocate(void* ptr) override;

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
      unsigned char* begin() const { return reinterpret_cast<unsigned char*>(const_cast<FreeListNode*>(this)); }
      unsigned char* end() const { return begin() + size + header_size; }
    };

    static_assert(sizeof(FreeListNode) == sizeof(AllocationHeader), "FreeListNode needs to the same size as AllocationHeader");

   public:
    static constexpr std::size_t header_size = sizeof(AllocationHeader);
  };
}  // namespace bifrost

#endif /* BIFROST_FREELIST_ALLOCATOR_HPP */
