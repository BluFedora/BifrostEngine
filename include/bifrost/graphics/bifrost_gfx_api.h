/*!
 * @file   bifrost_gfx_api.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_GFX_API_H
#define BIFROST_GFX_API_H

#include "bifrost/bifrost_std.h" /* bfBool32 */
#include "bifrost_gfx_handle.h"
#include "bifrost_gfx_limits.h"
#include "bifrost_gfx_pipeline_state.h" /* */
#include "bifrost_gfx_types.h"          /* */

#include <stdint.h> /* uint32_t */

#if __cplusplus
extern "C" {
#endif
/* Context */
typedef uint64_t bfBufferSize;

#define BIFROST_BUFFER_WHOLE_SIZE (~0ull)
#define BIFROST_TEXTURE_UNKNOWN_SIZE (-1)
#define BIFROST_SUBPASS_EXTERNAL (~0U)

/* Buffer */
typedef enum bfBufferPropertyFlags_t
{
  /// Best for Device Access to the Memory.
  BIFROST_BPF_DEVICE_LOCAL = (1 << 0),

  /// Can be mapped on the host.
  BIFROST_BPF_HOST_MAPPABLE = (1 << 1),

  /// You don't need 'vkFlushMappedMemoryRanges' and 'vkInvalidateMappedMemoryRanges' anymore.
  BIFROST_BPF_HOST_CACHE_MANAGED = (1 << 2),

  /// Always host coherent, cached on the host for increased host access speed.
  BIFROST_BPF_HOST_CACHED = (1 << 3),

  /// Implementation defined lazy allocation of the buffer.
  /// use: vkGetDeviceMemoryCommitment
  /// CAN NOT HAVE SET: BPF_HOST_MAPPABLE
  BIFROST_BPF_DEVICE_LAZY_ALLOC = (1 << 4),

  // Only device accessible and allows protected queue operations.
  // CAN NOT HAVE SET: BPF_HOST_MAPPABLE      or
  // CAN NOT HAVE SET: BPF_HOST_CACHE_MANAGED or
  // CAN NOT HAVE SET: BPF_HOST_CACHED
  BIFROST_BPF_PROTECTED = (1 << 5)

} bfBufferPropertyFlags;

typedef uint16_t bfBufferPropertyBits;

typedef enum bfBufferUsageFlags_t
{
  BIFROST_BUF_TRANSFER_SRC         = (1 << 0), /*!< Can be used to transfer data out of.             */
  BIFROST_BUF_TRANSFER_DST         = (1 << 1), /*!< Can be used to transfer data into.               */
  BIFROST_BUF_UNIFORM_TEXEL_BUFFER = (1 << 2), /*!< Can be used to TODO                              */
  BIFROST_BUF_STORAGE_TEXEL_BUFFER = (1 << 3), /*!< Can be used to TODO                              */
  BIFROST_BUF_UNIFORM_BUFFER       = (1 << 4), /*!< Can be used to store Uniform data.               */
  BIFROST_BUF_STORAGE_BUFFER       = (1 << 5), /*!< Can be used to store SSBO data                   */
  BIFROST_BUF_INDEX_BUFFER         = (1 << 6), /*!< Can be used to store Index data.                 */
  BIFROST_BUF_VERTEX_BUFFER        = (1 << 7), /*!< Can be used to store Vertex data.                */
  BIFROST_BUF_INDIRECT_BUFFER      = (1 << 8), /*!< Can be used to store Indirect Draw Command data. */

  /* NOTE(Shareef):
       Allows for mapped allocations to be shared by keeping it
       persistently mapped until all refs are freed.
       Requirements :
         'BIFROST_BPF_HOST_MAPPABLE'
  */
  BIFROST_BUF_PERSISTENTLY_MAPPED_BUFFER = (1 << 9) /*!< NOTE(Shareef): Can be used for data that is streamed to the gpu */

} bfBufferUsageFlags;

typedef uint16_t bfBufferUsageBits;

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

typedef uint8_t BifrostShaderStageBits;

typedef enum BifrostTextureType_t
{
  BIFROST_TEX_TYPE_1D,
  BIFROST_TEX_TYPE_2D,
  BIFROST_TEX_TYPE_3D,

} BifrostTextureType;

typedef enum BifrostSamplerFilterMode_t
{
  BIFROST_SFM_NEAREST,
  BIFROST_SFM_LINEAR,

} BifrostSamplerFilterMode;

typedef enum BifrostSamplerAddressMode_t
{
  BIFROST_SAM_REPEAT,
  BIFROST_SAM_MIRRORED_REPEAT,
  BIFROST_SAM_CLAMP_TO_EDGE,
  BIFROST_SAM_CLAMP_TO_BORDER,
  BIFROST_SAM_MIRROR_CLAMP_TO_EDGE,

} BifrostSamplerAddressMode;

typedef struct bfTextureSamplerProperties_t
{
  BifrostSamplerFilterMode  min_filter;
  BifrostSamplerFilterMode  mag_filter;
  BifrostSamplerAddressMode u_address;
  BifrostSamplerAddressMode v_address;
  BifrostSamplerAddressMode w_address;
  float                     min_lod;
  float                     max_lod;

} bfTextureSamplerProperties;

bfTextureSamplerProperties bfTextureSamplerProperties_init(BifrostSamplerFilterMode filter, BifrostSamplerAddressMode uv_addressing);

/* Create Params */
typedef struct
{
  const char* app_name;
  uint32_t    app_version;
  void*       platform_module;
  void*       platform_window;

  /*
    if (Window)
    {
      platform_module:HINSTANCE = GetModuleHandle(NULL);
      platform_window:HWND      = GetActiveWindow();
    }

    if (X11_Linux)
    {
      platform_module:Display* = ???;
      platform_module:Window   = ???;
    }
  */
} bfGfxContextCreateParams;

typedef struct bfAllocationCreateInfo_t
{
  bfBufferSize         size;
  bfBufferPropertyBits properties;

} bfAllocationCreateInfo;

typedef struct bfBufferCreateParams_t
{
  bfAllocationCreateInfo allocation;
  bfBufferUsageBits      usage;

} bfBufferCreateParams;

struct bfRenderpassInfo_t;
typedef struct bfRenderpassInfo_t bfRenderpassCreateParams;

typedef struct bfShaderProgramCreateParams_t
{
  const char* debug_name;
  uint32_t    num_desc_sets;

} bfShaderProgramCreateParams;

typedef enum BifrostTexFeatureBits_t
{
  BIFROST_TEX_IS_TRANSFER_SRC       = bfBit(0),
  BIFROST_TEX_IS_TRANSFER_DST       = bfBit(1),
  BIFROST_TEX_IS_SAMPLED            = bfBit(2),
  BIFROST_TEX_IS_STORAGE            = bfBit(3),
  BIFROST_TEX_IS_COLOR_ATTACHMENT   = bfBit(4),
  BIFROST_TEX_IS_DEPTH_ATTACHMENT   = bfBit(5),
  BIFROST_TEX_IS_STENCIL_ATTACHMENT = bfBit(6),
  BIFROST_TEX_IS_TRANSIENT          = bfBit(7),
  BIFROST_TEX_IS_INPUT_ATTACHMENT   = bfBit(8),
  BIFROST_TEX_IS_MULTI_QUEUE        = bfBit(9),
  BIFROST_TEX_IS_LINEAR             = bfBit(10),

} BifrostTexFeatureBits;

typedef uint16_t BifrostTexFeatureFlags;

typedef struct bfTextureCreateParams_t
{
  BifrostTextureType     type;
  BifrostImageFormat     format;
  uint32_t               width;
  uint32_t               height;
  uint32_t               depth;
  bfBool32               generate_mipmaps;
  uint32_t               num_layers;
  BifrostTexFeatureFlags flags;

} bfTextureCreateParams;

bfTextureCreateParams bfTextureCreateParams_init2D(BifrostImageFormat format, uint32_t width, uint32_t height);
bfTextureCreateParams bfTextureCreateParams_initCubeMap(uint32_t width, uint32_t height, BifrostImageFormat format);
bfTextureCreateParams bfTextureCreateParams_initColorAttachment(uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);
bfTextureCreateParams bfTextureCreateParams_initDepthAttachment(uint32_t width, uint32_t height, BifrostImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

typedef struct bfGfxCommandListCreateParams_t
{
  uint32_t thread_index;
  int      window_idx;

} bfGfxCommandListCreateParams;

typedef enum BifrostStencilFace_t
{
  BIFROST_STENCIL_FACE_FRONT,
  BIFROST_STENCIL_FACE_BACK,

} BifrostStencilFace;

typedef struct
{
  uint32_t frame_index;
  uint32_t frame_count;
  uint32_t num_frame_indices;

} bfGfxFrameInfo;

/* Context */
bfGfxContextHandle     bfGfxContext_new(const bfGfxContextCreateParams* params);
bfGfxDeviceHandle      bfGfxContext_device(bfGfxContextHandle self);
void                   bfGfxContext_onResize(bfGfxContextHandle self);
bfTextureHandle        bfGfxContext_swapchainImage(bfGfxContextHandle self);  // This needs to be able to select which window.
bfBool32               bfGfxContext_beginFrame(bfGfxContextHandle self, int window_idx);
bfGfxFrameInfo         bfGfxContext_getFrameInfo(bfGfxContextHandle self, int window_idx);
bfGfxCommandListHandle bfGfxContext_requestCommandList(bfGfxContextHandle self, const bfGfxCommandListCreateParams* params);
void                   bfGfxContext_endFrame(bfGfxContextHandle self);
void                   bfGfxContext_delete(bfGfxContextHandle self);

// TODO(SR): This should not be in this Public API header.
// Needs to be made internal.
typedef enum BifrostGfxObjectType_t
{
  BIFROST_GFX_OBJECT_BUFFER         = 0,
  BIFROST_GFX_OBJECT_RENDERPASS     = 1,
  BIFROST_GFX_OBJECT_SHADER_MODULE  = 2,
  BIFROST_GFX_OBJECT_SHADER_PROGRAM = 3,
  BIFROST_GFX_OBJECT_DESCRIPTOR_SET = 4,
  BIFROST_GFX_OBJECT_TEXTURE        = 5,
  BIFROST_GFX_OBJECT_FRAMEBUFFER    = 6,
  BIFROST_GFX_OBJECT_PIPELINE       = 7,

} BifrostGfxObjectType;

typedef uint32_t bfFrameCount_t;
#define bfFrameCountMax 0xFFFFFFFF

typedef struct BifrostGfxObjectBase_t
{
  BifrostGfxObjectType           type;
  struct BifrostGfxObjectBase_t* next;
  uint64_t                       hash_code;
  bfFrameCount_t                 last_frame_used;

} BifrostGfxObjectBase;

void BifrostGfxObjectBase_ctor(BifrostGfxObjectBase* self, BifrostGfxObjectType type);
// END: Private Impl Header

/* Logical Device */
typedef struct bfDeviceLimits_t
{
  bfBufferSize uniform_buffer_offset_alignment; /* Worst case is 256 (0x100) */

} bfDeviceLimits;

void                  bfGfxDevice_flush(bfGfxDeviceHandle self);
bfBufferHandle        bfGfxDevice_newBuffer(bfGfxDeviceHandle self, const bfBufferCreateParams* params);
bfRenderpassHandle    bfGfxDevice_newRenderpass(bfGfxDeviceHandle self, const bfRenderpassCreateParams* params);
bfShaderModuleHandle  bfGfxDevice_newShaderModule(bfGfxDeviceHandle self, BifrostShaderType type);
bfShaderProgramHandle bfGfxDevice_newShaderProgram(bfGfxDeviceHandle self, const bfShaderProgramCreateParams* params);
bfTextureHandle       bfGfxDevice_newTexture(bfGfxDeviceHandle self, const bfTextureCreateParams* params);
bfTextureHandle       bfGfxDevice_requestSurface(bfGfxCommandListHandle command_list);
bfDeviceLimits        bfGfxDevice_limits(bfGfxDeviceHandle self);
void                  bfGfxDevice_release(bfGfxDeviceHandle self, bfGfxBaseHandle resource);

/* Buffer */
bfBufferSize bfBuffer_size(bfBufferHandle self);
void*        bfBuffer_mappedPtr(bfBufferHandle self);
void*        bfBuffer_map(bfBufferHandle self, bfBufferSize offset, bfBufferSize size);
void         bfBuffer_invalidateRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges);
void         bfBuffer_invalidateRange(bfBufferHandle self, bfBufferSize offset, bfBufferSize size);
void         bfBuffer_copyCPU(bfBufferHandle self, bfBufferSize dst_offset, const void* data, bfBufferSize num_bytes);
void         bfBuffer_copyGPU(bfBufferHandle src, bfBufferSize src_offset, bfBufferHandle dst, bfBufferSize dst_offset, bfBufferSize num_bytes);
void         bfBuffer_flushRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges);
void         bfBuffer_flushRange(bfBufferHandle self, bfBufferSize offset, bfBufferSize size);
void         bfBuffer_unMap(bfBufferHandle self);

