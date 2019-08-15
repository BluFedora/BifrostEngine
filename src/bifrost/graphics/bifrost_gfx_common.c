#include "bifrost/graphics/bifrost_gfx_api.h"

static void setupSampler(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, uint32_t num_layers);
static void setupAttachment(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

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

static void setupSampler(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, uint32_t num_layers)
{
  self->type       = BIFROST_TEX_TYPE_2D;
  self->format     = format;
  self->width      = width;
  self->height     = height;
  self->depth      = 1;
  self->mip_levels = BIFROST_CALC_MIPMAP;
  self->num_layers = num_layers;
  self->flags      = BIFROST_TEX_IS_TRANSFER_DST | BIFROST_TEX_IS_SAMPLED;
}

static void setupAttachment(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient)
{
  self->type       = BIFROST_TEX_TYPE_2D;
  self->format     = format;
  self->width      = width;
  self->height     = height;
  self->depth      = 1;
  self->mip_levels = 1;
  self->num_layers = 1;
  self->flags      = 0x0;

  if (can_be_input)
  {
    self->flags |= BIFROST_TEX_IS_INPUT_ATTACHMENT;
  }

  if (is_transient)
  {
    self->flags |= BIFROST_TEX_IS_TRANSIENT;
  }
}
