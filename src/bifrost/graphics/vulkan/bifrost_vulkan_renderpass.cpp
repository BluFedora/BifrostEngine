#include "bifrost_vulkan_logical_device.h"

#include "bifrost/meta/bifrost_meta_utils.hpp"
#include "bifrost_vulkan_conversions.h"

#include <cassert> /* assert */

#define CUSTOM_ALLOC nullptr

bfRenderpassHandle bfGfxDevice_newRenderpass(bfGfxDeviceHandle self, const bfRenderpassCreateParams* params)
{
  bfRenderpassHandle      renderpass      = new bfRenderpass();
  const auto              num_attachments = params->num_attachments;
  VkAttachmentDescription attachments[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  const uint32_t          num_subpasses = params->num_subpasses;
  VkSubpassDescription    subpasses[BIFROST_GFX_RENDERPASS_MAX_SUBPASSES];
  const auto              num_dependencies = params->num_dependencies;
  VkSubpassDependency     dependencies[BIFROST_GFX_RENDERPASS_MAX_DEPENDENCIES];
  VkAttachmentReference   inputs[BIFROST_GFX_RENDERPASS_MAX_SUBPASSES][BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  VkAttachmentReference   outputs[BIFROST_GFX_RENDERPASS_MAX_SUBPASSES][BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  VkAttachmentReference   depth_atts[BIFROST_GFX_RENDERPASS_MAX_SUBPASSES];

  const auto bitsToLoadOp = [](uint32_t i, bfLoadStoreFlags load_ops, bfLoadStoreFlags clear_ops) -> VkAttachmentLoadOp {
    if (bfBit(i) & clear_ops)
    {
      // TODO(Shareef): Check for the load bit NOT to be set.
      return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }
    else if (bfBit(i) & load_ops)
    {
      // TODO(Shareef): Check for the clear bit NOT to be set.
      return VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  };

  const auto bitsToStoreOp = [](uint32_t i, bfLoadStoreFlags store_ops) -> VkAttachmentStoreOp {
    if (bfBit(i) & store_ops)
    {
      return VK_ATTACHMENT_STORE_OP_STORE;
    }

    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  };

  const auto bfAttToVkAtt = [](const bfAttachmentRefCache* in) -> VkAttachmentReference {
    VkAttachmentReference out;
    out.attachment = in->attachment_index;
    out.layout     = bfVkConvertImgLayout(in->layout);
    return out;
  };

  for (uint32_t i = 0; i < num_attachments; ++i)
  {
    VkAttachmentDescription* const att      = attachments + i;
    const bfAttachmentInfo* const  att_info = params->attachments + i;

    att->flags          = att_info->may_alias ? VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT : 0x0;
    att->format         = bfVkConvertFormat(att_info->texture->tex_format);
    att->samples        = bfVkConvertSampleCount(att_info->texture->tex_samples);
    att->loadOp         = bitsToLoadOp(i, params->load_ops, params->clear_ops);
    att->storeOp        = bitsToStoreOp(i, params->store_ops);
    att->stencilLoadOp  = bitsToLoadOp(i, params->stencil_load_ops, params->stencil_clear_ops);
    att->stencilStoreOp = bitsToStoreOp(i, params->stencil_store_ops);
    att->initialLayout  = bfVkConvertImgLayout(att_info->texture->tex_layout);
    att->finalLayout    = bfVkConvertImgLayout(att_info->final_layout);
  }

  for (uint32_t i = 0; i < num_subpasses; ++i)
  {
    VkSubpassDescription* const sub      = subpasses + i;
    const bfSubpassCache* const sub_info = params->subpasses + i;

    for (uint32_t j = 0; j < sub_info->num_in_attachment_refs; ++j)
    {
      inputs[i][j] = bfAttToVkAtt(sub_info->in_attachment_refs + j);
    }

    for (uint32_t j = 0; j < sub_info->num_out_attachment_refs; ++j)
    {
      outputs[i][j] = bfAttToVkAtt(sub_info->out_attachment_refs + j);
    }

    const bool has_depth = sub_info->depth_attachment.attachment_index != UINT32_MAX;

    if (has_depth)
    {
      depth_atts[i] = bfAttToVkAtt(&sub_info->depth_attachment);
    }

    sub->flags                   = 0x0;
    sub->pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub->inputAttachmentCount    = sub_info->num_in_attachment_refs;
    sub->pInputAttachments       = inputs[i];
    sub->colorAttachmentCount    = sub_info->num_out_attachment_refs;
    sub->pColorAttachments       = outputs[i];
    sub->pResolveAttachments     = NULL;  // TODO(Shareef): This is for multisampling.
    sub->pDepthStencilAttachment = has_depth ? depth_atts + i : nullptr;
    sub->preserveAttachmentCount = 0;
    sub->pPreserveAttachments    = NULL; // NOTE(Shareef): For attachments that must be preserved during this subpass but must not be touched by it.
  }

  for (uint32_t i = 0; i < num_dependencies; ++i)
  {
    VkSubpassDependency* const       dep      = dependencies + i;
    const bfSubpassDependency* const dep_info = params->dependencies + i;

    dep->srcSubpass      = dep_info->subpasses[0];
    dep->dstSubpass      = dep_info->subpasses[1];
    dep->srcStageMask    = dep_info->pipeline_stage_flags[0];  // TODO(Shareef): Convert using a function
    dep->dstStageMask    = dep_info->pipeline_stage_flags[1];  // TODO(Shareef): Convert using a function
    dep->srcAccessMask   = dep_info->access_flags[0];          // TODO(Shareef): Convert using a function
    dep->dstAccessMask   = dep_info->access_flags[1];          // TODO(Shareef): Convert using a function
    dep->dependencyFlags = dep_info->reads_same_pixel ? VK_DEPENDENCY_BY_REGION_BIT : 0x0;
  }

  VkRenderPassCreateInfo renderpass_create_info;
  renderpass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpass_create_info.pNext           = nullptr;
  renderpass_create_info.flags           = 0x0;
  renderpass_create_info.attachmentCount = num_attachments;
  renderpass_create_info.pAttachments    = attachments;
  renderpass_create_info.subpassCount    = num_subpasses;
  renderpass_create_info.pSubpasses      = subpasses;
  renderpass_create_info.dependencyCount = num_dependencies;
  renderpass_create_info.pDependencies   = dependencies;

  const auto err = vkCreateRenderPass(self->handle, &renderpass_create_info, CUSTOM_ALLOC, &renderpass->handle);
  assert(err == VK_SUCCESS && "Failed to create renderpass.");

  return renderpass;
}

void bfGfxDevice_deleteRenderpass(bfGfxDeviceHandle self, bfRenderpassHandle renderpass)
{
  vkDestroyRenderPass(self->handle, renderpass->handle, CUSTOM_ALLOC);
  delete renderpass;
}
