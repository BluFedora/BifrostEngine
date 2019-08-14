// After the API is finalized then move to separate files.

// Dependencies
//  -> Core Bifrost Types
#ifndef BIFROST_GFX_API_H
#define BIFROST_GFX_API_H

#if BIFROST_GFX_RENDER_GRAPH_TEST
#include "../bifrost_std.h" /* bfBool32 */
#else
#include "bifrost/bifrost_std.h" /* bfBool32 */
#endif
#include "bifrost_gfx_pipeline_state.h" /* */
#include "bifrost_gfx_types.h"          /* */
#include <stdint.h>                     /* uint32_t */

#if __cplusplus
extern "C" {
#endif
#define BIFROST_DECLARE_HANDLE(T) \
  struct bf##T##_t;               \
  typedef struct bf##T##_t bf##T; \
  typedef bf##T*           bf##T##Handle

#define BIFROST_DEFINE_HANDLE(T) struct bf##T##_t

BIFROST_DECLARE_HANDLE(GfxContext);
BIFROST_DECLARE_HANDLE(GfxDevice);
BIFROST_DECLARE_HANDLE(GfxCommandList);
BIFROST_DECLARE_HANDLE(Buffer);
BIFROST_DECLARE_HANDLE(Framebuffer);
BIFROST_DECLARE_HANDLE(VertexLayoutSet);  // Maybe this should be a POD rather than a handle?
BIFROST_DECLARE_HANDLE(DescriptorSet);    // Maybe this should be a POD rather than a handle?
BIFROST_DECLARE_HANDLE(Renderpass);
BIFROST_DECLARE_HANDLE(ShaderModule);
BIFROST_DECLARE_HANDLE(ShaderProgram);
BIFROST_DECLARE_HANDLE(Texture);

#undef BIFROST_DECLARE_HANDLE

/* Context */
typedef uint64_t bfBufferSize;

/* Buffer */
typedef enum bfBufferPropertyFlags_t
{
  // NOTE(Shareef):
  //   Best for Device Access to the Memory.
  BIFROST_BPF_DEVICE_LOCAL = (1 << 0),
  // NOTE(Shareef):
  //   Can be mapped on the host.
  BIFROST_BPF_HOST_MAPPABLE = (1 << 1),
  // NOTE(Shareef):
  //   You don't need 'vkFlushMappedMemoryRanges' and 'vkInvalidateMappedMemoryRanges' anymore.
  BIFROST_BPF_HOST_CACHE_MANAGED = (1 << 2),
  // NOTE(Shareef):
  //   Always host coherent, cached on the host for increased host access speed.
  BIFROST_BPF_HOST_CACHED = (1 << 3),
  // NOTE(Shareef):
  //   Implementation defined lazy allocation of the buffer.
  //   use: vkGetDeviceMemoryCommitment
  //   CAN NOT HAVE SET: BPF_HOST_MAPPABLE
  BIFROST_BPF_DEVICE_LAZY_ALLOC = (1 << 4),
  // NOTE(Shareef):
  //   Only device accessible and allows protected queue operations.
  //   CAN NOT HAVE SET: BPF_HOST_MAPPABLE      or
  //   CAN NOT HAVE SET: BPF_HOST_CACHE_MANAGED or
  //   CAN NOT HAVE SET: BPF_HOST_CACHED
  BIFROST_BPF_PROTECTED = (1 << 5)

} bfBufferPropertyFlags;

typedef enum bfBufferUsageFlags_t
{
  BIFROST_BUF_TRANSFER_SRC         = (1 << 0), /* NOTE(Shareef): Can be used to transfer data out of.             */
  BIFROST_BUF_TRANSFER_DST         = (1 << 1), /* NOTE(Shareef): Can be used to transfer data into.               */
  BIFROST_BUF_UNIFORM_TEXEL_BUFFER = (1 << 2), /* NOTE(Shareef): Can be used to TODO                              */
  BIFROST_BUF_STORAGE_TEXEL_BUFFER = (1 << 3), /* NOTE(Shareef): Can be used to TODO                              */
  BIFROST_BUF_UNIFORM_BUFFER       = (1 << 4), /* NOTE(Shareef): Can be used to store Uniform data.               */
  BIFROST_BUF_STORAGE_BUFFER       = (1 << 5), /* NOTE(Shareef): Can be used to store SSBO data                   */
  BIFROST_BUF_INDEX_BUFFER         = (1 << 6), /* NOTE(Shareef): Can be used to store Index data.                 */
  BIFROST_BUF_VERTEX_BUFFER        = (1 << 7), /* NOTE(Shareef): Can be used to store Vertex data.                */
  BIFROST_BUF_INDIRECT_BUFFER      = (1 << 8), /* NOTE(Shareef): Can be used to store Indirect Draw Command data. */

  /* NOTE(Shareef):
       Allows for mapped allocations to be shared by keeping it
       persistently mapped until all refs are freed.
       Requirements :
       BPF_HOST_MAPPABLE
  */
  BIFROST_BUF_PERSISTENTLY_MAPPED_BUFFER = (1 << 9) /* NOTE(Shareef): Can be used for data that is streamed to the gpu */

} bfBufferUsageFlags;

/* Misc / TODO */

typedef enum BifrostShaderType_t
{
  BIFROST_SHADER_TYPE_VERTEX                  = 0,
  BIFROST_SHADER_TYPE_TESSELLATION_CONTROL    = 1,
  BIFROST_SHADER_TYPE_TESSELLATION_EVALUATION = 2,
  BIFROST_SHADER_TYPE_GEOMETRY                = 3,
  BIFROST_SHADER_TYPE_FRAGMENT                = 4,
  BIFROST_SHADER_TYPE_COMPUTE                 = 5,
  BIFROST_SHADER_TYPE_MAX                     = 6,

} BifrostShaderType;

typedef enum
{
  BIFROST_SHADER_STAGE_VERTEX                  = bfBit(0),
  BIFROST_SHADER_STAGE_TESSELLATION_CONTROL    = bfBit(1),
  BIFROST_SHADER_STAGE_TESSELLATION_EVALUATION = bfBit(2),
  BIFROST_SHADER_STAGE_GEOMETRY                = bfBit(3),
  BIFROST_SHADER_STAGE_FRAGMENT                = bfBit(4),
  BIFROST_SHADER_STAGE_COMPUTE                 = bfBit(5),
  BIFROST_SHADER_STAGE_GRAPHICS                = BIFROST_SHADER_STAGE_VERTEX |
                                  BIFROST_SHADER_STAGE_TESSELLATION_CONTROL |
                                  BIFROST_SHADER_STAGE_TESSELLATION_EVALUATION |
                                  BIFROST_SHADER_STAGE_GEOMETRY |
                                  BIFROST_SHADER_STAGE_FRAGMENT,

} BifrostShaderStageFlags;

typedef enum BifrostTextureType_t
{
  BIFROST_TEX_TYPE_1D,
  BIFROST_TEX_TYPE_2D,
  BIFROST_TEX_TYPE_3D,
  BIFROST_TEX_TYPE_MAX

} BifrostTextureType;

typedef enum BifrostSamplerFilterMode_t
{
  BIFROST_SFM_NEAREST,
  BIFROST_SFM_LINEAR,
  BIFROST_SFM_MAX

} BifrostSamplerFilterMode;

typedef enum BifrostSamplerAddressMode_t
{
  BIFROST_SAM_REPEAT,
  BIFROST_SAM_MIRRORED_REPEAT,
  BIFROST_SAM_CLAMP_TO_EDGE,
  BIFROST_SAM_MIRROR_CLAMP_TO_EDGE,
  BIFROST_SAM_CLAMP_TO_BORDER,

} BifrostSamplerAddressMode;

typedef struct BifrostTextureSamplerProperties_t
{
  BifrostSamplerFilterMode  min_filter;
  BifrostSamplerFilterMode  mag_filter;
  BifrostSamplerAddressMode u_address;
  BifrostSamplerAddressMode v_address;
  BifrostSamplerAddressMode w_address;
  float                     min_lod;
  float                     max_lod;

} BifrostTextureSamplerProperties;

/* Create Params */
typedef struct
{
  const char* app_name;
  uint32_t    app_version;

} bfGfxContextCreateParams;

typedef struct bfBufferCreateParams_t
{
  bfBufferSize size;
  uint16_t     usage;       // (bfBufferUsageFlags)
  uint16_t     properties;  // (bfBufferPropertyFlags)

} bfBufferCreateParams;

typedef struct bfFramebufferCreateParams_t
{
  bfRenderpassHandle renderpass;
  uint32_t           width;
  uint32_t           height;
  uint32_t           depth;

} bfFramebufferCreateParams;

typedef struct bfRenderpassCreateParams_t
{
  uint16_t num_subpasses;

} bfRenderpassCreateParams;

typedef struct bfShaderProgramCreateParams_t
{
  const char* debug_name;

} bfShaderProgramCreateParams;

typedef struct bfDescriptorSetCreateParams_t
{
  bfShaderProgramHandle shader;

} bfDescriptorSetCreateParams;

#define BIFROST_CALC_MIPMAP (uint32_t) ~0u

typedef enum BifrostTexFeatureBits_t
{
  BIFROST_TEX_IS_TRANSFER_SRC     = bfBit(0),
  BIFROST_TEX_IS_TRANSFER_DST     = bfBit(1),
  BIFROST_TEX_IS_SAMPLED          = bfBit(2),
  BIFROST_TEX_IS_STORAGE          = bfBit(3),
  BIFROST_TEX_IS_COLOR_ATTACHMENT = bfBit(4),
  BIFROST_TEX_IS_DEPTH_ATTACHMENT = bfBit(5),
  BIFROST_TEX_IS_TRANSIENT        = bfBit(6),
  BIFROST_TEX_IS_INPUT_ATTACHMENT = bfBit(7),

} BifrostTexFeatureBits;

typedef uint8_t BifrostTexFeatureFlags;

typedef struct bfTextureCreateParams_t
{
  BifrostTextureType     type;
  BifrostImageFormat     format;
  uint32_t               width;
  uint32_t               height;
  uint32_t               depth;
  uint32_t               mip_levels;  // if set to 'BIFROST_CALC_MIPMAP' then it will be automatically calculates as : 1 + floor(log2((float)max(max(width, height), depth))
  uint32_t               num_layers;
  BifrostTexFeatureFlags flags;

} bfTextureCreateParams;

bfTextureCreateParams bfTextureCreateParams_init2D(uint32_t width, uint32_t height, BifrostImageFormat format);
bfTextureCreateParams bfTextureCreateParams_initCubeMap(uint32_t width, uint32_t height, BifrostImageFormat format);
bfTextureCreateParams bfTextureCreateParams_initColorAttachment(uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);
bfTextureCreateParams bfTextureCreateParams_initDepthAttachment(uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

typedef struct bfGfxCommandListCreateParams_t
{
  uint32_t thread_index;

} bfGfxCommandListCreateParams;

typedef enum BifrostStencilFace_t
{
  BIFROST_STENCIL_FACE_FRONT,
  BIFROST_STENCIL_FACE_BACK,

} BifrostStencilFace;

/* Context */
bfGfxContextHandle bfGfxContext_new(const bfGfxContextCreateParams* params);
bfGfxDeviceHandle  bfGfxContext_device(bfGfxContextHandle self);
void               bfGfxContext_onResize(bfGfxContextHandle self, uint32_t width, uint32_t height);
bfBool32           bfGfxContext_beginFrame(bfGfxContextHandle self);
void               bfGfxContext_endFrame(bfGfxContextHandle self);
void               bfGfxContext_delete(bfGfxContextHandle self);

/* Logical Device */
void                   bfGfxDevice_flush(bfGfxDeviceHandle self);
bfBufferHandle         bfGfxDevice_newBuffer(bfGfxDeviceHandle self, const bfBufferCreateParams* params);
void                   bfGfxDevice_deleteBuffer(bfGfxDeviceHandle self, bfBufferHandle buffer);
bfFramebufferHandle    bfGfxDevice_newFramebuffer(bfGfxDeviceHandle self, const bfFramebufferCreateParams* params);
void                   bfGfxDevice_deleteFramebuffer(bfGfxDeviceHandle self, bfFramebufferHandle framebuffer);
bfRenderpassHandle     bfGfxDevice_newRenderpass(bfGfxDeviceHandle self, const bfRenderpassCreateParams* params);
void                   bfGfxDevice_deleteRenderpass(bfGfxDeviceHandle self, bfRenderpassHandle renderpass);
bfShaderProgramHandle  bfGfxDevice_newShaderProgram(bfGfxDeviceHandle self, const bfShaderProgramCreateParams* params);
void                   bfGfxDevice_deleteShaderProgram(bfGfxDeviceHandle self, bfShaderProgramHandle shader_program);
bfDescriptorSetHandle  bfGfxDevice_newDescriptorSet(bfGfxDeviceHandle self, const bfDescriptorSetCreateParams* params);
void                   bfGfxDevice_deleteDescriptorSet(bfGfxDeviceHandle self, bfDescriptorSetHandle descriptor_set);
bfTextureHandle        bfGfxDevice_newTexture(bfGfxDeviceHandle self, const bfTextureCreateParams* params);
void                   bfGfxDevice_deleteTexture(bfGfxDeviceHandle self, bfTextureHandle texture);
bfGfxCommandListHandle bfGfxDevice_newGfxCommandList(bfGfxDeviceHandle self, const bfGfxCommandListCreateParams* params);
void                   bfGfxDevice_deleteGfxCommandList(bfGfxDeviceHandle self, bfGfxCommandListHandle command_list);

/* Buffer */
bfBufferSize bfBuffer_size(bfBufferHandle self);
void*        bfBuffer_map(bfBufferHandle self, bfBufferSize offset, bfBufferSize size);
void         bfBuffer_copyCPU(bfBufferHandle self, bfBufferSize dst_offset, const void* data, bfBufferSize num_bytes);
void         bfBuffer_copyGPU(bfBufferHandle src, bfBufferSize src_offset, bfBufferHandle dst, bfBufferSize dst_offset, bfBufferSize num_bytes);
void         bfBuffer_unMap(bfBufferHandle self);

/* Framebuffer */
void bfFramebuffer_setSampler(bfFramebufferHandle self, const bfTextureCreateParams* sampler_properties);
void bfFramebuffer_compile(bfFramebufferHandle self);

/* Vertex Binding */
bfVertexLayoutSetHandle bfVertexLayout_new(void);
void                    bfVertexLayout_addVertexBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t sizeof_vertex);
void                    bfVertexLayout_addInstanceBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t stride);
void                    bfVertexLayout_addVertexLayout(bfVertexLayoutSetHandle self, uint32_t binding, BifrostVertexFormatAttribute format, uint32_t offset);
void                    bfVertexLayout_delete(bfVertexLayoutSetHandle self);

