/******************************************************************************/
/*!
 * @file   bf_vulkan_gfx_object.h
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Provides the defintions for the Opaque API Handles.
 *
 * @version 0.0.1
 * @date    2021-03-10
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef BF_VULKAN_GFX_OBJECT_H
#define BF_VULKAN_GFX_OBJECT_H

#include "bf/bf_gfx_handle.h"

#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif

BF_DEFINE_GFX_HANDLE(WindowSurface)
{
  VkSurfaceKHR           surface;
  VulkanSwapchainInfo    swapchain_info;
  VulkanSwapchain        swapchain;
  VkSemaphore*           is_image_available;
  VkSemaphore*           is_render_done;
  uint32_t               image_index;
  bfBool32               swapchain_needs_deletion;
  bfBool32               swapchain_needs_creation;
  bfGfxCommandList       cmd_list_memory[5];
  bfGfxCommandListHandle current_cmd_list;
};

typedef struct bfDescriptorSetLayoutInfo
{
  uint32_t                     num_layout_bindings;
  VkDescriptorSetLayoutBinding layout_bindings[k_bfGfxDesfcriptorSetMaxLayoutBindings];
  uint32_t                     num_image_samplers;
  uint32_t                     num_uniforms;

} bfDescriptorSetLayoutInfo;

typedef struct bfShaderModuleList
{
  uint32_t             size;
  bfShaderModuleHandle elements[BF_SHADER_TYPE_MAX];

} bfShaderModuleList;

BF_DEFINE_GFX_HANDLE(ShaderProgram)
{
  bfBaseGfxObject           super;
  bfGfxDeviceHandle         parent;
  VkPipelineLayout          layout;
  uint32_t                  num_desc_set_layouts;
  VkDescriptorSetLayout     desc_set_layouts[k_bfGfxDescriptorSets];
  bfDescriptorSetLayoutInfo desc_set_layout_infos[k_bfGfxDescriptorSets];
  bfShaderModuleList        modules;
  char                      debug_name[k_bfGfxShaderProgramNameLength];
};

BF_DEFINE_GFX_HANDLE(DescriptorSet)
{
  bfBaseGfxObject        super;
  bfShaderProgramHandle  shader_program;
  VkDescriptorSet        handle;
  uint32_t               set_index;
  DescriptorLink*        pool_link;
  VkDescriptorBufferInfo buffer_info[k_bfGfxMaxDescriptorSetWrites];
  VkDescriptorImageInfo  image_info[k_bfGfxMaxDescriptorSetWrites];
  VkBufferView           buffer_view_info[k_bfGfxMaxDescriptorSetWrites];
  VkWriteDescriptorSet   writes[k_bfGfxMaxDescriptorSetWrites];
  uint16_t               num_buffer_info;
  uint16_t               num_image_info;
  uint16_t               num_buffer_view_info;
  uint16_t               num_writes;
};

BF_DEFINE_GFX_HANDLE(VertexLayoutSet)
{
  VkVertexInputBindingDescription   buffer_bindings[k_bfGfxMaxLayoutBindings];
  VkVertexInputAttributeDescription attrib_bindings[k_bfGfxMaxLayoutBindings];
  uint8_t                           num_buffer_bindings;
  uint8_t                           num_attrib_bindings;
};

#if __cplusplus
}
#endif

#endif /* BF_VULKAN_GFX_OBJECT_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

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