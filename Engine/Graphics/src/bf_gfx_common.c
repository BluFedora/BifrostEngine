#include "bf/bf_gfx_api.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#include <assert.h> /* assert         */
#include <stdlib.h> /* free           */
#include <string.h> /* memcpy, memset */

static void setupSampler(bfTextureCreateParams* self, uint32_t width, uint32_t height, bfGfxImageFormat format, uint32_t num_layers);
static void setupAttachment(bfTextureCreateParams* self, uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

void bfBaseGfxObject_ctor(bfBaseGfxObject* self, bfGfxObjectType type)
{
  self->type            = type;
  self->next            = NULL;
  self->hash_code       = 0x0;
  self->last_frame_used = -1;
}

bfTextureSamplerProperties bfTextureSamplerProperties_init(bfTexSamplerFilterMode filter, bfTexSamplerAddressMode uv_addressing)
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

bfTextureCreateParams bfTextureCreateParams_init2D(bfGfxImageFormat format, uint32_t width, uint32_t height)
{
  bfTextureCreateParams ret;
  setupSampler(&ret, width, height, format, 1);
  return ret;
}

bfTextureCreateParams bfTextureCreateParams_initCubeMap(uint32_t width, uint32_t height, bfGfxImageFormat format)
{
  bfTextureCreateParams ret;
  setupSampler(&ret, width, height, format, 6);
  return ret;
}

bfTextureCreateParams bfTextureCreateParams_initColorAttachment(uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient)
{
  bfTextureCreateParams ret;
  setupAttachment(&ret, width, height, format, can_be_input, is_transient);
  ret.flags |= BF_TEX_IS_COLOR_ATTACHMENT;
  return ret;
}

bfTextureCreateParams bfTextureCreateParams_initDepthAttachment(uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient)
{
  bfTextureCreateParams ret;
  setupAttachment(&ret, width, height, format, can_be_input, is_transient);
  ret.flags |= BF_TEX_IS_DEPTH_ATTACHMENT;
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

bfBool32 bfTexture_loadData(bfTextureHandle self, const void* pixels, size_t pixels_length)
{
  const int32_t  offset[3] = {0u, 0u, 0u};
  const uint32_t sizes[3]  = {bfTexture_width(self), bfTexture_height(self), bfTexture_depth(self)};

  return bfTexture_loadDataRange(self, pixels, pixels_length, offset, sizes);
}

bfRenderpassInfo bfRenderpassInfo_init(uint16_t num_subpasses)
{
  assert(num_subpasses < k_bfGfxMaxSubpasses);

  bfRenderpassInfo ret;
  memset(&ret, 0x0, sizeof(ret));

  ret.load_ops          = 0x0000;
  ret.stencil_load_ops  = 0x0000;
  ret.clear_ops         = 0x0000;
  ret.stencil_clear_ops = 0x0000;
  ret.store_ops         = 0x0000;
  ret.stencil_store_ops = 0x0000;
  ret.num_subpasses     = num_subpasses;
  // ret.subpasses[k_bfGfxMaxSubpasses];
  ret.num_attachments = 0;
  // ret.attachments[k_bfGfxMaxAttachments];
  // ret.depth_attachment;
  ret.num_dependencies = 0;
  // ret.dependencies[k_bfGfxMaxRenderpassDependencies];

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

void bfRenderpassInfo_addColorOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, bfGfxImageLayout layout)
{
  bfSubpassCache* const subpass = grabSubpass(self, subpass_index);

  assert(subpass->num_out_attachment_refs < k_bfGfxMaxAttachments);

  bfAttachmentRefCache* const attachment_ref = subpass->out_attachment_refs + subpass->num_out_attachment_refs;

  attachment_ref->attachment_index = attachment;
  attachment_ref->layout           = layout;

  ++subpass->num_out_attachment_refs;
}

void bfRenderpassInfo_addDepthOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, bfGfxImageLayout layout)
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

  assert(subpass->num_in_attachment_refs + 1 < k_bfGfxMaxAttachments);

  attachment_ref->attachment_index = attachment;
  attachment_ref->layout           = bfTexture_layout(self->attachments[attachment].texture);

  ++subpass->num_in_attachment_refs;
}

