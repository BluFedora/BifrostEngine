/******************************************************************************/
/*!
 * @file   bf_gfx_api.h
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Outlines the cross platform C-API for low level graphics.
 *
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020-2021
 */
/******************************************************************************/
#ifndef BF_GFX_API_H
#define BF_GFX_API_H

#include "bf/Core.h"               /* bfBool32, bfBit */
#include "bf_gfx_export.h"         /* BF_GFX_API      */
#include "bf_gfx_handle.h"         /*                 */
#include "bf_gfx_limits.h"         /*                 */
#include "bf_gfx_pipeline_state.h" /*                 */
#include "bf_gfx_resource.h"       /*                 */
#include "bf_gfx_types.h"          /*                 */

#include <stdint.h> /* uint32_t */

#if __cplusplus
extern "C" {
#endif
/* clang-format off */
#define k_bfBufferWholeSize    ((uint64_t)~0ull)
#define k_bfTextureUnknownSize ((uint32_t)-1)
#define k_bfSubpassExternal    (~0U)
/* clang-format on */

/* Forward Declarations */
struct bfWindow;
typedef struct bfRenderpassInfo bfRenderpassCreateParams;

typedef struct bfTextureSamplerProperties
{
  bfTexSamplerFilterMode  min_filter;
  bfTexSamplerFilterMode  mag_filter;
  bfTexSamplerAddressMode u_address;
  bfTexSamplerAddressMode v_address;
  bfTexSamplerAddressMode w_address;
  float                   min_lod;
  float                   max_lod;

} bfTextureSamplerProperties;

BF_GFX_API bfTextureSamplerProperties bfTextureSamplerProperties_init(bfTexSamplerFilterMode filter, bfTexSamplerAddressMode uv_addressing);

/* Create Params */

typedef struct bfGfxContextCreateParams
{
  const char* app_name;
  uint32_t    app_version;

} bfGfxContextCreateParams;

/* Same as Vulkan's versioning system */
#define bfGfxMakeVersion(major, minor, patch) \
  (((major) << 22) | ((minor) << 12) | (patch))

typedef struct bfAllocationCreateInfo
{
  bfBufferSize         size;
  bfBufferPropertyBits properties;

} bfAllocationCreateInfo;

typedef struct bfBufferCreateParams
{
  bfAllocationCreateInfo allocation;
  bfBufferUsageBits      usage;

} bfBufferCreateParams;

typedef struct bfShaderProgramCreateParams
{
  const char* debug_name;
  uint32_t    num_desc_sets;

} bfShaderProgramCreateParams;

typedef struct bfTextureCreateParams
{
  bfTextureType        type;
  bfGfxImageFormat     format;
  uint32_t             width;
  uint32_t             height;
  uint32_t             depth;
  bfBool32             generate_mipmaps;
  uint32_t             num_layers;
  bfTexFeatureFlags    flags;
  bfGfxSampleFlags     sample_count;
  bfBufferPropertyBits memory_properties;

} bfTextureCreateParams;

BF_GFX_API bfTextureCreateParams bfTextureCreateParams_init2D(bfGfxImageFormat format, uint32_t width, uint32_t height);
BF_GFX_API bfTextureCreateParams bfTextureCreateParams_initCubeMap(uint32_t width, uint32_t height, bfGfxImageFormat format);
BF_GFX_API bfTextureCreateParams bfTextureCreateParams_initColorAttachment(uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);
BF_GFX_API bfTextureCreateParams bfTextureCreateParams_initDepthAttachment(uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

typedef enum bfStencilFace
{
  BF_STENCIL_FACE_FRONT,
  BF_STENCIL_FACE_BACK,

} bfStencilFace;

typedef struct bfGfxFrameInfo
{
  uint32_t frame_index;
  uint32_t frame_count;
  uint32_t num_frame_indices;

} bfGfxFrameInfo;

/* 
  Context
    The graphics context is global so only one needs to be created per process.
*/
BF_GFX_API void                   bfGfxInit(const bfGfxContextCreateParams* params);
BF_GFX_API bfGfxDeviceHandle      bfGfxGetDevice(void);
BF_GFX_API bfGfxFrameInfo         bfGfxGetFrameInfo(void);
BF_GFX_API bfWindowSurfaceHandle  bfGfxCreateWindow(struct bfWindow* bf_window);
BF_GFX_API bfBool32               bfGfxBeginFrame(bfWindowSurfaceHandle window);
BF_GFX_API bfGfxCommandListHandle bfGfxRequestCommandList(bfWindowSurfaceHandle window, uint32_t thread_index);
BF_GFX_API void                   bfGfxEndFrame(void);
BF_GFX_API void                   bfGfxDestroyWindow(bfWindowSurfaceHandle window_handle);
BF_GFX_API void                   bfGfxDestroy(void);

/* Logical Device */
typedef struct bfDeviceLimits
{
  bfBufferSize uniform_buffer_offset_alignment; /*!< Worst case is 256 (0x100) */

} bfDeviceLimits;

BF_GFX_API void                  bfGfxDevice_flush(bfGfxDeviceHandle self);
BF_GFX_API bfBufferHandle        bfGfxDevice_newBuffer(bfGfxDeviceHandle self, const bfBufferCreateParams* params);
BF_GFX_API bfRenderpassHandle    bfGfxDevice_newRenderpass(bfGfxDeviceHandle self, const bfRenderpassCreateParams* params);
BF_GFX_API bfShaderModuleHandle  bfGfxDevice_newShaderModule(bfGfxDeviceHandle self, bfShaderType type);
BF_GFX_API bfShaderProgramHandle bfGfxDevice_newShaderProgram(bfGfxDeviceHandle self, const bfShaderProgramCreateParams* params);
BF_GFX_API bfTextureHandle       bfGfxDevice_newTexture(bfGfxDeviceHandle self, const bfTextureCreateParams* params);
BF_GFX_API bfTextureHandle       bfGfxDevice_requestSurface(bfWindowSurfaceHandle window);
BF_GFX_API bfDeviceLimits        bfGfxDevice_limits(bfGfxDeviceHandle self);

/*!
 * @brief
 *   Freeing a null handle is valid.
 *
 * @param self
 *   The device the resource belongs to.
 *
 * @param resource
 *   The resource that you want to free.
*/
BF_GFX_API void bfGfxDevice_release(bfGfxDeviceHandle self, bfGfxBaseHandle resource);

/* Buffer */

BF_GFX_API bfBufferSize bfBuffer_size(bfBufferHandle self);
BF_GFX_API bfBufferSize bfBuffer_offset(bfBufferHandle self);
BF_GFX_API void*        bfBuffer_mappedPtr(bfBufferHandle self);
BF_GFX_API void*        bfBuffer_map(bfBufferHandle self, bfBufferSize offset, bfBufferSize size);
BF_GFX_API void         bfBuffer_invalidateRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges);
BF_GFX_API void         bfBuffer_invalidateRange(bfBufferHandle self, bfBufferSize offset, bfBufferSize size);
BF_GFX_API void         bfBuffer_copyCPU(bfBufferHandle self, bfBufferSize dst_offset, const void* data, bfBufferSize num_bytes);
BF_GFX_API void         bfBuffer_copyGPU(bfBufferHandle src, bfBufferSize src_offset, bfBufferHandle dst, bfBufferSize dst_offset, bfBufferSize num_bytes);
BF_GFX_API void         bfBuffer_flushRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges);
BF_GFX_API void         bfBuffer_flushRange(bfBufferHandle self, bfBufferSize offset, bfBufferSize size);
BF_GFX_API void         bfBuffer_unMap(bfBufferHandle self);

