#include "bifrost/graphics/bifrost_gfx_api.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#include <assert.h> /* assert         */
#include <stdlib.h> /* free           */
#include <string.h> /* memcpy, memset */

static void setupSampler(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, uint32_t num_layers);
static void setupAttachment(bfTextureCreateParams* self, uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

void BifrostGfxObjectBase_ctor(BifrostGfxObjectBase* self, BifrostGfxObjectType type)
{
  self->type            = type;
  self->next            = NULL;
  self->hash_code       = 0x0;
  self->last_frame_used = -1;
}

bfTextureSamplerProperties bfTextureSamplerProperties_init(BifrostSamplerFilterMode filter, BifrostSamplerAddressMode uv_addressing)
{
  bfTextureSamplerProperties props;

  props.min_filter = filter;
  props.mag_filter = filter;
  props.u_address  = uv_addressing;
  props.v_address  = uv_addressing;
  props.w_address  = uv_addressing;
  props.min_lod    = 0.0f;
  props.max_lod    = 1.0f;

  return props;
}

bfTextureCreateParams bfTextureCreateParams_init2D(BifrostImageFormat format, uint32_t width, uint32_t height)
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

void bfBuffer_invalidateRange(bfBufferHandle self, bfBufferSize offset, bfBufferSize size)
{
  bfBuffer_invalidateRanges(self, &offset, &size, 1);
}

void bfBuffer_flushRange(bfBufferHandle self, bfBufferSize offset, bfBufferSize size)
{
  bfBuffer_flushRanges(self, &offset, &size, 1);
}

bfBool32 bfShaderModule_loadFile(bfShaderModuleHandle self, const char* file)
{
  long  buffer_size;
  char* buffer = LoadFileIntoMemory(file, &buffer_size);

  if (buffer)
  {
    const bfBool32 was_successful = bfShaderModule_loadData(self, buffer, buffer_size);
    free(buffer);
    return was_successful;
  }

  printf("Failed to load file %s\n", file);
  return bfFalse;
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
  bfSubpassCache* const subpass = grabSubpass(self, subpass_index);

  assert(subpass->num_out_attachment_refs < BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS);

  bfAttachmentRefCache* const attachment_ref = subpass->out_attachment_refs + subpass->num_out_attachment_refs;

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
  attachment_ref->layout           = bfTexture_layout(self->attachments[attachment].texture);

  ++subpass->num_in_attachment_refs;
}

void bfRenderpassInfo_addDependencies(bfRenderpassInfo* self, const bfSubpassDependency* dependencies, uint32_t num_dependencies)
{
  assert((self->num_dependencies + num_dependencies) < BIFROST_GFX_RENDERPASS_MAX_DEPENDENCIES);

  memcpy(self->dependencies, dependencies, num_dependencies * sizeof(bfSubpassDependency));
  self->num_dependencies += num_dependencies;
}

bfDescriptorSetInfo bfDescriptorSetInfo_make(void)
{
  bfDescriptorSetInfo self;
  self.num_bindings = 0;

  return self;
}

void bfDescriptorSetInfo_addTexture(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures)
{
  assert(self->num_bindings < BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS);

  self->bindings[self->num_bindings].type                = BIFROST_DESCRIPTOR_ELEMENT_TEXTURE;
  self->bindings[self->num_bindings].binding             = binding;
  self->bindings[self->num_bindings].array_element_start = array_element_start;
  self->bindings[self->num_bindings].num_handles         = num_textures;

  for (uint32_t i = 0; i < num_textures; ++i)
  {
    self->bindings[self->num_bindings].handles[i] = textures[i];
    self->bindings[self->num_bindings].offsets[i] = 0;
    self->bindings[self->num_bindings].sizes[i]   = 0;
  }

  ++self->num_bindings;
}

void bfDescriptorSetInfo_addUniform(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, const uint64_t* offsets, const uint64_t* sizes, bfBufferHandle* buffers, uint32_t num_buffers)
{
  assert(self->num_bindings < BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS);

  self->bindings[self->num_bindings].type                = BIFROST_DESCRIPTOR_ELEMENT_BUFFER;
  self->bindings[self->num_bindings].binding             = binding;
  self->bindings[self->num_bindings].array_element_start = array_element_start;
  self->bindings[self->num_bindings].num_handles         = num_buffers;

  for (uint32_t i = 0; i < num_buffers; ++i)
  {
    self->bindings[self->num_bindings].handles[i] = buffers[i];
    self->bindings[self->num_bindings].offsets[i] = offsets[i];
    self->bindings[self->num_bindings].sizes[i]   = sizes[i];
  }

  ++self->num_bindings;
}

void bfGfxCmdList_executionBarrier(bfGfxCommandListHandle self, BifrostPipelineStageBits src_stage, BifrostPipelineStageBits dst_stage, bfBool32 reads_same_pixel)
{
  bfGfxCmdList_pipelineBarriers(self, src_stage, dst_stage, NULL, 0, reads_same_pixel);
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
    self->flags |= BIFROST_TEX_IS_INPUT_ATTACHMENT | BIFROST_TEX_IS_SAMPLED;
  }

  if (is_transient)
  {
    self->flags |= BIFROST_TEX_IS_TRANSIENT;
  }
}

