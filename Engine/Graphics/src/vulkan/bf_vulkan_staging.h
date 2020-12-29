/******************************************************************************/
/*!
 * @file   bf_vulkan_staging.h
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
#ifndef BF_VULKAN_STAGING_HPP
#define BF_VULKAN_STAGING_HPP

#include "bf/bf_core.h"              /* bfBool32, bfByte      */
#include "bf/bf_gfx_limits.h"        /* k_bfGfxMaxFramesDelay */
#include "bf_vulkan_mem_allocator.h" /* bfBufferHandle        */

#include <vulkan/vulkan.h> /* VkCommandBuffer */

#if __cplusplus
extern "C" {
#endif

typedef struct
{
  VkBuffer        buffer_handle;
  VkCommandBuffer cmd_buffer;
  VkDeviceSize    amount_memory_used;
  VkFence         fence;
  bfByte*         data;
  bfBool32        is_submitted;

} bfVkStageBufferFrame;

typedef struct
{
  size_t               max_buffer_size;
  bfByte*              mapped_ptr;
  size_t               num_frames_delay;
  VkDeviceMemory       memory;
  VkCommandPool        cmd_pool;
  size_t               current_buffer;
  bfVkStageBufferFrame buffers[k_bfGfxMaxFramesDelay];
  VkDevice             parent_device;
  VkQueue              gfx_queue;

} bfVkStagingManager;

typedef struct
{
  bfByte*         mapped_ptr;
  VkCommandBuffer cmd_buffer;
  VkBuffer        buffer;
  VkDeviceSize    buffer_offset;

} bfVkStagingResult;

void              bfVkStagingManager_initialize(bfVkStagingManager* self, size_t num_frames_delay);
bfVkStagingResult bfVkStagingManager_stage(bfVkStagingManager* self, size_t size, size_t alignment);
void              bfVkStagingManager_flush(bfVkStagingManager* self);
void              bfVkStagingManager_shutdown(bfVkStagingManager* self);
#if __cplusplus
}
#endif

#endif /* BF_VULKAN_STAGING_HPP */

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