/* Vertex Binding */

BF_GFX_API bfVertexLayoutSetHandle bfVertexLayout_new(void);
BF_GFX_API void                    bfVertexLayout_addVertexBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t sizeof_vertex);
BF_GFX_API void                    bfVertexLayout_addInstanceBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t stride);
BF_GFX_API void                    bfVertexLayout_addVertexLayout(bfVertexLayoutSetHandle self, uint32_t binding, bfGfxVertexFormatAttribute format, uint32_t offset);
BF_GFX_API void                    bfVertexLayout_delete(bfVertexLayoutSetHandle self);

/* Renderpass */

BF_GFX_API bfRenderpassInfo bfRenderpassInfo_init(uint16_t num_subpasses);
BF_GFX_API void             bfRenderpassInfo_setLoadOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
BF_GFX_API void             bfRenderpassInfo_setStencilLoadOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
BF_GFX_API void             bfRenderpassInfo_setClearOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
BF_GFX_API void             bfRenderpassInfo_setStencilClearOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
BF_GFX_API void             bfRenderpassInfo_setStoreOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
BF_GFX_API void             bfRenderpassInfo_setStencilStoreOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
BF_GFX_API void             bfRenderpassInfo_addAttachment(bfRenderpassInfo* self, const bfAttachmentInfo* info);
BF_GFX_API void             bfRenderpassInfo_addColorOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, bfGfxImageLayout layout);
BF_GFX_API void             bfRenderpassInfo_addDepthOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, bfGfxImageLayout layout);
BF_GFX_API void             bfRenderpassInfo_addInput(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment);
BF_GFX_API void             bfRenderpassInfo_addDependencies(bfRenderpassInfo* self, const bfSubpassDependency* dependencies, uint32_t num_dependencies);

