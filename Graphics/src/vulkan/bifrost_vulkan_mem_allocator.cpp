#include "bifrost_vulkan_mem_allocator.h"

#include "bifrost_vulkan_logical_device.h"

#include "bf/data_structures/bifrost_array_t.h"

#include <cassert> /* assert */

#define BIFROST_POOL_ALLOC_NUM_PAGES_PER_BLOCK 10
#define CUSTOM_ALLOCATOR NULL

typedef struct BlockSpanIndexPair_t
{
  uint32_t blockIdx;
  uint32_t spanIdx;

} BlockSpanIndexPair;

static void DeviceMemoryBlockCtor(DeviceMemoryBlock* const self)
{
  self->layout         = bfArray_new(OffsetSize, CUSTOM_ALLOCATOR, nullptr);
  self->mem.handle     = VK_NULL_HANDLE;
  self->isPageMapped   = bfFalse;
  self->isPageReserved = bfFalse;
  self->pageMapping    = NULL;
}

static void DeviceMemoryBlockDtor(VkDevice device_handle, DeviceMemoryBlock* const self)
{
  if (self->mem.handle)
  {
    if (self->isPageMapped)
    {
      vkUnmapMemory(device_handle, self->mem.handle);
      self->isPageMapped = bfFalse;
    }

    vkFreeMemory(device_handle, self->mem.handle, CUSTOM_ALLOCATOR);
  }
  bfArray_delete((void**)&self->layout);
}

static void MemPoolDtor(VkDevice device_handle, MemoryPool* self)
{
  const size_t num_blocks = bfArray_size((void**)self);

  for (uint32_t j = 0; j < num_blocks; ++j)
  {
    DeviceMemoryBlockDtor(device_handle, (DeviceMemoryBlock*)bfArray_at((void**)self, j));
  }

  bfArray_delete((void**)self);
}

void VkPoolAllocatorCtor(PoolAllocator* self, const bfGfxDevice* const logical_device)
{
  const VulkanPhysicalDevice* const device = logical_device->parent;

  const uint32_t memory_properties_type_count = device->memory_properties.memoryTypeCount;

  self->m_MemTypeAllocSizes = bfArray_newA(self->m_MemTypeAllocSizes, CUSTOM_ALLOCATOR, nullptr);
  self->m_MemPools          = bfArray_newA(self->m_MemPools, CUSTOM_ALLOCATOR, nullptr);
  self->m_PageSize          = (uint32_t)device->device_properties.limits.bufferImageGranularity;
  self->m_MinBlockSize      = (uint64_t)self->m_PageSize * BIFROST_POOL_ALLOC_NUM_PAGES_PER_BLOCK;
  self->m_LogicalDevice     = logical_device;

  bfArray_reserve((void**)&self->m_MemTypeAllocSizes, memory_properties_type_count);
  bfArray_reserve((void**)&self->m_MemPools, memory_properties_type_count);

  for (uint32_t i = 0; i < memory_properties_type_count; ++i)
  {
    static const size_t initial_pool_size = 5;

    MemoryPool* const pool = (MemoryPool*)bfArray_emplace((void**)&self->m_MemPools);
    size_t* const     size = (size_t*)bfArray_emplace((void**)&self->m_MemTypeAllocSizes);

    *pool = bfArray_newA(*pool, CUSTOM_ALLOCATOR, nullptr);

    bfArray_reserve((void**)pool, initial_pool_size);

    for (uint32_t j = 0; j < initial_pool_size; ++j)
    {
      DeviceMemoryBlockCtor((DeviceMemoryBlock*)bfArray_emplace((void**)pool));
    }

    *size = 0u;
  }
}

// Aligns size to be a multiple of page_size
static VkDeviceSize align_to(VkDeviceSize size, VkDeviceSize page_size);
static bfBool32     find_free_check_for_alloc(BlockSpanIndexPair* const loc, MemoryPool* const mem_pool, VkDeviceSize real_size, bfBool32 needs_new_page);
static size_t       add_block_to_pool(PoolAllocator* self, uint32_t mem_type, VkDeviceSize size);
static void         update_chunk(MemoryPool* const pool, const BlockSpanIndexPair* indices, VkDeviceSize size);

