#ifndef BIFROST_VULKAN_LOGICAL_DEVICE_H
#define BIFROST_VULKAN_LOGICAL_DEVICE_H

#include "bifrost/graphics/bifrost_gfx_api.h"
#include "bifrost_vulkan_hash.hpp"
#include "bifrost_vulkan_material_pool.h"
#include "bifrost_vulkan_mem_allocator.h"
#include "bifrost_vulkan_physical_device.h"

typedef struct
{
  bfTextureHandle attachments[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  uint32_t        num_attachments;

} bfFramebufferState;

struct ComparebfDescriptorSetInfo
{
  bool operator()(const bfDescriptorSetInfo& a, const bfDescriptorSetInfo& b) const
  {
    if (a.num_bindings != b.num_bindings)
    {
      return false;
    }

    for (uint32_t i = 0; i < a.num_bindings; ++i)
    {
      const bfDescriptorElementInfo* const binding_a = &a.bindings[i];
      const bfDescriptorElementInfo* const binding_b = &b.bindings[i];

      if (binding_a->type != binding_b->type)
      {
        return false;
      }

      if (binding_a->binding != binding_b->binding)
      {
        return false;
      }

      if (binding_a->array_element_start != binding_b->array_element_start)
      {
        return false;
      }

      if (binding_a->num_handles != binding_b->num_handles)
      {
        return false;
      }

      // self = hash::addU32(self, parent.layout_bindings[i].stageFlags);

      for (uint32_t j = 0; j < binding_a->num_handles; ++j)
      {
        if (binding_a->handles[j] != binding_b->handles[j])
        {
          return false;
        }

        if (binding_a->type == BIFROST_DESCRIPTOR_ELEMENT_BUFFER)
        {
          if (binding_a->offsets[j] != binding_b->offsets[j])
          {
            return false;
          }

          if (binding_a->sizes[j] != binding_b->sizes[j])
          {
            return false;
          }
        }
      }
    }

    return true;
  }
};

struct ComparebfPipelineCache
{
  bool operator()(const bfPipelineCache& a, const bfPipelineCache& b) const;
};

struct ComparebfFramebufferState
{
  bool operator()(const bfFramebufferState& a, const bfFramebufferState& b) const
  {
    if (a.num_attachments != b.num_attachments)
    {
      return false;
    }

    for (std::uint32_t i = 0; i < a.num_attachments; ++i)
    {
      if (a.attachments[i] != b.attachments[i])
      {
        return false;
      }
    }

    return true;
  }
};

using VulkanDescSetCache     = bifrost::vk::ObjectHashCache<bfDescriptorSet, bfDescriptorSetInfo, ComparebfDescriptorSetInfo>;
using VulkanPipelineCache    = bifrost::vk::ObjectHashCache<bfPipeline, bfPipelineCache, ComparebfPipelineCache>;
using VulkanFramebufferCache = bifrost::vk::ObjectHashCache<bfFramebuffer, bfFramebufferState, ComparebfFramebufferState>;

BIFROST_DEFINE_HANDLE(GfxDevice)
{
  VulkanPhysicalDevice*                                        parent;
  VkDevice                                                     handle;
  PoolAllocator                                                device_memory_allocator;
  VulkanDescriptorPool*                                        descriptor_pool;
  VkQueue                                                      queues[BIFROST_GFX_QUEUE_MAX];
  bifrost::vk::ObjectHashCache<bfRenderpass, bfRenderpassInfo> cache_renderpass;
  VulkanPipelineCache                                          cache_pipeline;
  VulkanFramebufferCache                                       cache_framebuffer;
  VulkanDescSetCache                                           cache_descriptor_set;
  BifrostGfxObjectBase*                                        cached_resources; /* Linked List */
};

#if __cplusplus
extern "C" {
#endif
BIFROST_DEFINE_HANDLE(Renderpass)
{
  BifrostGfxObjectBase super;
  VkRenderPass         handle;
  bfRenderpassInfo     info;
};

BIFROST_DEFINE_HANDLE(Framebuffer)
{
  BifrostGfxObjectBase super;
  VkFramebuffer        handle;
  // uint32_t             num_attachments;
  bfTextureHandle      attachments[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
};

BIFROST_DEFINE_HANDLE(Pipeline)
{
  BifrostGfxObjectBase super;
  VkPipeline           handle;
};

BIFROST_DEFINE_HANDLE(WindowSurface)
{
  VkSurfaceKHR           surface;
  VulkanSwapchainInfo    swapchain_info;
  VulkanSwapchain        swapchain;
  VkSemaphore*           is_image_available;
  VkSemaphore*           is_render_done;
  uint32_t               image_index;
  bfBool32               swapchain_needs_creation;
  bfGfxCommandListHandle current_cmd_list;
};

using VulkanWindow = bfWindowSurface;

BIFROST_DEFINE_HANDLE(GfxCommandList)
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
  VkClearValue        clear_colors[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  bfBool32            has_command;
  uint16_t            dynamic_state_dirty;
};

BIFROST_DEFINE_HANDLE(Buffer)
{
  BifrostGfxObjectBase super;
  PoolAllocator*       alloc_pool;
  VkBuffer             handle;
  Allocation           alloc_info;  // This has the aligned size.
  bfBufferSize         real_size;
};

BIFROST_DEFINE_HANDLE(ShaderModule)
{
  BifrostGfxObjectBase super;
  bfGfxDeviceHandle    parent;
  BifrostShaderType    type;
  VkShaderModule       handle;
  char                 entry_point[BIFROST_GFX_SHADER_ENTRY_POINT_NAME_LENGTH];
};

typedef struct
{
  uint32_t             size;
  bfShaderModuleHandle elements[BIFROST_SHADER_TYPE_MAX];

} bfShaderModuleList;

typedef struct bfDescriptorSetLayoutInfo_t
{
  uint32_t                     num_layout_bindings;
  VkDescriptorSetLayoutBinding layout_bindings[BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS];
  uint32_t                     num_image_samplers;
  uint32_t                     num_uniforms;

} bfDescriptorSetLayoutInfo;

BIFROST_DEFINE_HANDLE(ShaderProgram)
{
  BifrostGfxObjectBase      super;
  bfGfxDeviceHandle         parent;
  VkPipelineLayout          layout;
  uint32_t                  num_desc_set_layouts;
  VkDescriptorSetLayout     desc_set_layouts[BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS];
  bfDescriptorSetLayoutInfo desc_set_layout_infos[BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS];
  bfShaderModuleList        modules;
  char                      debug_name[BIFROST_GFX_SHADER_PROGRAM_NAME_LENGTH];
};

BIFROST_DEFINE_HANDLE(DescriptorSet)
{
  BifrostGfxObjectBase   super;
  bfShaderProgramHandle  shader_program;
  VkDescriptorSet        handle;
  uint32_t               set_index;
  DescriptorLink*        pool_link;
  VkDescriptorBufferInfo buffer_info[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  VkDescriptorImageInfo  image_info[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  VkBufferView           buffer_view_info[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  VkWriteDescriptorSet   writes[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  uint16_t               num_buffer_info;
  uint16_t               num_image_info;
  uint16_t               num_buffer_view_info;
  uint16_t               num_writes;
};

BIFROST_DEFINE_HANDLE(VertexLayoutSet)
{
  VkVertexInputBindingDescription   buffer_bindings[BIFROST_GFX_VERTEX_LAYOUT_MAX_BINDINGS];
  VkVertexInputAttributeDescription attrib_bindings[BIFROST_GFX_VERTEX_LAYOUT_MAX_BINDINGS];
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

  if (a.vertex_set_layout != b.vertex_set_layout)
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

  if (a.subpass_index != b.subpass_index)
  {
    return false;
  }

  if (a.subpass_index != b.subpass_index)
  {
    return false;
  }

  const auto num_attachments_a = a.renderpass->info.subpasses[a.subpass_index].num_out_attachment_refs;
  const auto num_attachments_b = b.renderpass->info.subpasses[b.subpass_index].num_out_attachment_refs;

  if (num_attachments_a != num_attachments_b)
  {
    return false;
  }

  for (std::uint32_t i = 0; i < num_attachments_a; ++i)
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

#endif /* BIFROST_VULKAN_LOGICAL_DEVICE_H */