/* Shader Program / Module */

BF_GFX_API bfShaderType          bfShaderModule_type(bfShaderModuleHandle self);
BF_GFX_API bfBool32              bfShaderModule_loadFile(bfShaderModuleHandle self, const char* file);
BF_GFX_API bfBool32              bfShaderModule_loadData(bfShaderModuleHandle self, const char* source, size_t source_length);
BF_GFX_API void                  bfShaderProgram_addModule(bfShaderProgramHandle self, bfShaderModuleHandle module);
BF_GFX_API void                  bfShaderProgram_link(bfShaderProgramHandle self);
BF_GFX_API void                  bfShaderProgram_addAttribute(bfShaderProgramHandle self, const char* name, uint32_t binding);
BF_GFX_API void                  bfShaderProgram_addUniformBuffer(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, bfShaderStageBits stages);
BF_GFX_API void                  bfShaderProgram_addImageSampler(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, bfShaderStageBits stages);
BF_GFX_API void                  bfShaderProgram_compile(bfShaderProgramHandle self);
BF_GFX_API bfDescriptorSetHandle bfShaderProgram_createDescriptorSet(bfShaderProgramHandle self, uint32_t index);

/* Descriptor Set
     The Descriptor Set API is for 'Immutable' Bindings otherwise use the bfDescriptorSetInfo API 
*/

BF_GFX_API void bfDescriptorSet_setCombinedSamplerTextures(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures);
BF_GFX_API void bfDescriptorSet_setUniformBuffers(bfDescriptorSetHandle self, uint32_t binding, const bfBufferSize* offsets, const bfBufferSize* sizes, bfBufferHandle* buffers, uint32_t num_buffers);
BF_GFX_API void bfDescriptorSet_flushWrites(bfDescriptorSetHandle self);

BF_GFX_API bfDescriptorSetInfo bfDescriptorSetInfo_make(void);
BF_GFX_API void                bfDescriptorSetInfo_addTexture(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, const bfTextureHandle* textures, uint32_t num_textures);
BF_GFX_API void                bfDescriptorSetInfo_addUniform(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, const uint64_t* offsets, const uint64_t* sizes, bfBufferHandle* buffers, uint32_t num_buffers);

/* Texture */

BF_GFX_API uint32_t         bfTexture_width(bfTextureHandle self);
BF_GFX_API uint32_t         bfTexture_height(bfTextureHandle self);
BF_GFX_API uint32_t         bfTexture_depth(bfTextureHandle self);
BF_GFX_API uint32_t         bfTexture_numMipLevels(bfTextureHandle self);
BF_GFX_API bfGfxImageLayout bfTexture_layout(bfTextureHandle self);
BF_GFX_API bfBool32         bfTexture_loadFile(bfTextureHandle self, const char* file);
BF_GFX_API bfBool32         bfTexture_loadPNG(bfTextureHandle self, const void* png_bytes, size_t png_bytes_length);
BF_GFX_API bfBool32         bfTexture_loadData(bfTextureHandle self, const void* pixels, size_t pixels_length);
BF_GFX_API bfBool32         bfTexture_loadDataRange(bfTextureHandle self, const void* pixels, size_t pixels_length, const int32_t offset[3], const uint32_t sizes[3]);
BF_GFX_API void             bfTexture_loadBuffer(bfTextureHandle self, bfBufferHandle buffer, const int32_t offset[3], const uint32_t sizes[3]);
BF_GFX_API void             bfTexture_setSampler(bfTextureHandle self, const bfTextureSamplerProperties* sampler_properties);

/* Command List */

typedef enum bfPipelineBarrierType
{
  BF_PIPELINE_BARRIER_MEMORY,
  BF_PIPELINE_BARRIER_BUFFER,
  BF_PIPELINE_BARRIER_IMAGE,

} bfPipelineBarrierType;

typedef struct bfPipelineBarrier
{
  bfPipelineBarrierType type;
  bfGfxAccessFlagsBits  access[2];         /*!< [src, dst]                             */
  bfGfxQueueType        queue_transfer[2]; /*!< [old, new] For Buffer and Image types. */

  union
  {
    struct
    {
      bfBufferHandle handle;
      bfBufferSize   offset;
      bfBufferSize   size;

    } buffer;

    struct
    {
      bfTextureHandle  handle;
      bfGfxImageLayout layout_transition[2]; /*!< [old, new] */
      uint32_t         base_mip_level;
      uint32_t         level_count;
      uint32_t         base_array_layer;
      uint32_t         layer_count;

    } image;
  };

} bfPipelineBarrier;