void VkPoolAllocator_alloc(PoolAllocator* self, const bfAllocationCreateInfo* create_info, const bfBool32 is_globally_mapped, const uint32_t mem_type, Allocation* out)
{
  const bfBool32     needs_own_page = create_info->properties != BIFROST_BPF_DEVICE_LOCAL || !is_globally_mapped;
  const VkDeviceSize size           = create_info->size;
  const VkDeviceSize real_size      = align_to(size, self->m_PageSize);
  MemoryPool* const  pool           = (MemoryPool*)bfArray_at((void**)&self->m_MemPools, mem_type);
  BlockSpanIndexPair loc            = {0, 0};
  const bfBool32     found          = find_free_check_for_alloc(&loc, pool, real_size, needs_own_page);
  uint64_t* const    mem_alloc_size = (uint64_t*)bfArray_at((void**)&self->m_MemTypeAllocSizes, mem_type);

  *mem_alloc_size += real_size;

  if (!found)
  {
    loc.blockIdx = (uint32_t)add_block_to_pool(self, mem_type, real_size);
    loc.spanIdx  = 0;
  }

  DeviceMemoryBlock* const block = (DeviceMemoryBlock*)bfArray_at((void**)pool, loc.blockIdx);

  block->isPageReserved = needs_own_page;

  if (is_globally_mapped && !block->isPageMapped)
  {
    VkResult err = vkMapMemory(self->m_LogicalDevice->handle,
                               block->mem.handle,
                               0,
                               BIFROST_BUFFER_WHOLE_SIZE,
                               0,
                               &block->pageMapping);

    assert(err != VK_ERROR_MEMORY_MAP_FAILED && "There was not a region of host mappable memory available.");
    assert(err == VK_SUCCESS && "Some unexplained error occurred");

    block->isPageMapped = bfTrue;
  }

  out->handle     = block->mem.handle;
  out->size       = size;
  out->offset     = ((OffsetSize*)bfArray_at((void**)&block->layout, loc.spanIdx))->offset;
  out->type       = mem_type;
  out->index      = loc.blockIdx;
  out->mapped_ptr = (char*)block->pageMapping + out->offset;

  update_chunk(pool, &loc, real_size);
}

void VkPoolAllocator_free(PoolAllocator* self, const Allocation* allocation)
{
  const VkDeviceSize       real_size = align_to(allocation->size, self->m_PageSize);
  const OffsetSize         span      = {allocation->offset, real_size};
  MemoryPool* const        pool      = (MemoryPool*)bfArray_at((void**)&self->m_MemPools, allocation->type);
  DeviceMemoryBlock* const block     = (DeviceMemoryBlock*)bfArray_at((void**)pool, allocation->index);
  bfBool32                 found     = bfFalse;

  block->isPageReserved = bfFalse;
  block->isPageMapped   = allocation->mapped_ptr != NULL;

  const size_t num_layouts = bfArray_size((void**)&block->layout);

  for (size_t i = 0; i < num_layouts; ++i)
  {
    OffsetSize* const layout = (OffsetSize*)bfArray_at((void**)&block->layout, i);

    if (layout->offset == real_size + allocation->offset)
    {
      layout->offset = allocation->offset;
      layout->size += real_size;
      found = bfTrue;
      break;
    }
  }

  assert(found && "VkPoolAllocator_free was called with an invalid allocation.");

  bfArray_push((void**)&block->layout, &span);

  VkDeviceSize* const mem_alloc_size = (VkDeviceSize*)bfArray_at((void**)&self->m_MemTypeAllocSizes, allocation->type);

  (*mem_alloc_size) -= real_size;
}

uint64_t VkPoolAllocator_allocationSize(const PoolAllocator* const self, const uint32_t mem_type)
{
  return *(const uint64_t* const)bfArray_at((void**)&self->m_MemTypeAllocSizes, mem_type);
}

