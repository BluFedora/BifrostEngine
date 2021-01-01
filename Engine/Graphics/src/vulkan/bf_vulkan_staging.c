/******************************************************************************/
/*!
 * @file   bf_vulkan_staging.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Reuses a set of staging buffers for a more efficient allocation scheme
 *   and easier uploading of host data.
 *
 * @version 0.0.1
 * @date    2020-12-29
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#include "bf_vulkan_staging.h"

#include <assert.h> /* assert */

#define BF_VULKAN_CUSTOM_ALLOCATOR NULL

#if __cplusplus
extern "C" {
#endif
VkBool32 memory_type_from_properties(VkPhysicalDeviceMemoryProperties mem_props, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex);
#if __cplusplus
}
#endif

static size_t megabytesToBytes(size_t value)
{
  return value * 1024 * 1024;
}

static void CreateStagingManager(bfVkStagingManager* self, VkQueue gfx_queue, uint32_t graphics_queue_index, VkDevice device, VkPhysicalDeviceMemoryProperties memory_properties, size_t num_frames_delay, size_t max_buffer_size_mb)
{
  self->max_buffer_size  = megabytesToBytes(max_buffer_size_mb);
  self->mapped_ptr       = NULL;
  self->num_frames_delay = num_frames_delay;
  self->parent_device    = device;
  self->current_buffer   = 0u;
  self->gfx_queue        = gfx_queue;

  // Create Buffers
  {
    VkBufferCreateInfo buffer_create_info;
    buffer_create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext                 = NULL;
    buffer_create_info.flags                 = 0x0;
    buffer_create_info.size                  = self->max_buffer_size;
    buffer_create_info.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices   = NULL;

    for (size_t i = 0; i < num_frames_delay; ++i)
    {
      bfVkStageBufferFrame* buffer = self->buffers + i;

      const VkResult err = vkCreateBuffer(device, &buffer_create_info, BF_VULKAN_CUSTOM_ALLOCATOR, &buffer->buffer_handle);

      assert(err == VK_SUCCESS && "Failed to create buffer for staging frame.");
    }
  }

  VkDeviceSize aligned_size;

  // Memory Backing
  {
    VkMemoryRequirements memory_requirements;

    vkGetBufferMemoryRequirements(device, self->buffers[0].buffer_handle, &memory_requirements);

    const VkDeviceSize align_mod               = memory_requirements.size % memory_requirements.alignment;
    aligned_size                               = (align_mod == 0) ? memory_requirements.size : (memory_requirements.size + memory_requirements.alignment - align_mod);
    const VkDeviceSize total_memory_block_size = aligned_size * num_frames_delay;

    VkMemoryAllocateInfo memory_alloc_info;
    memory_alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memory_alloc_info.pNext          = NULL;
    memory_alloc_info.allocationSize = total_memory_block_size;

    const VkMemoryPropertyFlags preferred_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const VkMemoryPropertyFlags required_flags  = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    VkBool32 found_memory_type = memory_type_from_properties(
     memory_properties,
     memory_requirements.memoryTypeBits,
     preferred_flags,
     &memory_alloc_info.memoryTypeIndex);

    // If we could not get out preferred_flags.
    if (!found_memory_type)
    {
      found_memory_type = memory_type_from_properties(
       memory_properties,
       memory_requirements.memoryTypeBits,
       required_flags,
       &memory_alloc_info.memoryTypeIndex);
    }

    assert(found_memory_type && "The memory type index could not be found.");

    VkResult err = vkAllocateMemory(device, &memory_alloc_info, BF_VULKAN_CUSTOM_ALLOCATOR, &self->memory);

    assert(err == VK_SUCCESS && "Could not allocate memory for the staging buffers.");

    err = vkMapMemory(device, self->memory, 0, total_memory_block_size, 0x0, (void**)&self->mapped_ptr);

    assert(err == VK_SUCCESS && "Could not map memory.");
  }

  // Setup Command Pool
  {
    VkCommandPoolCreateInfo command_pool_create_info;
    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext            = NULL;
    command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = graphics_queue_index;

    const VkResult err = vkCreateCommandPool(device, &command_pool_create_info, BF_VULKAN_CUSTOM_ALLOCATOR, &self->cmd_pool);

    assert(err == VK_SUCCESS && "Could create comamnd pool for staging manager.");
  }

  // Rest of initialization of the per frame buffer.
  {
    VkFenceCreateInfo fence_create_info;
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = NULL;
    fence_create_info.flags = 0x0;

    VkCommandBufferAllocateInfo command_buffer_alloc_info;
    command_buffer_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_alloc_info.pNext              = NULL;
    command_buffer_alloc_info.commandPool        = self->cmd_pool;
    command_buffer_alloc_info.level              = 0;
    command_buffer_alloc_info.commandBufferCount = 1;

    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext            = NULL;
    command_buffer_begin_info.flags            = 0x0;
    command_buffer_begin_info.pInheritanceInfo = NULL;

    for (size_t i = 0; i < num_frames_delay; ++i)
    {
      bfVkStageBufferFrame* buffer                = self->buffers + i;
      const VkDeviceSize    memory_binding_offset = i * aligned_size;

      {
        const VkResult err = vkBindBufferMemory(device, buffer->buffer_handle, self->memory, memory_binding_offset);

        assert(err == VK_SUCCESS && "Failed to bind buffer to memory for staging frame.");
      }

      {
        const VkResult err = vkAllocateCommandBuffers(device, &command_buffer_alloc_info, &buffer->cmd_buffer);

        assert(err == VK_SUCCESS && "Failed to allocate command buffer for staging frame.");
      }

      {
        const VkResult err = vkCreateFence(device, &fence_create_info, NULL, &buffer->fence);

        assert(err == VK_SUCCESS && "Failed to create fence for staging frame.");
      }

      {
        const VkResult err = vkBeginCommandBuffer(buffer->cmd_buffer, &command_buffer_begin_info);

        assert(err == VK_SUCCESS && "Failed to begin command buffer.");
      }

      buffer->amount_memory_used = 0u;
      buffer->data               = self->mapped_ptr + memory_binding_offset;
    }
  }
}