void bfRenderpassInfo_addDependencies(bfRenderpassInfo* self, const bfSubpassDependency* dependencies, uint32_t num_dependencies)
{
  assert((self->num_dependencies + num_dependencies) < k_bfGfxMaxRenderpassDependencies);

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
  assert(self->num_bindings < k_bfGfxDesfcriptorSetMaxLayoutBindings);

  self->bindings[self->num_bindings].type                = BF_DESCRIPTOR_ELEMENT_TEXTURE;
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
  assert(self->num_bindings < k_bfGfxDesfcriptorSetMaxLayoutBindings);

  self->bindings[self->num_bindings].type                = BF_DESCRIPTOR_ELEMENT_BUFFER;
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

void bfGfxCmdList_executionBarrier(bfGfxCommandListHandle self, bfGfxPipelineStageBits src_stage, bfGfxPipelineStageBits dst_stage, bfBool32 reads_same_pixel)
{
  bfGfxCmdList_pipelineBarriers(self, src_stage, dst_stage, NULL, 0, reads_same_pixel);
}

static bfSubpassCache* grabSubpass(bfRenderpassInfo* self, uint16_t subpass_index)
{
  assert(subpass_index < self->num_subpasses);
  return self->subpasses + subpass_index;
}

static void setupSampler(bfTextureCreateParams* self, uint32_t width, uint32_t height, bfGfxImageFormat format, uint32_t num_layers)
{
  self->type             = BF_TEX_TYPE_2D;
  self->format           = format;
  self->width            = width;
  self->height           = height;
  self->depth            = 1;
  self->generate_mipmaps = bfTrue;
  self->num_layers       = num_layers;
  self->flags            = BF_TEX_IS_TRANSFER_DST | BF_TEX_IS_SAMPLED;
  self->sample_count     = BF_SAMPLE_1;
}

static void setupAttachment(bfTextureCreateParams* self, uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient)
{
  self->type             = BF_TEX_TYPE_2D;
  self->format           = format;
  self->width            = width;
  self->height           = height;
  self->depth            = 1;
  self->generate_mipmaps = bfFalse;
  self->num_layers       = 1;
  self->flags            = 0x0;
  self->sample_count     = BF_SAMPLE_1;

  if (can_be_input)
  {
    self->flags |= BF_TEX_IS_INPUT_ATTACHMENT | BF_TEX_IS_SAMPLED;
  }

  if (is_transient)
  {
    self->flags |= BF_TEX_IS_TRANSIENT;
  }
}

static bfPipelineBarrier bfPipelineBarrier_makeBase(bfPipelineBarrierType type, bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access)
{
  bfPipelineBarrier result;

  memset(&result, 0x0, sizeof(bfPipelineBarrier));

  result.type              = type;
  result.access[0]         = src_access;
  result.access[1]         = dst_access;
  result.queue_transfer[0] = BF_GFX_QUEUE_IGNORE;
  result.queue_transfer[1] = BF_GFX_QUEUE_IGNORE;

  return result;
}

bfPipelineBarrier bfPipelineBarrier_memory(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access)
{
  return bfPipelineBarrier_makeBase(BF_PIPELINE_BARRIER_MEMORY, src_access, dst_access);
}

bfPipelineBarrier bfPipelineBarrier_buffer(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size)
{
  bfPipelineBarrier result = bfPipelineBarrier_makeBase(BF_PIPELINE_BARRIER_BUFFER, src_access, dst_access);

  result.info.buffer.handle = buffer;
  result.info.buffer.offset = offset;
  result.info.buffer.size   = size;

  // ReSharper disable once CppSomeObjectMembersMightNotBeInitialized
  return result;
}

bfPipelineBarrier bfPipelineBarrier_image(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access, bfTextureHandle image, bfGfxImageLayout new_layout)
{
  bfPipelineBarrier result = bfPipelineBarrier_makeBase(BF_PIPELINE_BARRIER_IMAGE, src_access, dst_access);

  result.info.image.handle               = image;
  result.info.image.layout_transition[0] = bfTexture_layout(image);
  result.info.image.layout_transition[1] = new_layout;
  result.info.image.base_mip_level       = 0;
  result.info.image.level_count          = bfTexture_numMipLevels(image);
  result.info.image.base_array_layer     = 0;
  result.info.image.layer_count          = bfTexture_depth(image);

  return result;
}

void bfGfxCmdList_setDefaultPipeline(bfGfxCommandListHandle self)
{
  bfGfxCmdList_setDrawMode(self, BF_DRAW_MODE_TRIANGLE_LIST);
  bfGfxCmdList_setFrontFace(self, BF_FRONT_FACE_CCW);
  bfGfxCmdList_setCullFace(self, BF_CULL_FACE_NONE);
  bfGfxCmdList_setDepthTesting(self, bfFalse);
  bfGfxCmdList_setDepthWrite(self, bfFalse);
  bfGfxCmdList_setDepthTestOp(self, BF_COMPARE_OP_ALWAYS);
  bfGfxCmdList_setStencilTesting(self, bfFalse);
  bfGfxCmdList_setPrimitiveRestart(self, bfFalse);
  bfGfxCmdList_setRasterizerDiscard(self, bfFalse);
  bfGfxCmdList_setDepthBias(self, bfFalse);
  bfGfxCmdList_setSampleShading(self, bfFalse);
  bfGfxCmdList_setAlphaToCoverage(self, bfFalse);
  bfGfxCmdList_setAlphaToOne(self, bfFalse);
  bfGfxCmdList_setLogicOp(self, BF_LOGIC_OP_CLEAR);
  bfGfxCmdList_setPolygonFillMode(self, BF_POLYGON_MODE_FILL);

  for (int i = 0; i < k_bfGfxMaxAttachments; ++i)
  {
    bfGfxCmdList_setColorWriteMask(self, i, BF_COLOR_MASK_RGBA);
    bfGfxCmdList_setColorBlendOp(self, i, BF_BLEND_OP_ADD);
    bfGfxCmdList_setBlendSrc(self, i, BF_BLEND_FACTOR_SRC_ALPHA);
    bfGfxCmdList_setBlendDst(self, i, BF_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    bfGfxCmdList_setAlphaBlendOp(self, i, BF_BLEND_OP_ADD);
    // bfGfxCmdList_setBlendSrcAlpha(self, i, BF_BLEND_FACTOR_ONE);
    // bfGfxCmdList_setBlendDstAlpha(self, i, BF_BLEND_FACTOR_ZERO);
    bfGfxCmdList_setBlendSrcAlpha(self, i, BF_BLEND_FACTOR_SRC_ALPHA);
    bfGfxCmdList_setBlendDstAlpha(self, i, BF_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
  }

  for (bfStencilFace face = BF_STENCIL_FACE_FRONT; face <= BF_STENCIL_FACE_BACK; face = (bfStencilFace)(face + 1))
  {
    bfGfxCmdList_setStencilFailOp(self, face, BF_STENCIL_OP_KEEP);
    bfGfxCmdList_setStencilPassOp(self, face, BF_STENCIL_OP_REPLACE);
    bfGfxCmdList_setStencilDepthFailOp(self, face, BF_STENCIL_OP_KEEP);
    bfGfxCmdList_setStencilCompareOp(self, face, BF_COMPARE_OP_ALWAYS);
    bfGfxCmdList_setStencilCompareMask(self, face, 0xFF);
    bfGfxCmdList_setStencilWriteMask(self, face, 0xFF);
    bfGfxCmdList_setStencilReference(self, face, 0xFF);
  }

  bfGfxCmdList_setDynamicStates(self, BF_PIPELINE_DYNAMIC_NONE);
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
  assert(x >= 0.0f && x <= 1.0f);
  assert(y >= 0.0f && y <= 1.0f);
  assert(width >= 0.0f && width <= 1.0f);
  assert(height >= 0.0f && height <= 1.0f);
  assert((x + width) >= 0.0f && (x + width) <= 1.0f);
  assert((y + height) >= 0.0f && (y + height) <= 1.0f);

  const float fb_width  = (float)(bfTexture_width(texture));
  const float fb_height = (float)(bfTexture_height(texture));

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
    fseek(f, 0, SEEK_SET);  // same as rewind(f);
    buffer = (char*)malloc(file_size + 1ul);
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

/* Prefer Dedicated GPU On Windows */
#ifdef _WIN32
#include <intsafe.h> /* DWORD */

#ifdef __cplusplus
extern "C" {
#endif

__declspec(dllexport) DWORD NvOptimusEnablement                = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
}
#endif

#endif  //  _WIN32