/* Vertex Binding */
bfVertexLayoutSetHandle bfVertexLayout_new(void);
void                    bfVertexLayout_addVertexBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t sizeof_vertex);
void                    bfVertexLayout_addInstanceBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t stride);
void                    bfVertexLayout_addVertexLayout(bfVertexLayoutSetHandle self, uint32_t binding, BifrostVertexFormatAttribute format, uint32_t offset);
void                    bfVertexLayout_delete(bfVertexLayoutSetHandle self);

/* Renderpass */
typedef struct bfAttachmentInfo_t
{
  bfTextureHandle    texture;  // [format, layouts[0], sample_count]
  BifrostImageLayout final_layout;
  bfBool32           may_alias;

} bfAttachmentInfo;

typedef struct bfSubpassDependency_t
{
  uint32_t                  subpasses[2];             // [src, dst]
  BifrostPipelineStageFlags pipeline_stage_flags[2];  // [src, dst]
  BifrostAccessFlags        access_flags[2];          // [src, dst]
  bfBool32                  reads_same_pixel;         // should be true in most cases (exception being bluring)

} bfSubpassDependency;

typedef uint16_t bfLoadStoreFlags;

typedef struct bfAttachmentRefCache_t
{
  uint32_t           attachment_index;
  BifrostImageLayout layout;

} bfAttachmentRefCache;