/* Renderpass */

typedef struct
{
  BifrostImageFormat format;
  BifrostImageLayout layouts[2];  // [initial, final]
  BifrostSampleFlags sample_count;
  bfBool32           may_alias;

} bfAttachmentInfo;

typedef struct
{
  uint32_t                  subpasses[2];             // [src, dst]
  BifrostPipelineStageFlags pipeline_stage_flags[2];  // [src, dst]
  BifrostAccessFlags        access_flags[2];          // [src, dst]
  bfBool32                  reads_same_pixel;         //

} bfSubpassDependency;

typedef uint16_t bfLoadStoreFlags;

// VK_ATTACHMENT_LOAD_OP_LOAD
// VK_ATTACHMENT_LOAD_OP_CLEAR
// VK_ATTACHMENT_LOAD_OP_DONT_CARE
// VK_ATTACHMENT_STORE_OP_STORE
// VK_ATTACHMENT_STORE_OP_DONT_CARE
void     bfRenderpass_setLoadOps(bfRenderpassHandle self, bfLoadStoreFlags attachment_mask);
void     bfRenderpass_setStencilLoadOps(bfRenderpassHandle self, bfLoadStoreFlags attachment_mask);
void     bfRenderpass_setClearOps(bfRenderpassHandle self, bfLoadStoreFlags attachment_mask);
void     bfRenderpass_setStencilClearOps(bfRenderpassHandle self, bfLoadStoreFlags attachment_mask);
void     bfRenderpass_setStoreOps(bfRenderpassHandle self, bfLoadStoreFlags attachment_mask);
void     bfRenderpass_setStencilStoreOps(bfRenderpassHandle self, uint16_t attachment_mask);
void     bfRenderpass_addColorAttachment(bfRenderpassHandle self, const bfAttachmentInfo* info);
void     bfRenderpass_addDepthAttachment(bfRenderpassHandle self, const bfAttachmentInfo* info);
void     bfRenderpass_addColorOut(bfRenderpassHandle self, uint16_t subpass_index, uint32_t attachment);
void     bfRenderpass_addColorIn(bfRenderpassHandle self, uint16_t subpass_index, uint32_t attachment);
void     bfRenderpass_addDepthOut(bfRenderpassHandle self, uint16_t subpass_index, uint32_t attachment);
void     bfRenderpass_addDepthIn(bfRenderpassHandle self, uint16_t subpass_index, uint32_t attachment);
void     bfRenderpass_addDependencies(bfRenderpassHandle self, const bfSubpassDependency* dependencies, size_t num_dependencies);
bfBool32 bfRenderpass_compile(bfRenderpassHandle self);

