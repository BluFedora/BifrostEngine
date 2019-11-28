#include "bifrost/graphics/bifrost_gfx_api.h"

#include "vulkan/bifrost_vulkan_physical_device.h"  // TODO: This is bad

#include <assert.h>                       /* assert         */
#include <string.h> /* memcpy, memset */  // TODO(Shareef): Use the Bifrost Versions

static void setupSampler(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, uint32_t num_layers);
static void setupAttachment(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

bfTextureSamplerProperties bfTextureSamplerProperties_init(BifrostSamplerFilterMode filter, BifrostSamplerAddressMode uv_addressing)
{
  bfTextureSamplerProperties props;

  props.min_filter = filter;
  props.mag_filter = filter;
  props.u_address = uv_addressing;
  props.v_address = uv_addressing;
  props.w_address = uv_addressing;
  props.min_lod = 0.0f;
  props.max_lod = 1.0f;

  return props;
}

bfTextureCreateParams bfTextureCreateParams_init2D(uint32_t width, uint32_t height, BifrostImageFormat format)
{
  bfTextureCreateParams ret;
  setupSampler(&ret, width, height, format, 1);
  return ret;
}

bfTextureCreateParams bfTextureCreateParams_initCubeMap(uint32_t width, uint32_t height, BifrostImageFormat format)
{
  bfTextureCreateParams ret;
  setupSampler(&ret, width, height, format, 6);
  return ret;
}

bfTextureCreateParams bfTextureCreateParams_initColorAttachment(uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient)
{
  bfTextureCreateParams ret;
  setupAttachment(&ret, width, height, format, can_be_input, is_transient);
  ret.flags |= BIFROST_TEX_IS_COLOR_ATTACHMENT;
  return ret;
}

bfTextureCreateParams bfTextureCreateParams_initDepthAttachment(uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient)
{
  bfTextureCreateParams ret;
  setupAttachment(&ret, width, height, format, can_be_input, is_transient);
  ret.flags |= BIFROST_TEX_IS_DEPTH_ATTACHMENT;
  return ret;
}

bfRenderpassInfo bfRenderpassInfo_init(uint16_t num_subpasses)
{
  assert(num_subpasses < BIFROST_GFX_RENDERPASS_MAX_SUBPASSES);

  bfRenderpassInfo ret;
  memset(&ret, 0x0, sizeof(ret));

  ret.load_ops          = 0x0000;
  ret.stencil_load_ops  = 0x0000;
  ret.clear_ops         = 0x0000;
  ret.stencil_clear_ops = 0x0000;
  ret.store_ops         = 0x0000;
  ret.stencil_store_ops = 0x0000;
  ret.num_subpasses     = num_subpasses;
  // ret.subpasses[BIFROST_GFX_RENDERPASS_MAX_SUBPASSES];
  ret.num_attachments = 0;
  // ret.attachments[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  // ret.depth_attachment;
  ret.num_dependencies = 0;
  // ret.dependencies[BIFROST_GFX_RENDERPASS_MAX_DEPENDENCIES];

  for (uint16_t i = 0; i < num_subpasses; ++i)
  {
    ret.subpasses[i].depth_attachment.attachment_index = UINT32_MAX;
  }

  return ret;
}

void bfRenderpassInfo_setLoadOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask)
{
  self->load_ops = attachment_mask;
}

void bfRenderpassInfo_setStencilLoadOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask)
{
  self->stencil_load_ops = attachment_mask;
}

void bfRenderpassInfo_setClearOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask)
{
  self->clear_ops = attachment_mask;
}

void bfRenderpassInfo_setStencilClearOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask)
{
  self->stencil_clear_ops = attachment_mask;
}

void bfRenderpassInfo_setStoreOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask)
{
  self->store_ops = attachment_mask;
}

void bfRenderpassInfo_setStencilStoreOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask)
{
  self->stencil_store_ops = attachment_mask;
}

void bfRenderpassInfo_addAttachment(bfRenderpassInfo* self, const bfAttachmentInfo* info)
{
  self->attachments[self->num_attachments++] = *info;
}

static bfSubpassCache* grabSubpass(bfRenderpassInfo* self, uint16_t subpass_index);

void bfRenderpassInfo_addColorOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, BifrostImageLayout layout)
{
  bfSubpassCache* const       subpass        = grabSubpass(self, subpass_index);
  bfAttachmentRefCache* const attachment_ref = subpass->out_attachment_refs + subpass->num_out_attachment_refs;

  assert((subpass->num_out_attachment_refs + 1) < BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS);

  attachment_ref->attachment_index = attachment;
  attachment_ref->layout           = layout;

  ++subpass->num_out_attachment_refs;
}

void bfRenderpassInfo_addDepthOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, BifrostImageLayout layout)
{
  bfSubpassCache* const       subpass        = grabSubpass(self, subpass_index);
  bfAttachmentRefCache* const attachment_ref = &subpass->depth_attachment;

  attachment_ref->attachment_index = attachment;
  attachment_ref->layout           = layout;
}

void bfRenderpassInfo_addInput(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment)
{
  bfSubpassCache* const       subpass        = grabSubpass(self, subpass_index);
  bfAttachmentRefCache* const attachment_ref = subpass->in_attachment_refs + subpass->num_in_attachment_refs;

  assert(subpass->num_in_attachment_refs + 1 < BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS);

  attachment_ref->attachment_index = attachment;
  attachment_ref->layout           = self->attachments[attachment].texture->tex_layout;  // TODO: This is bad

  ++subpass->num_in_attachment_refs;
}

void bfRenderpassInfo_addDependencies(bfRenderpassInfo* self, const bfSubpassDependency* dependencies, size_t num_dependencies)
{
  assert((self->num_dependencies + num_dependencies) < BIFROST_GFX_RENDERPASS_MAX_DEPENDENCIES);

  memcpy(self->dependencies, dependencies, num_dependencies * sizeof(bfSubpassDependency));
  self->num_dependencies += num_dependencies;
}

static bfSubpassCache* grabSubpass(bfRenderpassInfo* self, uint16_t subpass_index)
{
  assert(subpass_index < self->num_subpasses);
  return self->subpasses + subpass_index;
}

static void setupSampler(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, uint32_t num_layers)
{
  self->type             = BIFROST_TEX_TYPE_2D;
  self->format           = format;
  self->width            = width;
  self->height           = height;
  self->depth            = 1;
  self->generate_mipmaps = bfTrue;
  self->num_layers       = num_layers;
  self->flags            = BIFROST_TEX_IS_TRANSFER_DST | BIFROST_TEX_IS_SAMPLED;
}

static void setupAttachment(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient)
{
  self->type             = BIFROST_TEX_TYPE_2D;
  self->format           = format;
  self->width            = width;
  self->height           = height;
  self->depth            = 1;
  self->generate_mipmaps = bfFalse;
  self->num_layers       = 1;
  self->flags            = 0x0;

  if (can_be_input)
  {
    self->flags |= BIFROST_TEX_IS_INPUT_ATTACHMENT;
  }

  if (is_transient)
  {
    self->flags |= BIFROST_TEX_IS_TRANSIENT;
  }
}