void bfGfxCmdList_setDefaultPipeline(bfGfxCommandListHandle self)
{
  bfGfxCmdList_setDrawMode(self, BIFROST_DRAW_MODE_TRIANGLE_LIST);
  bfGfxCmdList_setFrontFace(self, BIFROST_FRONT_FACE_CCW);
  bfGfxCmdList_setCullFace(self, BIFROST_CULL_FACE_NONE);
  bfGfxCmdList_setDepthTesting(self, bfFalse);
  bfGfxCmdList_setDepthWrite(self, bfFalse);
  bfGfxCmdList_setDepthTestOp(self, BIFROST_COMPARE_OP_ALWAYS);
  bfGfxCmdList_setStencilTesting(self, bfFalse);
  bfGfxCmdList_setPrimitiveRestart(self, bfFalse);
  bfGfxCmdList_setRasterizerDiscard(self, bfFalse);
  bfGfxCmdList_setDepthBias(self, bfFalse);
  bfGfxCmdList_setSampleShading(self, bfFalse);
  bfGfxCmdList_setAlphaToCoverage(self, bfFalse);
  bfGfxCmdList_setAlphaToOne(self, bfFalse);
  bfGfxCmdList_setLogicOp(self, BIFROST_LOGIC_OP_CLEAR);
  bfGfxCmdList_setPolygonFillMode(self, BIFROST_POLYGON_MODE_FILL);

  for (int i = 0; i < BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS; ++i)
  {
    bfGfxCmdList_setColorWriteMask(self, i, BIFROST_COLOR_MASK_RGBA);
    bfGfxCmdList_setColorBlendOp(self, i, BIFROST_BLEND_OP_ADD);
    bfGfxCmdList_setBlendSrc(self, i, BIFROST_BLEND_FACTOR_SRC_ALPHA);
    bfGfxCmdList_setBlendDst(self, i, BIFROST_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    bfGfxCmdList_setAlphaBlendOp(self, i, BIFROST_BLEND_OP_ADD);
    // bfGfxCmdList_setBlendSrcAlpha(self, i, BIFROST_BLEND_FACTOR_ONE);
    // bfGfxCmdList_setBlendDstAlpha(self, i, BIFROST_BLEND_FACTOR_ZERO);
    bfGfxCmdList_setBlendSrcAlpha(self, i, BIFROST_BLEND_FACTOR_SRC_ALPHA);
    bfGfxCmdList_setBlendDstAlpha(self, i, BIFROST_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
  }

  for (BifrostStencilFace face = BIFROST_STENCIL_FACE_FRONT; face <= BIFROST_STENCIL_FACE_BACK; face = (BifrostStencilFace)(face + 1))
  {
    bfGfxCmdList_setStencilFailOp(self, face, BIFROST_STENCIL_OP_KEEP);
    bfGfxCmdList_setStencilPassOp(self, face, BIFROST_STENCIL_OP_REPLACE);
    bfGfxCmdList_setStencilDepthFailOp(self, face, BIFROST_STENCIL_OP_KEEP);
    bfGfxCmdList_setStencilCompareOp(self, face, BIFROST_COMPARE_OP_ALWAYS);
    bfGfxCmdList_setStencilCompareMask(self, face, 0xFF);
    bfGfxCmdList_setStencilWriteMask(self, face, 0xFF);
    bfGfxCmdList_setStencilReference(self, face, 0xFF);
  }

  bfGfxCmdList_setDynamicStates(self, BIFROST_PIPELINE_DYNAMIC_NONE);
  const float depths[] = {0.0f, 1.0f};
  bfGfxCmdList_setViewport(self, 0.0f, 0.0f, 0.0f, 0.0f, depths);
  bfGfxCmdList_setScissorRect(self, 0, 0, 1, 1);
  const float blend_constants[] = {1.0f, 1.0f, 1.0f, 1.0f};
  bfGfxCmdList_setBlendConstants(self, blend_constants);
  bfGfxCmdList_setLineWidth(self, 1.0f);
  bfGfxCmdList_setDepthClampEnabled(self, bfFalse);
  bfGfxCmdList_setDepthBoundsTestEnabled(self, bfFalse);
  bfGfxCmdList_setDepthBounds(self, 0.0f, 1.0f);
  bfGfxCmdList_setDepthBiasConstantFactor(self, 0.0f);
  bfGfxCmdList_setDepthBiasClamp(self, 0.0f);
  bfGfxCmdList_setDepthBiasSlopeFactor(self, 0.0f);
  bfGfxCmdList_setMinSampleShading(self, 0.0f);
  bfGfxCmdList_setSampleMask(self, 0xFFFFFFFF);
}

void bfGfxCmdList_setRenderAreaRelImpl(bfTextureHandle texture, bfGfxCommandListHandle self, float x, float y, float width, float height)
{
  // TODO: Clamp the paramters or emit an error.

  const int32_t fb_width  = bfTexture_width(texture);
  const int32_t fb_height = bfTexture_height(texture);

  bfGfxCmdList_setRenderAreaAbs(
   self,
   (int32_t)(fb_width * x),
   (int32_t)(fb_height * y),
   (uint32_t)(fb_width * width),
   (uint32_t)(fb_height * height));
}

#include <stdio.h>

char* LoadFileIntoMemory(const char* const filename, long* out_size)
{
  FILE* f      = fopen(filename, "rb");  // NOLINT(android-cloexec-fopen)
  char* buffer = NULL;

  if (f)
  {
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    buffer = (char*)(malloc(file_size + 1));
    if (buffer)
    {
      fread(buffer, sizeof(char), file_size, f);
      buffer[file_size] = '\0';
    }
    else
    {
      file_size = 0;
    }

    *out_size = file_size;
    fclose(f);
  }

  return buffer;
}