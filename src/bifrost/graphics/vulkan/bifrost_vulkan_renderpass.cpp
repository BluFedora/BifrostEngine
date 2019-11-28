#include "bifrost_vulkan_logical_device.h"

#include "bifrost/meta/bifrost_meta_utils.hpp"
#include "bifrost_vulkan_conversions.h"

#include <cassert> /* assert */

#define CUSTOM_ALLOC nullptr

bfRenderpassHandle bfGfxDevice_newRenderpass(bfGfxDeviceHandle self, const bfRenderpassCreateParams* params)
{
  bfRenderpassHandle renderpass           = new bfRenderpass();
  renderpass->super.type                  = BIFROST_GFX_OBJECT_RENDERPASS;
  renderpass->info                        = *params;
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

    if (bfBit(i) & load_ops)
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
    att->format         = att_info->texture->tex_format;
    att->samples        = bfVkConvertSampleCount(att_info->texture->tex_samples);
    att->loadOp         = bitsToLoadOp(i, params->load_ops, params->clear_ops);
    att->storeOp        = bitsToStoreOp(i, params->store_ops);
    att->stencilLoadOp  = bitsToLoadOp(i, params->stencil_load_ops, params->stencil_clear_ops);
    att->stencilStoreOp = bitsToStoreOp(i, params->stencil_store_ops);
    att->initialLayout  = att_info->texture->tex_layout;
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
    sub->pPreserveAttachments    = NULL;
    // NOTE(Shareef): For attachments that must be preserved during this subpass but must not be touched by it.
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

void bfGfxDevice_release(bfGfxDeviceHandle self, void* resource)
{
  BifrostGfxObjectBase* const obj = static_cast<BifrostGfxObjectBase*>(resource);

  switch (obj->type)
  {
    case BIFROST_GFX_OBJECT_BUFFER:
    {
      bfBufferHandle buffer = reinterpret_cast<bfBufferHandle>(obj);

      vkDestroyBuffer(self->handle, buffer->handle, CUSTOM_ALLOC);
      VkPoolAllocator_free(buffer->alloc_pool, &buffer->alloc_info);
      break;
    }
    case BIFROST_GFX_OBJECT_RENDERPASS:
    {
      bfRenderpassHandle renderpass = reinterpret_cast<bfRenderpassHandle>(obj);
      vkDestroyRenderPass(self->handle, renderpass->handle, CUSTOM_ALLOC);
      break;
    }
    case BIFROST_GFX_OBJECT_SHADER_MODULE:
    {
      bfShaderModuleHandle shader_module = reinterpret_cast<bfShaderModuleHandle>(obj);

      if (shader_module->handle != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(shader_module->parent->handle, shader_module->handle, CUSTOM_ALLOC);
      }
      break;
    }
    case BIFROST_GFX_OBJECT_SHADER_PROGRAM:
    {
      bfShaderProgramHandle shader_program = reinterpret_cast<bfShaderProgramHandle>(obj);

      for (uint32_t i = 0; i < shader_program->num_desc_set_layouts; ++i)
      {
        const VkDescriptorSetLayout layout = shader_program->desc_set_layouts[i];

        if (layout != VK_NULL_HANDLE)
        {
          vkDestroyDescriptorSetLayout(shader_program->parent->handle, layout, CUSTOM_ALLOC);
        }
      }

      if (shader_program->layout != VK_NULL_HANDLE)
      {
        vkDestroyPipelineLayout(shader_program->parent->handle, shader_program->layout, CUSTOM_ALLOC);
      }
      break;
    }
    case BIFROST_GFX_OBJECT_DESCRIPTOR_SET:
    {
      break;
    }
    case BIFROST_GFX_OBJECT_TEXTURE: {
      bfTextureHandle texture = reinterpret_cast<bfTextureHandle>(obj);

      bfTexture_setSampler(texture, nullptr);

      if (texture->tex_view != VK_NULL_HANDLE)
      {
        vkDestroyImageView(texture->parent->handle, texture->tex_view, CUSTOM_ALLOC);
      }

      if (texture->tex_memory != VK_NULL_HANDLE)
      {
        vkFreeMemory(texture->parent->handle, texture->tex_memory, CUSTOM_ALLOC);
      }

      if (texture->tex_image != VK_NULL_HANDLE)
      {
        vkDestroyImage(texture->parent->handle, texture->tex_image, CUSTOM_ALLOC);
      }
      break;
    }
    default:
      assert(false);
      break;
  }

  delete obj;
}