typedef struct bfSubpassCache_t
{
  uint16_t             num_out_attachment_refs;
  uint16_t             num_in_attachment_refs;
  bfAttachmentRefCache out_attachment_refs[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  bfAttachmentRefCache in_attachment_refs[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  bfAttachmentRefCache depth_attachment;

} bfSubpassCache;

typedef struct bfRenderpassInfo_t
{
  uint64_t            hash_code;
  bfLoadStoreFlags    load_ops;
  bfLoadStoreFlags    stencil_load_ops;
  bfLoadStoreFlags    clear_ops;
  bfLoadStoreFlags    stencil_clear_ops;
  bfLoadStoreFlags    store_ops;
  bfLoadStoreFlags    stencil_store_ops;
  uint16_t            num_subpasses;
  uint16_t            num_attachments;
  uint16_t            num_dependencies;
  bfSubpassCache      subpasses[BIFROST_GFX_RENDERPASS_MAX_SUBPASSES];
  bfAttachmentInfo    attachments[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  bfSubpassDependency dependencies[BIFROST_GFX_RENDERPASS_MAX_DEPENDENCIES];

} bfRenderpassInfo;

bfRenderpassInfo bfRenderpassInfo_init(uint16_t num_subpasses);
void             bfRenderpassInfo_setLoadOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
void             bfRenderpassInfo_setStencilLoadOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
void             bfRenderpassInfo_setClearOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
void             bfRenderpassInfo_setStencilClearOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
void             bfRenderpassInfo_setStoreOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
void             bfRenderpassInfo_setStencilStoreOps(bfRenderpassInfo* self, bfLoadStoreFlags attachment_mask);
void             bfRenderpassInfo_addAttachment(bfRenderpassInfo* self, const bfAttachmentInfo* info);
void             bfRenderpassInfo_addColorOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, BifrostImageLayout layout);
void             bfRenderpassInfo_addDepthOut(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment, BifrostImageLayout layout);
void             bfRenderpassInfo_addInput(bfRenderpassInfo* self, uint16_t subpass_index, uint32_t attachment);
void             bfRenderpassInfo_addDependencies(bfRenderpassInfo* self, const bfSubpassDependency* dependencies, uint32_t num_dependencies);

/* Shader Program + Module */
BifrostShaderType     bfShaderModule_type(bfShaderModuleHandle self);
bfBool32              bfShaderModule_loadFile(bfShaderModuleHandle self, const char* file);
bfBool32              bfShaderModule_loadData(bfShaderModuleHandle self, const char* source, size_t source_length);
void                  bfShaderProgram_addModule(bfShaderProgramHandle self, bfShaderModuleHandle module);
void                  bfShaderProgram_addAttribute(bfShaderProgramHandle self, const char* name, uint32_t binding);  // glBindAttribLocation
void                  bfShaderProgram_addUniformBuffer(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, BifrostShaderStageBits stages);
void                  bfShaderProgram_addImageSampler(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, BifrostShaderStageBits stages);
void                  bfShaderProgram_compile(bfShaderProgramHandle self);
bfDescriptorSetHandle bfShaderProgram_createDescriptorSet(bfShaderProgramHandle self, uint32_t index);

/* Descriptor Set */
typedef enum bfDescriptorElementInfoType_t
{
  BIFROST_DESCRIPTOR_ELEMENT_TEXTURE,
  BIFROST_DESCRIPTOR_ELEMENT_BUFFER,
  BIFROST_DESCRIPTOR_ELEMENT_DYNAMIC_BUFFER,
  BIFROST_DESCRIPTOR_ELEMENT_BUFFER_VIEW,
  BIFROST_DESCRIPTOR_ELEMENT_INPUT_ATTACHMENT,

} bfDescriptorElementInfoType;

typedef struct bfDescriptorElementInfo_t
{
  bfDescriptorElementInfoType type;
  uint32_t                    binding;
  uint32_t                    array_element_start;
  uint32_t                    num_handles; /* also size of bfDescriptorElementInfo::offsets and bfDescriptorElementInfo::sizes */
  bfGfxBaseHandle             handles[BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS];
  uint64_t                    offsets[BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS];
  uint64_t                    sizes[BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS];

} bfDescriptorElementInfo;

typedef struct bfDescriptorSetInfo_t
{
  bfDescriptorElementInfo bindings[BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS];
  uint32_t                num_bindings;

} bfDescriptorSetInfo;

bfDescriptorSetInfo bfDescriptorSetInfo_make(void);
void                bfDescriptorSetInfo_addTexture(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures);
void                bfDescriptorSetInfo_addUniform(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, const uint64_t* offsets, const uint64_t* sizes, bfBufferHandle* buffers, uint32_t num_buffers);

/* The Descriptor Set API is for 'Immutable' Bindings otherwise use the bfDescriptorSetInfo API */

void bfDescriptorSet_setCombinedSamplerTextures(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures);
void bfDescriptorSet_setUniformBuffers(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, const bfBufferSize* offsets, const bfBufferSize* sizes, bfBufferHandle* buffers, uint32_t num_buffers);
void bfDescriptorSet_flushWrites(bfDescriptorSetHandle self);

/* Texture */
uint32_t           bfTexture_width(bfTextureHandle self);
uint32_t           bfTexture_height(bfTextureHandle self);
uint32_t           bfTexture_depth(bfTextureHandle self);
BifrostImageLayout bfTexture_layout(bfTextureHandle self);
bfBool32           bfTexture_loadFile(bfTextureHandle self, const char* file);
void               bfTexture_loadBuffer(bfTextureHandle self, bfBufferHandle buffer);
bfBool32           bfTexture_loadData(bfTextureHandle self, const char* pixels, size_t pixels_length);
void               bfTexture_setSampler(bfTextureHandle self, const bfTextureSamplerProperties* sampler_properties);

/* CommandList */

typedef enum bfPipelineBarrierType_t
{
  BIFROST_PIPELINE_BARRIER_MEMORY,
  BIFROST_PIPELINE_BARRIER_BUFFER,
  BIFROST_PIPELINE_BARRIER_IMAGE,

} bfPipelineBarrierType;

typedef struct bfPipelineBarrier_t
{
  bfPipelineBarrierType  type;
  BifrostAccessFlagsBits access[2];         /*!< [src, dst]                             */
  BifrostGfxQueueType    queue_transfer[2]; /*!< [old, new] For Buffer and Image types. */

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
      bfTextureHandle    handle;
      BifrostImageLayout layout_transition[2]; /*!< [old, new] */
      uint32_t           base_mip_level;
      uint32_t           level_count;
      uint32_t           base_array_layer;
      uint32_t           layer_count;

    } image;

  } info;

} bfPipelineBarrier;