uint32_t VkPoolAllocator_numAllocations(const PoolAllocator* const self)
{
  return self->m_NumAllocations;
}

void VkPoolAllocatorDtor(PoolAllocator* self)
{
  const size_t num_pools = bfArray_size((void**)&self->m_MemPools);

  for (uint32_t i = 0; i < num_pools; ++i)
  {
    MemPoolDtor(self->m_LogicalDevice->handle, (MemoryPool*)bfArray_at((void**)&self->m_MemPools, i));
  }

  bfArray_delete((void**)&self->m_MemPools);
  bfArray_delete((void**)&self->m_MemTypeAllocSizes);
}

static bfBool32 find_free_check_for_alloc(BlockSpanIndexPair* const loc, MemoryPool* const mem_pool, VkDeviceSize real_size, bfBool32 needs_new_page)
{
  for (uint32_t i = 0; i < bfArray_size((void**)mem_pool); ++i)
  {
    const DeviceMemoryBlock* block = (const DeviceMemoryBlock*)bfArray_at((void**)mem_pool, i);

    if (!block->isPageReserved)
    {
      for (uint32_t j = 0; j < bfArray_size((void**)&block->layout); ++j)
      {
        const OffsetSize* offset = (const OffsetSize*)bfArray_at((void**)&block->layout, j);

        const bfBool32 valid_offset = (needs_new_page) ? (offset->offset == 0) : bfTrue;

        if (valid_offset && offset->size >= real_size)
        {
          loc->blockIdx = i;
          loc->spanIdx  = j;

          return bfTrue;
        }
      }
    }
  }

  return bfFalse;
}

static VkDeviceSize align_to(VkDeviceSize size, VkDeviceSize page_size)
{
  return (size / page_size + 1) * page_size;
}

static size_t add_block_to_pool(PoolAllocator* self, uint32_t mem_type, VkDeviceSize size)
{
  MemoryPool* const pool = (MemoryPool*)bfArray_at((void**)&self->m_MemPools, mem_type);

  VkDeviceSize pool_size = size << 1;
  pool_size              = pool_size < self->m_MinBlockSize ? self->m_MinBlockSize : pool_size;

  VkMemoryAllocateInfo alloc_info;
  alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext           = nullptr;
  alloc_info.allocationSize  = pool_size;
  alloc_info.memoryTypeIndex = mem_type;

  DeviceMemoryBlock* new_block = (DeviceMemoryBlock*)bfArray_emplace((void**)pool);
  DeviceMemoryBlockCtor(new_block);

  const VkResult err = vkAllocateMemory(
   self->m_LogicalDevice->handle,
   &alloc_info,
   CUSTOM_ALLOCATOR,
   &new_block->mem.handle);

  assert(err != VK_ERROR_OUT_OF_HOST_MEMORY && "We ran out of host memory");
  assert(err != VK_ERROR_OUT_OF_DEVICE_MEMORY && "We ran out of device memory");
  assert(err != VK_ERROR_TOO_MANY_OBJECTS && "We ran out of allocations");
  assert(err != VK_ERROR_INVALID_EXTERNAL_HANDLE && "I passed something in wrong - Shareef");
  assert(err == VK_SUCCESS && "Some unexplained error occurred");

  new_block->mem.type = mem_type;
  new_block->mem.size = pool_size;

  OffsetSize* offset = (OffsetSize*)bfArray_emplace((void**)&new_block->layout);

  offset->offset = 0;
  offset->size   = pool_size;

  ++self->m_NumAllocations;

  return bfArray_size((void**)pool) - 1;
}

static void update_chunk(MemoryPool* const pool, const BlockSpanIndexPair* indices, VkDeviceSize size)
{
  const DeviceMemoryBlock* const block       = (const DeviceMemoryBlock*)bfArray_at((void**)pool, indices->blockIdx);
  OffsetSize* const              offset_size = (OffsetSize*)bfArray_at((void**)&block->layout, indices->spanIdx);

  offset_size->offset += size;
  offset_size->size -= size;
}