BF_GFX_API bfPipelineBarrier bfPipelineBarrier_memory(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access);
BF_GFX_API bfPipelineBarrier bfPipelineBarrier_buffer(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size);
BF_GFX_API bfPipelineBarrier bfPipelineBarrier_image(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access, bfTextureHandle image, bfGfxImageLayout new_layout);

BF_GFX_API bfWindowSurfaceHandle bfGfxCmdList_window(bfGfxCommandListHandle self);

BF_GFX_API void     bfGfxCmdList_setDefaultPipeline(bfGfxCommandListHandle self);
BF_GFX_API bfBool32 bfGfxCmdList_begin(bfGfxCommandListHandle self);  // Returns True if no error
BF_GFX_API void     bfGfxCmdList_executionBarrier(bfGfxCommandListHandle self, bfGfxPipelineStageBits src_stage, bfGfxPipelineStageBits dst_stage, bfBool32 reads_same_pixel);
BF_GFX_API void     bfGfxCmdList_pipelineBarriers(bfGfxCommandListHandle self, bfGfxPipelineStageBits src_stage, bfGfxPipelineStageBits dst_stage, const bfPipelineBarrier* barriers, uint32_t num_barriers, bfBool32 reads_same_pixel);
BF_GFX_API void     bfGfxCmdList_setRenderpass(bfGfxCommandListHandle self, bfRenderpassHandle renderpass);
BF_GFX_API void     bfGfxCmdList_setRenderpassInfo(bfGfxCommandListHandle self, const bfRenderpassInfo* renderpass_info);
BF_GFX_API void     bfGfxCmdList_setClearValues(bfGfxCommandListHandle self, const bfClearValue* clear_values);
BF_GFX_API void     bfGfxCmdList_setAttachments(bfGfxCommandListHandle self, bfTextureHandle* attachments);
BF_GFX_API void     bfGfxCmdList_setRenderAreaAbs(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height);  // 0 => AttachmentWidth, 0 => AttachmentHeight
BF_GFX_API void     bfGfxCmdList_setRenderAreaRel(bfGfxCommandListHandle self, float x, float y, float width, float height);            // Normalized [0.0f, 1.0f] coords
BF_GFX_API void     bfGfxCmdList_beginRenderpass(bfGfxCommandListHandle self);
BF_GFX_API void     bfGfxCmdList_nextSubpass(bfGfxCommandListHandle self);
BF_GFX_API void     bfGfxCmdList_setDrawMode(bfGfxCommandListHandle self, bfDrawMode draw_mode);
BF_GFX_API void     bfGfxCmdList_setFrontFace(bfGfxCommandListHandle self, bfFrontFace front_face);
BF_GFX_API void     bfGfxCmdList_setCullFace(bfGfxCommandListHandle self, bfCullFaceFlags cull_face);
BF_GFX_API void     bfGfxCmdList_setDepthTesting(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setDepthWrite(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setDepthTestOp(bfGfxCommandListHandle self, bfCompareOp op);
BF_GFX_API void     bfGfxCmdList_setStencilTesting(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setPrimitiveRestart(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setRasterizerDiscard(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setDepthBias(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setSampleShading(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setAlphaToCoverage(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setAlphaToOne(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setLogicOpEnabled(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setLogicOp(bfGfxCommandListHandle self, bfLogicOp op);
BF_GFX_API void     bfGfxCmdList_setPolygonFillMode(bfGfxCommandListHandle self, bfPolygonFillMode fill_mode);
BF_GFX_API void     bfGfxCmdList_setColorWriteMask(bfGfxCommandListHandle self, uint32_t output_attachment_idx, uint8_t color_mask);
BF_GFX_API void     bfGfxCmdList_setColorBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendOp op);
BF_GFX_API void     bfGfxCmdList_setBlendSrc(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void     bfGfxCmdList_setBlendDst(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void     bfGfxCmdList_setAlphaBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendOp op);
BF_GFX_API void     bfGfxCmdList_setBlendSrcAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void     bfGfxCmdList_setBlendDstAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void     bfGfxCmdList_setStencilFailOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op);
BF_GFX_API void     bfGfxCmdList_setStencilPassOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op);
BF_GFX_API void     bfGfxCmdList_setStencilDepthFailOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op);
BF_GFX_API void     bfGfxCmdList_setStencilCompareOp(bfGfxCommandListHandle self, bfStencilFace face, bfCompareOp op);
BF_GFX_API void     bfGfxCmdList_setStencilCompareMask(bfGfxCommandListHandle self, bfStencilFace face, uint8_t cmp_mask);
BF_GFX_API void     bfGfxCmdList_setStencilWriteMask(bfGfxCommandListHandle self, bfStencilFace face, uint8_t write_mask);
BF_GFX_API void     bfGfxCmdList_setStencilReference(bfGfxCommandListHandle self, bfStencilFace face, uint8_t ref_mask);
BF_GFX_API void     bfGfxCmdList_setDynamicStates(bfGfxCommandListHandle self, uint16_t dynamic_states);
BF_GFX_API void     bfGfxCmdList_setViewport(bfGfxCommandListHandle self, float x, float y, float width, float height, const float depth[2]);
BF_GFX_API void     bfGfxCmdList_setScissorRect(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height);
BF_GFX_API void     bfGfxCmdList_setBlendConstants(bfGfxCommandListHandle self, const float constants[4]);
BF_GFX_API void     bfGfxCmdList_setLineWidth(bfGfxCommandListHandle self, float value);
BF_GFX_API void     bfGfxCmdList_setDepthClampEnabled(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setDepthBoundsTestEnabled(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void     bfGfxCmdList_setDepthBounds(bfGfxCommandListHandle self, float min, float max);
BF_GFX_API void     bfGfxCmdList_setDepthBiasConstantFactor(bfGfxCommandListHandle self, float value);
BF_GFX_API void     bfGfxCmdList_setDepthBiasClamp(bfGfxCommandListHandle self, float value);
BF_GFX_API void     bfGfxCmdList_setDepthBiasSlopeFactor(bfGfxCommandListHandle self, float value);
BF_GFX_API void     bfGfxCmdList_setMinSampleShading(bfGfxCommandListHandle self, float value);
BF_GFX_API void     bfGfxCmdList_setSampleMask(bfGfxCommandListHandle self, uint32_t sample_mask);
BF_GFX_API void     bfGfxCmdList_bindDrawCallPipeline(bfGfxCommandListHandle self, const bfDrawCallPipeline* pipeline_state);
BF_GFX_API void     bfGfxCmdList_bindVertexDesc(bfGfxCommandListHandle self, bfVertexLayoutSetHandle vertex_set_layout);
BF_GFX_API void     bfGfxCmdList_bindVertexBuffers(bfGfxCommandListHandle self, uint32_t first_binding, bfBufferHandle* buffers, uint32_t num_buffers, const uint64_t* offsets);
BF_GFX_API void     bfGfxCmdList_bindIndexBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, uint64_t offset, bfGfxIndexType idx_type);
BF_GFX_API void     bfGfxCmdList_bindProgram(bfGfxCommandListHandle self, bfShaderProgramHandle shader);
BF_GFX_API void     bfGfxCmdList_bindDescriptorSets(bfGfxCommandListHandle self, uint32_t binding, bfDescriptorSetHandle* desc_sets, uint32_t num_desc_sets);  // Call after pipeline is setup.
BF_GFX_API void     bfGfxCmdList_bindDescriptorSet(bfGfxCommandListHandle self, uint32_t set_index, const bfDescriptorSetInfo* desc_set_info);                 // Call after pipeline is setup.
BF_GFX_API void     bfGfxCmdList_draw(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices);                                              // Draw Cmds
BF_GFX_API void     bfGfxCmdList_drawInstanced(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices, uint32_t first_instance, uint32_t num_instances);
BF_GFX_API void     bfGfxCmdList_drawIndexed(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset);
BF_GFX_API void     bfGfxCmdList_drawIndexedInstanced(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset, uint32_t first_instance, uint32_t num_instances);
BF_GFX_API void     bfGfxCmdList_executeSubCommands(bfGfxCommandListHandle self, bfGfxCommandListHandle* commands, uint32_t num_commands);  // General
BF_GFX_API void     bfGfxCmdList_endRenderpass(bfGfxCommandListHandle self);
BF_GFX_API void     bfGfxCmdList_end(bfGfxCommandListHandle self);
BF_GFX_API void     bfGfxCmdList_updateBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size, const void* data);  // Outside Renderpass
BF_GFX_API void     bfGfxCmdList_submit(bfGfxCommandListHandle self);

#if __cplusplus
}
#endif

#endif /* BF_GFX_API_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
