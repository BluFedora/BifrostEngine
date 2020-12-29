#ifndef BIFROST_VULKAN_MEM_ALLOCATOR_H
#define BIFROST_VULKAN_MEM_ALLOCATOR_H

// References:
//   [http://kylehalladay.com/blog/tutorial/2017/12/13/Custom-Allocators-Vulkan.html]

#include "bf/bf_gfx_api.h"

#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif
typedef struct
{
  VkDeviceMemory handle;
  uint32_t       type;
  uint32_t       index;
  uint64_t       size;  // This is the aligned size.
  uint64_t       offset;
  void*          mapped_ptr;

} Allocation;

typedef struct
{
  uint64_t offset;
  uint64_t size;

} OffsetSize;

typedef struct
{
  Allocation  mem;
  OffsetSize* layout;
  bfBool32    isPageReserved;
  bfBool32    isPageMapped;
  void*       pageMapping;

} DeviceMemoryBlock;

typedef DeviceMemoryBlock* MemoryPool;

typedef struct
{
  const bfGfxDevice* m_LogicalDevice;
  uint64_t           m_MinBlockSize;
  MemoryPool*        m_MemPools;
  uint32_t           m_PageSize;
  uint64_t*          m_MemTypeAllocSizes;
  uint32_t           m_NumAllocations;

} PoolAllocator;

BF_DEFINE_GFX_HANDLE(Buffer)
{
  bfBaseGfxObject super;
  PoolAllocator*  alloc_pool;
  VkBuffer        handle;
  Allocation      alloc_info;  // This has the aligned size.
  bfBufferSize    real_size;
};

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