/* Shader Program + Module */

bfShaderModuleHandle bfShaderModule_new(BifrostShaderType type);
BifrostShaderType    bfShaderModule_type(bfShaderModuleHandle self);
bfBool32             bfShaderModule_loadFile(bfShaderModuleHandle self, const char* file);
bfBool32             bfShaderModule_loadData(bfShaderModuleHandle self, const char* source, size_t source_length);
void                 bfShaderModule_delete(bfShaderModuleHandle self);

void bfShaderProgram_addModule(bfShaderProgramHandle self, bfShaderModuleHandle module);
void bfShaderProgram_addAttribute(bfShaderProgramHandle self, const char* name, uint32_t binding);  // glBindAttribLocation
void bfShaderProgram_addUniformBuffer(bfShaderProgramHandle self, const char* name, uint32_t how_many, BifrostShaderStageFlags stages);
void bfShaderProgram_addUniformBufferAt(bfShaderProgramHandle self, const char* name, uint32_t binding, uint32_t how_many, BifrostShaderStageFlags stages);
void bfShaderProgram_addImageSampler(bfShaderProgramHandle self, const char* name, uint32_t how_many, BifrostShaderStageFlags stages);
void bfShaderProgram_addImageSamplerAt(bfShaderProgramHandle self, const char* name, uint32_t binding, uint32_t how_many, BifrostShaderStageFlags stages);
void bfShaderProgram_compile(bfShaderProgramHandle self);

