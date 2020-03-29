#include "bifrost_vulkan_mem_allocator.h"

#include "bifrost_vulkan_logical_device.h"

#include "bifrost/data_structures/bifrost_array_t.h"

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
  self->layout         = Array_new(OffsetSize, 1);
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
  Array_delete(&self->layout);
}

static void MemPoolDtor(VkDevice device_handle, MemoryPool* self)
{
  const size_t num_blocks = Array_size(self);

  for (uint32_t j = 0; j < num_blocks; ++j)
  {
    DeviceMemoryBlockDtor(device_handle, (DeviceMemoryBlock*)Array_at(self, j));
  }

  Array_delete(self);
}

void VkPoolAllocatorCtor(PoolAllocator* self, const bfGfxDevice* const logical_device)
{
  const VulkanPhysicalDevice* const device = logical_device->parent;

  const uint32_t memory_properties_type_count = device->memory_properties.memoryTypeCount;

  self->m_MemTypeAllocSizes = OLD_bfArray_newA(self->m_MemTypeAllocSizes, memory_properties_type_count);
  self->m_MemPools          = OLD_bfArray_newA(self->m_MemPools, memory_properties_type_count);
  self->m_PageSize          = (uint32_t)device->device_properties.limits.bufferImageGranularity;
  self->m_MinBlockSize      = (uint64_t)self->m_PageSize * BIFROST_POOL_ALLOC_NUM_PAGES_PER_BLOCK;
  self->m_LogicalDevice     = logical_device;

  for (uint32_t i = 0; i < memory_properties_type_count; ++i)
  {
    static const size_t initial_pool_size = 5;

    MemoryPool* const pool = (MemoryPool*)Array_emplace(&self->m_MemPools);
    size_t* const     size = (size_t*)Array_emplace(&self->m_MemTypeAllocSizes);

    *pool = OLD_bfArray_newA(*pool, initial_pool_size);

    for (uint32_t j = 0; j < initial_pool_size; ++j)
    {
      DeviceMemoryBlockCtor((DeviceMemoryBlock*)Array_emplace(pool));
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
  MemoryPool* const  pool           = (MemoryPool*)Array_at(&self->m_MemPools, mem_type);
  BlockSpanIndexPair loc            = {0, 0};
  const bfBool32     found          = find_free_check_for_alloc(&loc, pool, real_size, needs_own_page);
  uint64_t* const    mem_alloc_size = (uint64_t*)Array_at(&self->m_MemTypeAllocSizes, mem_type);

  *mem_alloc_size += real_size;

  if (!found)
  {
    loc.blockIdx = (uint32_t)add_block_to_pool(self, mem_type, real_size);
    loc.spanIdx  = 0;
  }

  DeviceMemoryBlock* const block = (DeviceMemoryBlock*)Array_at(pool, loc.blockIdx);

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
  out->offset     = ((OffsetSize*)Array_at(&block->layout, loc.spanIdx))->offset;
  out->type       = mem_type;
  out->index      = loc.blockIdx;
  out->mapped_ptr = (char*)block->pageMapping + out->offset;

  update_chunk(pool, &loc, real_size);
}

void VkPoolAllocator_free(PoolAllocator* self, const Allocation* allocation)
{
  const VkDeviceSize       real_size = align_to(allocation->size, self->m_PageSize);
  const OffsetSize         span      = {allocation->offset, real_size};
  MemoryPool* const        pool      = (MemoryPool*)Array_at(&self->m_MemPools, allocation->type);
  DeviceMemoryBlock* const block     = (DeviceMemoryBlock*)Array_at(pool, allocation->index);
  bfBool32                 found     = bfFalse;

  block->isPageReserved = bfFalse;

  const size_t num_layouts = Array_size(&block->layout);

  for (size_t i = 0; i < num_layouts; ++i)
  {
    OffsetSize* const layout = (OffsetSize*)Array_at(&block->layout, i);

    if (layout->offset == real_size + allocation->offset)
    {
      layout->offset = allocation->offset;
      layout->size += real_size;
      found = bfTrue;
      break;
    }
  }

  assert(found && "VkPoolAllocator_free was called with an invalid allocation.");

  Array_push(&block->layout, &span);

  VkDeviceSize* const mem_alloc_size = (VkDeviceSize*)Array_at(&self->m_MemTypeAllocSizes, allocation->type);

  (*mem_alloc_size) -= real_size;
}

uint64_t VkPoolAllocator_allocationSize(const PoolAllocator* const self, const uint32_t mem_type)
{
  return *(const uint64_t* const)Array_at(&self->m_MemTypeAllocSizes, mem_type);
}

uint32_t VkPoolAllocator_numAllocations(const PoolAllocator* const self)
{
  return self->m_NumAllocations;
}

void VkPoolAllocatorDtor(PoolAllocator* self)
{
  const size_t num_pools = Array_size(&self->m_MemPools);

  for (uint32_t i = 0; i < num_pools; ++i)
  {
    MemPoolDtor(self->m_LogicalDevice->handle, (MemoryPool*)Array_at(&self->m_MemPools, i));
  }

  Array_delete(&self->m_MemPools);
  Array_delete(&self->m_MemTypeAllocSizes);
}

static bfBool32 find_free_check_for_alloc(BlockSpanIndexPair* const loc, MemoryPool* const mem_pool, VkDeviceSize real_size, bfBool32 needs_new_page)
{
  for (uint32_t i = 0; i < Array_size(mem_pool); ++i)
  {
    const DeviceMemoryBlock* block = (const DeviceMemoryBlock*)Array_at(mem_pool, i);

    if (!block->isPageReserved)
    {
      for (uint32_t j = 0; j < Array_size(&block->layout); ++j)
      {
        const OffsetSize* offset = (const OffsetSize*)Array_at(&block->layout, j);

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
  MemoryPool* const pool = (MemoryPool*)Array_at(&self->m_MemPools, mem_type);

  VkDeviceSize pool_size = size << 1;
  pool_size              = pool_size < self->m_MinBlockSize ? self->m_MinBlockSize : pool_size;

  VkMemoryAllocateInfo alloc_info;
  alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext           = nullptr;
  alloc_info.allocationSize  = pool_size;
  alloc_info.memoryTypeIndex = mem_type;

  DeviceMemoryBlock* new_block = (DeviceMemoryBlock*)Array_emplace(pool);
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

  OffsetSize* offset = (OffsetSize*)Array_emplace(&new_block->layout);

  offset->offset = 0;
  offset->size   = pool_size;

  ++self->m_NumAllocations;

  return Array_size(pool) - 1;
}

static void update_chunk(MemoryPool* const pool, const BlockSpanIndexPair* indices, VkDeviceSize size)
{
  const DeviceMemoryBlock* const block       = (const DeviceMemoryBlock*)Array_at(pool, indices->blockIdx);
  OffsetSize* const              offset_size = (OffsetSize*)Array_at(&block->layout, indices->spanIdx);

  offset_size->offset += size;
  offset_size->size -= size;
}