bfPipelineBarrier bfPipelineBarrier_memory(BifrostAccessFlagsBits src_access, BifrostAccessFlagsBits dst_access);
bfPipelineBarrier bfPipelineBarrier_buffer(BifrostAccessFlagsBits src_access, BifrostAccessFlagsBits dst_access, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size);
bfPipelineBarrier bfPipelineBarrier_image(BifrostAccessFlagsBits src_access, BifrostAccessFlagsBits dst_access, bfTextureHandle image, BifrostImageLayout new_layout);

bfBool32 bfGfxCmdList_begin(bfGfxCommandListHandle self);  // True if no error
void     bfGfxCmdList_executionBarrier(bfGfxCommandListHandle self, BifrostPipelineStageBits src_stage, BifrostPipelineStageBits dst_stage, bfBool32 reads_same_pixel);
void     bfGfxCmdList_pipelineBarriers(bfGfxCommandListHandle self, BifrostPipelineStageBits src_stage, BifrostPipelineStageBits dst_stage, const bfPipelineBarrier* barriers, uint32_t num_barriers, bfBool32 reads_same_pixel);
void     bfGfxCmdList_setRenderpass(bfGfxCommandListHandle self, bfRenderpassHandle renderpass);
void     bfGfxCmdList_setRenderpassInfo(bfGfxCommandListHandle self, const bfRenderpassInfo* renderpass_info);
void     bfGfxCmdList_setClearValues(bfGfxCommandListHandle self, const BifrostClearValue* clear_values);
void     bfGfxCmdList_setAttachments(bfGfxCommandListHandle self, bfTextureHandle* attachments);
void     bfGfxCmdList_setRenderAreaAbs(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height);
void     bfGfxCmdList_setRenderAreaRel(bfGfxCommandListHandle self, float x, float y, float width, float height);  // Normalized [0.0f, 1.0f] coords
void     bfGfxCmdList_beginRenderpass(bfGfxCommandListHandle self);
void     bfGfxCmdList_nextSubpass(bfGfxCommandListHandle self);
void     bfGfxCmdList_setDrawMode(bfGfxCommandListHandle self, BifrostDrawMode draw_mode);
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
void     bfGfxCmdList_setColorWriteMask(bfGfxCommandListHandle self, uint32_t output_attachment_idx, uint8_t color_mask);
void     bfGfxCmdList_setColorBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendOp op);
void     bfGfxCmdList_setBlendSrc(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor);
void     bfGfxCmdList_setBlendDst(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor);
void     bfGfxCmdList_setAlphaBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendOp op);
void     bfGfxCmdList_setBlendSrcAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor);
void     bfGfxCmdList_setBlendDstAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, BifrostBlendFactor factor);
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
void     bfGfxCmdList_setDepthClampEnabled(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setDepthBoundsTestEnabled(bfGfxCommandListHandle self, bfBool32 value);
void     bfGfxCmdList_setDepthBounds(bfGfxCommandListHandle self, float min, float max);
void     bfGfxCmdList_setDepthBiasConstantFactor(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setDepthBiasClamp(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setDepthBiasSlopeFactor(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setMinSampleShading(bfGfxCommandListHandle self, float value);
void     bfGfxCmdList_setSampleMask(bfGfxCommandListHandle self, uint32_t sample_mask);
void     bfGfxCmdList_bindVertexDesc(bfGfxCommandListHandle self, bfVertexLayoutSetHandle vertex_set_layout);
void     bfGfxCmdList_bindVertexBuffers(bfGfxCommandListHandle self, uint32_t binding, bfBufferHandle* buffers, uint32_t num_buffers, const uint64_t* offsets);
void     bfGfxCmdList_bindIndexBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, uint64_t offset, BifrostIndexType idx_type);
void     bfGfxCmdList_bindProgram(bfGfxCommandListHandle self, bfShaderProgramHandle shader);
void     bfGfxCmdList_bindDescriptorSets(bfGfxCommandListHandle self, uint32_t binding, bfDescriptorSetHandle* desc_sets, uint32_t num_desc_sets);  // Call after pipeline is setup.
void     bfGfxCmdList_bindDescriptorSet(bfGfxCommandListHandle self, uint32_t set_index, const bfDescriptorSetInfo* desc_set_info);                 // Call after pipeline is setup.
void     bfGfxCmdList_draw(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices);                                              // Draw Cmds
void     bfGfxCmdList_drawInstanced(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices, uint32_t first_instance, uint32_t num_instances);
void     bfGfxCmdList_drawIndexed(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset);
void     bfGfxCmdList_drawIndexedInstanced(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset, uint32_t first_instance, uint32_t num_instances);
void     bfGfxCmdList_executeSubCommands(bfGfxCommandListHandle self, bfGfxCommandListHandle* commands, uint32_t num_commands);  // General
void     bfGfxCmdList_endRenderpass(bfGfxCommandListHandle self);
void     bfGfxCmdList_end(bfGfxCommandListHandle self);
void     bfGfxCmdList_updateBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size, const void* data);  // Outside Renderpass
void     bfGfxCmdList_submit(bfGfxCommandListHandle self);

// TODO: This should not be in this file...
char* LoadFileIntoMemory(const char* filename, long* out_size);

#if __cplusplus
}
#endif

#endif /* BIFROST_GFX_API_H */