/* Descriptor Set */
void bfDescriptorSet_addTextures(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures);
// void bfDescriptorSet_addTextureI(bfDescriptorSetHandle self);
// void bfDescriptorSet_addTexture(bfDescriptorSetHandle self);
void bfDescriptorSet_addUniformBuffers(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, const uint64_t* offsets, uint64_t* sizes, bfBuffer* buffers, uint32_t num_buffers);
// void bfDescriptorSet_addUniformBufferI(bfDescriptorSetHandle self);
// void bfDescriptorSet_addUniformBuffer(bfDescriptorSetHandle self);

/* Textures */
uint32_t           bfTexture_width(bfTextureHandle self);
uint32_t           bfTexture_height(bfTextureHandle self);
uint32_t           bfTexture_depth(bfTextureHandle self);
BifrostImageLayout bfTexture_layout(BifrostImageLayout self);
void               bfTexture_setLayout(BifrostImageLayout self, BifrostImageLayout layout);  // TODO(SR): Maybe this should solely be internal, it will be punlic to help me rember what I need to do. The reason this is a bad way to have in the public API is because of the needed syncronzation needed to transiotion in vulkan but we should never need to change layout unless for a backend like RenderGraph and Texture_setData.
bfBool32           bfTexture_loadFile(bfTextureHandle self, const char* file);
bfBool32           bfTexture_loadData(bfTextureHandle self, const char* source, size_t source_length);
void               bfTexture_setSampler(bfTextureHandle self, const bfTextureCreateParams* sampler_properties);

