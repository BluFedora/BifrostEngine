#ifndef BF_VULKAN_LOGICAL_DEVICE_H
#define BF_VULKAN_LOGICAL_DEVICE_H

#include "../bf_gfx_object_cache.hpp"

#include "bf_vulkan_hash.hpp"
#include "bf_vulkan_material_pool.h"
#include "bf_vulkan_mem_allocator.h"
#include "bf_vulkan_physical_device.h"

#include "bf_vulkan_gfx_object.h"

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

template<typename T>
T* xxx_Alloc()
{
  return new T();
}

template<typename T>
void xxx_Free(T* ptr)
{
  delete ptr;
}

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

  std::uint64_t a_state_bits[2];
  std::uint64_t b_state_bits[2];

  std::memcpy(a_state_bits, &a.state, sizeof(a.state));
  std::memcpy(b_state_bits, &b.state, sizeof(b.state));

  a_state_bits[0] &= bfPipelineCache_state0Mask(&a.state);
  a_state_bits[1] &= bfPipelineCache_state1Mask(&a.state);
  b_state_bits[0] &= bfPipelineCache_state0Mask(&b.state);
  b_state_bits[1] &= bfPipelineCache_state1Mask(&b.state);

  if (std::memcmp(a_state_bits, b_state_bits, sizeof(a.state)) != 0)
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
    if (std::memcmp(&a.blending[i], &b.blending[i], sizeof(bfFramebufferBlending)) != 0)
    {
      return false;
    }
  }

  return true;
}

#endif /* BF_VULKAN_LOGICAL_DEVICE_H */
