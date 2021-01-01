#ifndef BF_VULKAN_LOGICAL_DEVICE_H
#define BF_VULKAN_LOGICAL_DEVICE_H

#include "../bf_gfx_object_cache.hpp"
#include "bf_vulkan_hash.hpp"
#include "bf_vulkan_material_pool.h"
#include "bf_vulkan_mem_allocator.h"
#include "bf_vulkan_physical_device.h"

BF_DEFINE_GFX_HANDLE(GfxDevice)
{
  VulkanPhysicalDevice*  parent;
  VkDevice               handle;
  PoolAllocator          device_memory_allocator;
  VulkanDescriptorPool*  descriptor_pool;
  VkQueue                queues[BF_GFX_QUEUE_MAX];
  GfxRenderpassCache     cache_renderpass;
  VulkanPipelineCache    cache_pipeline;
  VulkanFramebufferCache cache_framebuffer;
  VulkanDescSetCache     cache_descriptor_set;
  bfBaseGfxObject*       cached_resources; /* Linked List */
};

#if __cplusplus
extern "C" {
#endif
BF_DEFINE_GFX_HANDLE(Renderpass)
{
  bfBaseGfxObject  super;
  VkRenderPass     handle;
  bfRenderpassInfo info;
};

BF_DEFINE_GFX_HANDLE(Framebuffer)
{
  bfBaseGfxObject super;
  VkFramebuffer   handle;
};

BF_DEFINE_GFX_HANDLE(Pipeline)
{
  bfBaseGfxObject super;
  VkPipeline      handle;
};

using VulkanWindow = bfWindowSurface;

BF_DEFINE_GFX_HANDLE(GfxCommandList)
{
  bfGfxContextHandle  context;
  bfGfxDeviceHandle   parent;
  VkCommandBuffer     handle;
  VkFence             fence;
  VulkanWindow*       window;
  VkRect2D            render_area;
  bfFramebufferHandle framebuffer;
  bfPipelineHandle    pipeline;
  bfPipelineCache     pipeline_state;
  VkClearValue        clear_colors[k_bfGfxMaxAttachments];
  uint32_t            attachment_size[2];
  uint16_t            dynamic_state_dirty;
  bfBool16            has_command;
};

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

BF_DEFINE_GFX_HANDLE(ShaderModule)
{
  bfBaseGfxObject   super;
  bfGfxDeviceHandle parent;
  bfShaderType      type;
  VkShaderModule    handle;
  char              entry_point[k_bfGfxShaderEntryPointNameLength];
};

typedef struct
{
  uint32_t             size;
  bfShaderModuleHandle elements[BF_SHADER_TYPE_MAX];

} bfShaderModuleList;

typedef struct bfDescriptorSetLayoutInfo_t
{
  uint32_t                     num_layout_bindings;
  VkDescriptorSetLayoutBinding layout_bindings[k_bfGfxDesfcriptorSetMaxLayoutBindings];
  uint32_t                     num_image_samplers;
  uint32_t                     num_uniforms;

} bfDescriptorSetLayoutInfo;

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

inline bool ComparebfPipelineCache::operator()(const bfPipelineCache& a, const bfPipelineCache& b) const
{
  if (a.program != b.program)
  {
    return false;
  }

  // TODO: Check if this is strictly required.
  if (a.renderpass != b.renderpass)
  {
    return false;
  }

  if (a.vertex_layout != b.vertex_layout)
  {
    return false;
  }

  std::uint64_t state_bits[4];

  std::memcpy(state_bits + 0, &a.state, sizeof(a.state));
  std::memcpy(state_bits + 2, &b.state, sizeof(b.state));

  state_bits[0] &= bfPipelineCache_state0Mask(&a.state);
  state_bits[1] &= bfPipelineCache_state1Mask(&a.state);
  state_bits[2] &= bfPipelineCache_state0Mask(&b.state);
  state_bits[3] &= bfPipelineCache_state1Mask(&b.state);

  if (std::memcmp(state_bits + 0, state_bits + 2, sizeof(a.state)) != 0)
  {
    return false;
  }

  if (!a.state.dynamic_viewport)
  {
    if (std::memcmp(&a.viewport, &b.viewport, sizeof(a.viewport)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_scissor)
  {
    if (std::memcmp(&a.scissor_rect, &b.scissor_rect, sizeof(a.scissor_rect)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_blend_constants)
  {
    if (std::memcmp(a.blend_constants, b.blend_constants, sizeof(a.blend_constants)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_line_width)
  {
    if (std::memcmp(&a.line_width, &b.line_width, sizeof(a.line_width)) != 0)
    {
      return false;
    }
  }

  if (!a.state.dynamic_depth_bias)
  {
    if (a.depth.bias_constant_factor != b.depth.bias_constant_factor)
    {
      return false;
    }

    if (a.depth.bias_clamp != b.depth.bias_clamp)
    {
      return false;
    }

    if (a.depth.bias_slope_factor != b.depth.bias_slope_factor)
    {
      return false;
    }
  }

  if (!a.state.dynamic_depth_bounds)
  {
    if (a.depth.min_bound != b.depth.min_bound)
    {
      return false;
    }

    if (a.depth.max_bound != b.depth.max_bound)
    {
      return false;
    }
  }

  if (a.min_sample_shading != b.min_sample_shading)
  {
    return false;
  }

  if (a.sample_mask != b.sample_mask)
  {
    return false;
  }

  /*
    NOTE(SR):
      This check is not needed since if the two pipelines share the same
      RenderPass as well as subpass_index then of course the number of attachments are the same.
   
    const auto num_attachments_a = a.renderpass->info.subpasses[a.state.subpass_index].num_out_attachment_refs;
    const auto num_attachments_b = b.renderpass->info.subpasses[b.state.subpass_index].num_out_attachment_refs;

    if (num_attachments_a != num_attachments_b)
    {
      return false;
    }
  */

  const auto num_attachments = a.renderpass->info.subpasses[a.state.subpass_index].num_out_attachment_refs;

  for (std::uint32_t i = 0; i < num_attachments; ++i)
  {
    uint32_t blend_state_bits_a;
    uint32_t blend_state_bits_b;

    std::memcpy(&blend_state_bits_a, &a.blending[i], sizeof(uint32_t));
    std::memcpy(&blend_state_bits_b, &b.blending[i], sizeof(uint32_t));

    if (blend_state_bits_a != blend_state_bits_b)
    {
      return false;
    }
  }

  return true;
}

#endif /* BF_VULKAN_LOGICAL_DEVICE_H */