// Transient Resources
// Also need a temp Framebuffer / Attachment API for effective framegraph use.
// void bfGfxCmdList_createFramebuffer(bfGfxCommandListHandle self);
// void bfGfxCmdList_createVertexBuffer(bfGfxCommandListHandle self);
// void bfGfxCmdList_createIndexBuffer(bfGfxCommandListHandle self);
// void bfGfxCmdList_pipelineBarrier(bfGfxCommandListHandle self);
// void bfGfxCmdList_memoryBarrier(bfGfxCommandListHandle self);
// void bfGfxCmdList_event(bfGfxCommandListHandle self);

// if 'VkDescriptorSetLayout' is the same for two pipelines then the layout can be shared.

/* CommandList */
void     bfGfxCmdList_begin(bfGfxCommandListHandle self, bfBool32 reset_after_done);
void     bfGfxCmdList_nextSubpass(bfGfxCommandListHandle self);
void     bfGfxCmdList_setClearValues(const BifrostClearValue* clear_values, size_t num_clear_values);
void     bfGfxCmdList_setRenderArea(const BifrostScissorRect* render_area);
void     bfGfxCmdList_beginRenderpass(bfGfxCommandListHandle self, bfRenderpassHandle renderpass, bfFramebufferHandle framebuffer);
void     bfGfxCmdList_beginFramebuffer(bfGfxCommandListHandle self, bfFramebufferHandle framebuffer);
uint32_t bfGfxCmdList_saveState(bfGfxCommandListHandle self);  // Pipeline Static State
void     bfGfxCmdList_loadState(bfGfxCommandListHandle self, uint32_t save_index);
void     bfGfxCmdList_setDrawMode(bfGfxCommandListHandle self, BifrostDrawMode draw_mode);
void     bfGfxCmdList_setBlendSrc(bfGfxCommandListHandle self, BifrostBlendFactor factor);
void     bfGfxCmdList_setBlendDst(bfGfxCommandListHandle self, BifrostBlendFactor factor);
void     bfGfxCmdList_setFrontFace(bfGfxCommandListHandle self, BifrostFrontFace front_face);
void     bfGfxCmdList_setCullFace(bfGfxCommandListHandle self, BifrostCullFaceFlags cull_face);
void     bfGfxCmdList_setDepthTesting(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setDepthWrite(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setDepthTestOp(bfGfxCommandListHandle self, BifrostCompareOp op);
void     bfGfxCmdList_setStencilTesting(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setPrimitiveRestart(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setRasterizerDiscard(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setDepthBias(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setSampleShading(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setAlphaToCoverage(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setAlphaToOne(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setLogicOp(bfGfxCommandListHandle self, BifrostLogicOp op);
void     bfGfxCmdList_setPolygonFillMode(bfGfxCommandListHandle self, BifrostPolygonFillMode fill_mode);
void     bfGfxCmdList_setColorBlendOp(bfGfxCommandListHandle self, BifrostBlendOp op);
void     bfGfxCmdList_setAlphaBlendOp(bfGfxCommandListHandle self, BifrostBlendOp op);
void     bfGfxCmdList_setBlendSrcAlpha(bfGfxCommandListHandle self, BifrostBlendFactor factor);
void     bfGfxCmdList_setBlendDstAlpha(bfGfxCommandListHandle self, BifrostBlendFactor factor);
void     bfGfxCmdList_setColorWriteMask(bfGfxCommandListHandle self, uint8_t color_mask);
void     bfGfxCmdList_setStencilFailOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op);
void     bfGfxCmdList_setStencilPassOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op);
void     bfGfxCmdList_setStencilDepthFailOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op);
void     bfGfxCmdList_setStencilCompareOp(bfGfxCommandListHandle self, BifrostStencilFace face, BifrostStencilOp op);
void     bfGfxCmdList_setStencilCompareMask(bfGfxCommandListHandle self, BifrostStencilFace face, uint8_t cmp_mask);
void     bfGfxCmdList_setStencilWriteMask(bfGfxCommandListHandle self, BifrostStencilFace face, uint8_t write_mask);
void     bfGfxCmdList_setStencilReference(bfGfxCommandListHandle self, BifrostStencilFace face, uint8_t ref_mask);
void     bfGfxCmdList_setDynamicStates(bfGfxCommandListHandle self, uint16_t dynamic_states);
void     bfGfxCmdList_setViewport(bfGfxCommandListHandle self, float x, float y, float width, float height, const float depth[2]);
void     bfGfxCmdList_setScissorRect(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height);
void     bfGfxCmdList_setBlendConstants(bfGfxCommandListHandle self, const float constants[4]);
void     bfGfxCmdList_setLineWidth(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setDepthBiasConstantFactor(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setDepthBiasClamp(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setDepthBiasSlopeFactor(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setMinSampleShading(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setSampleCountFlags(bfGfxCommandListHandle self, uint32_t sample_flags);
void     bfGfxCmdList_restoreState(bfGfxCommandListHandle self, uint32_t index);
void     bfGfxCmdList_bindVertexBuffers(bfGfxCommandListHandle self, uint32_t binding, bfBufferHandle* buffers, uint32_t num_buffers, const uint64_t* offsets);
void     bfGfxCmdList_bindVertexDesc(bfGfxCommandListHandle self, uint32_t bind_point, bfVertexLayoutSet* vertex_layouts, uint32_t num_vertex_layouts);
void     bfGfxCmdList_bindIndexBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, uint64_t offset, BifrostIndexType idx_type);
void     bfGfxCmdList_bindShader(bfGfxCommandListHandle self, bfShaderProgramHandle shader);
void     bfGfxCmdList_bindDescriptorSet(bfGfxCommandListHandle self, uint32_t bind_point, bfDescriptorSetHandle* desc_sets, uint32_t num_desc_sets);  // Call after pipeline is setup.
void     bfGfxCmdList_draw(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices);                                                // Draw Cmds
void     bfGfxCmdList_drawInstanced(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices, uint32_t first_instance, uint32_t num_instances);
void     bfGfxCmdList_drawIndexed(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset);
void     bfGfxCmdList_drawIndexedInstanced(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset, uint32_t first_instance, uint32_t num_instances);
void     bfGfxCmdList_executeSubCommands(bfGfxCommandListHandle self, bfGfxCommandListHandle* commands, uint32_t num_commands);  // General
void     bfGfxCmdList_endRenderpass(bfGfxCommandListHandle self);
void     bfGfxCmdList_end(bfGfxCommandListHandle self);
void     bfGfxCmdList_updateBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size, const void* data);  // Outside Renderpass
void     bfGfxCmdList_submit(bfGfxCommandListHandle self);
#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_API_H */