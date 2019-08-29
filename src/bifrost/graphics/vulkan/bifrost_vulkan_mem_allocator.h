#ifndef BIFROST_VULKAN_MEM_ALLOCATOR_H
#define BIFROST_VULKAN_MEM_ALLOCATOR_H

#include "bifrost/graphics/bifrost_gfx_api.h"

#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif
typedef struct Allocation_t
{
  VkDeviceMemory handle;
  uint32_t       type;
  uint32_t       index;
  uint64_t       size;  // This is the aligned size.
  uint64_t       offset;
  void*          mapped_ptr;

} Allocation;

typedef struct OffsetSize_t
{
  uint64_t offset;
  uint64_t size;

} OffsetSize;

typedef struct DeviceMemoryBlock_t
{
  Allocation  mem;
  OffsetSize* layout;
  bfBool32    isPageReserved;
  bfBool32    isPageMapped;
  void*       pageMapping;

} DeviceMemoryBlock;

typedef DeviceMemoryBlock* MemoryPool;

typedef struct PoolAllocator_t
{
  const bfGfxDevice* m_LogicalDevice;
  uint64_t           m_MinBlockSize;
  MemoryPool*        m_MemPools;
  uint32_t           m_PageSize;
  uint64_t*          m_MemTypeAllocSizes;
  uint32_t           m_NumAllocations;

} PoolAllocator;

void     VkPoolAllocatorCtor(PoolAllocator* self, const bfGfxDevice* const logical_device);
void     VkPoolAllocator_alloc(PoolAllocator*                self,
                               const bfAllocationCreateInfo* create_info,
                               const bfBool32                is_globally_mapped,
                               const uint32_t                mem_type,
                               Allocation*                   out);
void     VkPoolAllocator_free(PoolAllocator* self, const Allocation* allocation);
uint64_t VkPoolAllocator_allocationSize(const PoolAllocator* const self, const uint32_t mem_type);
uint32_t VkPoolAllocator_numAllocations(const PoolAllocator* const self);
void     VkPoolAllocatorDtor(PoolAllocator* self);
#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_MEM_ALLOCATOR_H */