static void Flush(bfVkStagingManager* self)
{
  bfVkStageBufferFrame* current_buffer = self->buffers + self->current_buffer;

  if (!current_buffer->is_submitted && current_buffer->amount_memory_used != 0u)
  {
    VkMemoryBarrier barrier;
    barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrier.pNext         = NULL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;

    vkCmdPipelineBarrier(
     current_buffer->cmd_buffer,
     VK_PIPELINE_STAGE_TRANSFER_BIT,
     VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
     0,
     1,
     &barrier,
     0,
     NULL,
     0,
     NULL);

    vkEndCommandBuffer(current_buffer->cmd_buffer);

    VkMappedMemoryRange memory_range;
    memory_range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    memory_range.pNext  = NULL;
    memory_range.memory = self->memory;
    memory_range.offset = (self->current_buffer * self->max_buffer_size);
    memory_range.size   = current_buffer->amount_memory_used;

    vkFlushMappedMemoryRanges(self->parent_device, 1, &memory_range);

    VkSubmitInfo submit_info;
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = NULL;
    submit_info.waitSemaphoreCount   = 0u;
    submit_info.pWaitSemaphores      = NULL;
    submit_info.pWaitDstStageMask    = NULL;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &current_buffer->cmd_buffer;
    submit_info.signalSemaphoreCount = 0u;
    submit_info.pSignalSemaphores    = NULL;

    vkQueueSubmit(self->gfx_queue, 1, &submit_info, current_buffer->fence);

    current_buffer->is_submitted = bfTrue;

    self->current_buffer = (self->current_buffer + 1) % self->num_frames_delay;
  }
}

static void Wait(bfVkStagingManager* self, bfVkStageBufferFrame* buffer)
{
  if (buffer->is_submitted)
  {
    VkResult err = vkWaitForFences(self->parent_device, 1, &buffer->fence, VK_TRUE, UINT64_MAX);

    assert(err == VK_SUCCESS);

    err = vkResetFences(self->parent_device, 1, &buffer->fence);

    assert(err == VK_SUCCESS);

    buffer->amount_memory_used = 0u;
    buffer->is_submitted       = bfFalse;

    VkCommandBufferBeginInfo command_buffer_begin_info;
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext            = NULL;
    command_buffer_begin_info.flags            = 0x0;
    command_buffer_begin_info.pInheritanceInfo = NULL;

    err = vkBeginCommandBuffer(buffer->cmd_buffer, &command_buffer_begin_info);
    assert(err == VK_SUCCESS && "Failed to begin command buffer.");
  }
}

static bfVkStagingResult Stage(bfVkStagingManager* self, size_t size, size_t alignment)
{
  assert(size <= self->max_buffer_size && "Staging buffer cannoy hold that amount of memory.");

  bfVkStageBufferFrame* current_buffer = self->buffers + self->current_buffer;
  const VkDeviceSize    align_mod      = current_buffer->amount_memory_used % alignment;

  if (align_mod != 0)
  {
    current_buffer->amount_memory_used += (alignment - align_mod);
  }

  if ((current_buffer->amount_memory_used + size) >= self->max_buffer_size && !current_buffer->is_submitted)
  {
    Flush(self);
  }

  current_buffer = self->buffers + self->current_buffer;

  Wait(self, current_buffer);

  bfVkStagingResult result;

  result.mapped_ptr    = current_buffer->data + current_buffer->amount_memory_used;
  result.cmd_buffer    = current_buffer->cmd_buffer;
  result.buffer        = current_buffer->buffer_handle;
  result.buffer_offset = current_buffer->amount_memory_used;

  current_buffer->amount_memory_used += size;

  return result;
}

static void Destroy(bfVkStagingManager* self)
{
  const size_t num_frames_delay = self->num_frames_delay;

  vkUnmapMemory(self->parent_device, self->memory);

  for (size_t i = 0; i < num_frames_delay; ++i)
  {
    bfVkStageBufferFrame* buffer = self->buffers + i;

    vkDestroyFence(self->parent_device, buffer->fence, BF_VULKAN_CUSTOM_ALLOCATOR);
    vkDestroyBuffer(self->parent_device, buffer->buffer_handle, BF_VULKAN_CUSTOM_ALLOCATOR);
    vkFreeCommandBuffers(self->parent_device, self->cmd_pool, 1, &buffer->cmd_buffer);
  }
}

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
