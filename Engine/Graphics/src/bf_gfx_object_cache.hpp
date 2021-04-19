#ifndef BF_GFX_OBJECT_CACHE_HPP
#define BF_GFX_OBJECT_CACHE_HPP

#include "bf/bf_gfx_api.h"                                  /* bf gfx API      */
#include "bf/bf_hash.hpp"                                   /* bf::hash::*     */
#include "bf/data_structures/bifrost_object_hash_cache.hpp" /* ObjectHashCache */

#include <algorithm>

typedef struct
{
  bfTextureHandle attachments[k_bfGfxMaxAttachments];
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
        if (binding_a->handles[j]->id != binding_b->handles[j]->id)
        {
          return false;
        }

        if (binding_a->type == BF_DESCRIPTOR_ELEMENT_BUFFER)
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

    return std::equal(
     a.attachments,
     a.attachments + a.num_attachments,
     b.attachments,
     b.attachments + b.num_attachments,
     [](bfTextureHandle a, bfTextureHandle b) {
       return a->super.id == b->super.id;
     });
  }
};

using GfxRenderpassCache     = bf::ObjectHashCache<bfRenderpass, bfRenderpassInfo>;
using VulkanDescSetCache     = bf::ObjectHashCache<bfDescriptorSet, bfDescriptorSetInfo, ComparebfDescriptorSetInfo>;
using VulkanPipelineCache    = bf::ObjectHashCache<bfPipeline, bfPipelineCache, ComparebfPipelineCache>;
using VulkanFramebufferCache = bf::ObjectHashCache<bfFramebuffer, bfFramebufferState, ComparebfFramebufferState>;

namespace bf
{
  namespace gfx_hash
  {
    inline void hash(std::uint64_t& self, const bfViewport& vp)
    {
      self = hash::addF32(self, vp.x);
      self = hash::addF32(self, vp.y);
      self = hash::addF32(self, vp.width);
      self = hash::addF32(self, vp.height);
      self = hash::addF32(self, vp.min_depth);
      self = hash::addF32(self, vp.max_depth);
    }

    inline void hash(std::uint64_t& self, const bfScissorRect& scissor)
    {
      self = hash::addS32(self, scissor.x);
      self = hash::addS32(self, scissor.y);
      self = hash::addU32(self, scissor.width);
      self = hash::addU32(self, scissor.height);
    }

    inline void hash(std::uint64_t& self, const bfPipelineDepthInfo& depth, const bfPipelineState& state)
    {
      if (!state.dynamic_depth_bias)
      {
        self = hash::addF32(self, depth.bias_constant_factor);
        self = hash::addF32(self, depth.bias_clamp);
        self = hash::addF32(self, depth.bias_slope_factor);
      }

      if (!state.dynamic_depth_bounds)
      {
        self = hash::addF32(self, depth.min_bound);
        self = hash::addF32(self, depth.max_bound);
      }
    }

    inline void hash(std::uint64_t& self, const bfFramebufferBlending& fb_blending)
    {
      uint32_t blend_state_bits;

      static_assert(sizeof(bfFramebufferBlending) == sizeof(blend_state_bits), "Required size.");

      std::memcpy(&blend_state_bits, &fb_blending, sizeof(blend_state_bits));

      self = hash::addU32(self, blend_state_bits);
    }

    inline std::uint64_t hash(std::uint64_t self, bfTextureHandle* attachments, std::size_t num_attachments)
    {
      if (num_attachments)
      {
        self = hash::addS32(self, bfTexture_width(attachments[0]));
        self = hash::addS32(self, bfTexture_height(attachments[0]));
      }

      for (std::size_t i = 0; i < num_attachments; ++i)
      {
        self = hash::addPointer(self, attachments[i]);
      }

      return self;
    }

    inline std::uint64_t hash(std::uint64_t self, const bfAttachmentInfo* attachment_info)
    {
      self = hash::addU32(self, attachment_info->texture->super.id);
      self = hash::addU32(self, attachment_info->final_layout);
      self = hash::addU32(self, attachment_info->may_alias);

      return self;
    }

    inline std::uint64_t hash(std::uint64_t self, const bfAttachmentRefCache* attachment_ref_info)
    {
      self = hash::addU32(self, attachment_ref_info->attachment_index);
      self = hash::addU32(self, attachment_ref_info->layout);

      return self;
    }

    inline std::uint64_t hash(std::uint64_t self, const bfSubpassCache* subpass_info)
    {
      self = hash::addU32(self, subpass_info->num_out_attachment_refs);

      for (uint32_t i = 0; i < subpass_info->num_out_attachment_refs; ++i)
      {
        self = hash(self, subpass_info->out_attachment_refs + i);
      }

      self = hash::addU32(self, subpass_info->num_in_attachment_refs);

      for (uint32_t i = 0; i < subpass_info->num_in_attachment_refs; ++i)
      {
        self = hash(self, subpass_info->in_attachment_refs + i);
      }

      self = hash(self, &subpass_info->depth_attachment);

      return self;
    }

    inline std::uint64_t hash(std::uint64_t self, const bfRenderpassInfo* renderpass_info)
    {
      self = hash::addU32(self, renderpass_info->load_ops);
      self = hash::addU32(self, renderpass_info->stencil_load_ops);
      self = hash::addU32(self, renderpass_info->clear_ops);
      self = hash::addU32(self, renderpass_info->stencil_clear_ops);
      self = hash::addU32(self, renderpass_info->store_ops);
      self = hash::addU32(self, renderpass_info->stencil_store_ops);
      self = hash::addU32(self, renderpass_info->num_subpasses);

      for (uint16_t i = 0; i < renderpass_info->num_subpasses; ++i)
      {
        self = hash(self, renderpass_info->subpasses + i);
      }

      self = hash::addU32(self, renderpass_info->num_attachments);

      for (uint32_t i = 0; i < renderpass_info->num_attachments; ++i)
      {
        self = hash(self, renderpass_info->attachments + i);
      }

      return self;
    }
  }  // namespace gfx_hash
}  // namespace bf

#endif /* BF_GFX_OBJECT_CACHE_HPP */