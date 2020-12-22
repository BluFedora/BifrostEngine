/******************************************************************************/
/*!
 * @file   bf_gfx_api.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Outlines the cross platform C-API for low level graphics.
 *
 * @version 0.0.1
 * @date    2020-03-22
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BF_GFX_API_H
#define BF_GFX_API_H

#include "bf/Core.h"               /* bfBool32, bfBit */
#include "bf_gfx_export.h"         /* BF_GFX_API      */
#include "bf_gfx_handle.h"         /*                 */
#include "bf_gfx_limits.h"         /*                 */
#include "bf_gfx_pipeline_state.h" /*                 */
#include "bf_gfx_types.h"          /*                 */

#include <stdint.h> /* uint32_t */

#if __cplusplus
extern "C" {
#endif
/* Constants */

enum
{
  k_bfGfxMaxFramesDelay = 3, /*!< The max number of frames the CPU and GPU can be out of synce by. */
};

/* clang-format off */
#define k_bfBufferWholeSize    ((uint64_t)~0ull)
#define k_bfTextureUnknownSize ((uint32_t)-1)
#define k_bfSubpassExternal    (~0U)
/* clang-format on */

/* Forward Declarations */
struct bfWindow;

typedef uint64_t bfBufferSize;

/* Buffer */
enum
{
  /// Best for Device Access to the Memory.
  BF_BUFFER_PROP_DEVICE_LOCAL = (1 << 0),

  /// Can be mapped on the host.
  BF_BUFFER_PROP_HOST_MAPPABLE = (1 << 1),

  /// You don't need 'vkFlushMappedMemoryRanges' and 'vkInvalidateMappedMemoryRanges' anymore.
  BF_BUFFER_PROP_HOST_CACHE_MANAGED = (1 << 2),

  /// Always host coherent, cached on the host for increased host access speed.
  BF_BUFFER_PROP_HOST_CACHED = (1 << 3),

  /// Implementation defined lazy allocation of the buffer.
  /// use: vkGetDeviceMemoryCommitment
  ///   Mutually Exclusive To: BPF_HOST_MAPPABLE
  BF_BUFFER_PROP_DEVICE_LAZY_ALLOC = (1 << 4),

  /// Only device accessible and allows protected queue operations.
  ///   Mutually Exclusive To: BPF_HOST_MAPPABLE, BPF_HOST_CACHE_MANAGED, BPF_HOST_CACHED.
  BF_BUFFER_PROP_PROTECTED = (1 << 5),

} /* bfBufferPropertyFlags */;

typedef uint16_t bfBufferPropertyBits;

enum
{
  BF_BUFFER_USAGE_TRANSFER_SRC         = bfBit(0), /*!< Can be used to transfer data out of.             */
  BF_BUFFER_USAGE_TRANSFER_DST         = bfBit(1), /*!< Can be used to transfer data into.               */
  BF_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER = bfBit(2), /*!< Can be used to TODO                              */
  BF_BUFFER_USAGE_STORAGE_TEXEL_BUFFER = bfBit(3), /*!< Can be used to TODO                              */
  BF_BUFFER_USAGE_UNIFORM_BUFFER       = bfBit(4), /*!< Can be used to store Uniform data.               */
  BF_BUFFER_USAGE_STORAGE_BUFFER       = bfBit(5), /*!< Can be used to store SSBO data                   */
  BF_BUFFER_USAGE_INDEX_BUFFER         = bfBit(6), /*!< Can be used to store Index data.                 */
  BF_BUFFER_USAGE_VERTEX_BUFFER        = bfBit(7), /*!< Can be used to store Vertex data.                */
  BF_BUFFER_USAGE_INDIRECT_BUFFER      = bfBit(8), /*!< Can be used to store Indirect Draw Command data. */

  /*
     NOTE(SR):
       Allows for mapped allocations to be shared by keeping it
       persistently mapped until all refs to the shared buffer are freed.
       This functionality is managed by 'PoolAllocator' in `vulkan/bf_vulkan_mem_allocator.h`

     Requirements :
       'BF_BUFFER_PROP_HOST_MAPPABLE'
  */
  BF_BUFFER_USAGE_PERSISTENTLY_MAPPED_BUFFER = (1 << 9) /*!< Can be used for data that is streamed to the gpu */

} /* bfBufferUsageFlags */;

typedef uint16_t bfBufferUsageBits;

typedef enum
{
  BF_SHADER_TYPE_VERTEX                  = 0,
  BF_SHADER_TYPE_TESSELLATION_CONTROL    = 1,
  BF_SHADER_TYPE_TESSELLATION_EVALUATION = 2,
  BF_SHADER_TYPE_GEOMETRY                = 3,
  BF_SHADER_TYPE_FRAGMENT                = 4,
  BF_SHADER_TYPE_COMPUTE                 = 5,
  BF_SHADER_TYPE_MAX                     = 6,

} bfShaderType;

enum
{
  BF_SHADER_STAGE_VERTEX                  = bfBit(0),
  BF_SHADER_STAGE_TESSELLATION_CONTROL    = bfBit(1),
  BF_SHADER_STAGE_TESSELLATION_EVALUATION = bfBit(2),
  BF_SHADER_STAGE_GEOMETRY                = bfBit(3),
  BF_SHADER_STAGE_FRAGMENT                = bfBit(4),
  BF_SHADER_STAGE_COMPUTE                 = bfBit(5),
  BF_SHADER_STAGE_GRAPHICS                = BF_SHADER_STAGE_VERTEX |
                             BF_SHADER_STAGE_TESSELLATION_CONTROL |
                             BF_SHADER_STAGE_TESSELLATION_EVALUATION |
                             BF_SHADER_STAGE_GEOMETRY |
                             BF_SHADER_STAGE_FRAGMENT,

} /* bfShaderStageFlags */;

typedef uint8_t bfShaderStageBits;

typedef enum
{
  BF_TEX_TYPE_1D,
  BF_TEX_TYPE_2D,
  BF_TEX_TYPE_3D,

} bfTextureType;

typedef enum
{
  BF_SFM_NEAREST,
  BF_SFM_LINEAR,

} bfTexSamplerFilterMode;

typedef enum
{
  BF_SAM_REPEAT,
  BF_SAM_MIRRORED_REPEAT,
  BF_SAM_CLAMP_TO_EDGE,
  BF_SAM_CLAMP_TO_BORDER,
  BF_SAM_MIRROR_CLAMP_TO_EDGE,

} bfTexSamplerAddressMode;

typedef struct
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

typedef struct
{
  const char* app_name;
  uint32_t    app_version;

} bfGfxContextCreateParams;

/* Same as Vulkan's versioning system */
#define bfGfxMakeVersion(major, minor, patch) \
  (((major) << 22) | ((minor) << 12) | (patch))

typedef struct
{
  bfBufferSize         size;
  bfBufferPropertyBits properties;

} bfAllocationCreateInfo;

typedef struct
{
  bfAllocationCreateInfo allocation;
  bfBufferUsageBits      usage;

} bfBufferCreateParams;

typedef struct bfRenderpassInfo_t bfRenderpassCreateParams;

typedef struct
{
  const char* debug_name;
  uint32_t    num_desc_sets;

} bfShaderProgramCreateParams;

enum
{
  BF_TEX_IS_TRANSFER_SRC       = bfBit(0),
  BF_TEX_IS_TRANSFER_DST       = bfBit(1),
  BF_TEX_IS_SAMPLED            = bfBit(2),
  BF_TEX_IS_STORAGE            = bfBit(3),
  BF_TEX_IS_COLOR_ATTACHMENT   = bfBit(4),
  BF_TEX_IS_DEPTH_ATTACHMENT   = bfBit(5),
  BF_TEX_IS_STENCIL_ATTACHMENT = bfBit(6),
  BF_TEX_IS_TRANSIENT          = bfBit(7),
  BF_TEX_IS_INPUT_ATTACHMENT   = bfBit(8),
  BF_TEX_IS_MULTI_QUEUE        = bfBit(9),
  BF_TEX_IS_LINEAR             = bfBit(10),

} /* bfTexFeatureBits */;

typedef uint16_t bfTexFeatureFlags;

typedef struct
{
  bfTextureType     type;
  bfGfxImageFormat  format;
  uint32_t          width;
  uint32_t          height;
  uint32_t          depth;
  bfBool32          generate_mipmaps;
  uint32_t          num_layers;
  bfTexFeatureFlags flags;
  bfGfxSampleFlags  sample_count;

} bfTextureCreateParams;

BF_GFX_API bfTextureCreateParams bfTextureCreateParams_init2D(bfGfxImageFormat format, uint32_t width, uint32_t height);
BF_GFX_API bfTextureCreateParams bfTextureCreateParams_initCubeMap(uint32_t width, uint32_t height, bfGfxImageFormat format);
BF_GFX_API bfTextureCreateParams bfTextureCreateParams_initColorAttachment(uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);
BF_GFX_API bfTextureCreateParams bfTextureCreateParams_initDepthAttachment(uint32_t width, uint32_t height, bfGfxImageFormat format, bfBool32 can_be_input, bfBool32 is_transient);

typedef struct
{
  uint32_t thread_index;
  int      window_idx;

} bfGfxCommandListCreateParams;

typedef enum
{
  BF_STENCIL_FACE_FRONT,
  BF_STENCIL_FACE_BACK,

} bfStencilFace;

typedef struct
{
  uint32_t frame_index;
  uint32_t frame_count;
  uint32_t num_frame_indices;

} bfGfxFrameInfo;

/* Context */
BF_GFX_API bfGfxContextHandle     bfGfxContext_new(const bfGfxContextCreateParams* params);
BF_GFX_API bfGfxDeviceHandle      bfGfxContext_device(bfGfxContextHandle self);
BF_GFX_API bfWindowSurfaceHandle  bfGfxContext_createWindow(bfGfxContextHandle self, struct bfWindow* bf_window);
BF_GFX_API void                   bfGfxContext_destroyWindow(bfGfxContextHandle self, bfWindowSurfaceHandle window_handle);
BF_GFX_API bfBool32               bfGfxContext_beginFrame(bfGfxContextHandle self, bfWindowSurfaceHandle window);
BF_GFX_API bfGfxFrameInfo         bfGfxContext_getFrameInfo(bfGfxContextHandle self);
BF_GFX_API bfGfxCommandListHandle bfGfxContext_requestCommandList(bfGfxContextHandle self, bfWindowSurfaceHandle window, uint32_t thread_index);
BF_GFX_API void                   bfGfxContext_endFrame(bfGfxContextHandle self);
BF_GFX_API void                   bfGfxContext_delete(bfGfxContextHandle self);

// TODO(SR): This should not be in this Public API header. Needs to be made internal.
// \/ BEGIN Internal: \/
typedef enum
{
  BF_GFX_OBJECT_BUFFER         = 0,
  BF_GFX_OBJECT_RENDERPASS     = 1,
  BF_GFX_OBJECT_SHADER_MODULE  = 2,
  BF_GFX_OBJECT_SHADER_PROGRAM = 3,
  BF_GFX_OBJECT_DESCRIPTOR_SET = 4,
  BF_GFX_OBJECT_TEXTURE        = 5,
  BF_GFX_OBJECT_FRAMEBUFFER    = 6,
  BF_GFX_OBJECT_PIPELINE       = 7,

} bfGfxObjectType;

#define bfFrameCountMax 0xFFFFFFFF
typedef uint32_t bfFrameCount_t;

typedef struct bfBaseGfxObject bfBaseGfxObject;
struct bfBaseGfxObject
{
  bfGfxObjectType  type;
  bfBaseGfxObject* next;
  uint64_t         hash_code;
  bfFrameCount_t   last_frame_used;
};

void bfBaseGfxObject_ctor(bfBaseGfxObject* self, bfGfxObjectType type);
// ^ END Internal ^

/* Logical Device */
typedef struct  // bfDeviceLimits_t
{
  bfBufferSize uniform_buffer_offset_alignment; /* Worst case is 256 (0x100) */

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
 * @param resource 
 * @return 
*/
BF_GFX_API void bfGfxDevice_release(bfGfxDeviceHandle self, bfGfxBaseHandle resource);

/* Buffer */
BF_GFX_API bfBufferSize bfBuffer_size(bfBufferHandle self);
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
typedef struct
{
  bfTextureHandle  texture;  // [format, layouts[0], sample_count]
  bfGfxImageLayout final_layout;
  bfBool32         may_alias;

} bfAttachmentInfo;

typedef struct
{
  uint32_t                subpasses[2];             // [src, dst]
  bfGfxPipelineStageFlags pipeline_stage_flags[2];  // [src, dst]
  bfGfxAccessFlags        access_flags[2];          // [src, dst]
  bfBool32                reads_same_pixel;         // should be true in most cases (exception being bluring)

} bfSubpassDependency;

typedef uint16_t bfLoadStoreFlags;

typedef struct
{
  uint32_t         attachment_index;
  bfGfxImageLayout layout;

} bfAttachmentRefCache;

typedef struct
{
  uint16_t             num_out_attachment_refs;
  uint16_t             num_in_attachment_refs;
  bfAttachmentRefCache out_attachment_refs[k_bfGfxMaxAttachments];
  bfAttachmentRefCache in_attachment_refs[k_bfGfxMaxAttachments];
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
  bfSubpassCache      subpasses[k_bfGfxMaxSubpasses];
  bfAttachmentInfo    attachments[k_bfGfxMaxAttachments];
  bfSubpassDependency dependencies[k_bfGfxMaxRenderpassDependencies];

} bfRenderpassInfo;

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

/* Shader Program + Module */
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

/* Descriptor Set */
typedef enum
{
  BF_DESCRIPTOR_ELEMENT_TEXTURE,
  BF_DESCRIPTOR_ELEMENT_BUFFER,
  BF_DESCRIPTOR_ELEMENT_DYNAMIC_BUFFER,
  BF_DESCRIPTOR_ELEMENT_BUFFER_VIEW,
  BF_DESCRIPTOR_ELEMENT_INPUT_ATTACHMENT,

} bfDescriptorElementInfoType;

typedef struct
{
  bfDescriptorElementInfoType type;
  uint32_t                    binding;
  uint32_t                    array_element_start;
  uint32_t                    num_handles; /* also length of bfDescriptorElementInfo::offsets and bfDescriptorElementInfo::sizes */
  bfGfxBaseHandle             handles[k_bfGfxDesfcriptorSetMaxLayoutBindings];
  uint64_t                    offsets[k_bfGfxDesfcriptorSetMaxLayoutBindings];
  uint64_t                    sizes[k_bfGfxDesfcriptorSetMaxLayoutBindings];

} bfDescriptorElementInfo;

typedef struct
{
  bfDescriptorElementInfo bindings[k_bfGfxDesfcriptorSetMaxLayoutBindings];
  uint32_t                num_bindings;

} bfDescriptorSetInfo;

BF_GFX_API bfDescriptorSetInfo bfDescriptorSetInfo_make(void);
BF_GFX_API void                bfDescriptorSetInfo_addTexture(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures);
BF_GFX_API void                bfDescriptorSetInfo_addUniform(bfDescriptorSetInfo* self, uint32_t binding, uint32_t array_element_start, const uint64_t* offsets, const uint64_t* sizes, bfBufferHandle* buffers, uint32_t num_buffers);

/* The Descriptor Set API is for 'Immutable' Bindings otherwise use the bfDescriptorSetInfo API */

BF_GFX_API void bfDescriptorSet_setCombinedSamplerTextures(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures);
BF_GFX_API void bfDescriptorSet_setUniformBuffers(bfDescriptorSetHandle self, uint32_t binding, const bfBufferSize* offsets, const bfBufferSize* sizes, bfBufferHandle* buffers, uint32_t num_buffers);
BF_GFX_API void bfDescriptorSet_flushWrites(bfDescriptorSetHandle self);

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

/* CommandList */

typedef enum bfPipelineBarrierType_t
{
  BF_PIPELINE_BARRIER_MEMORY,
  BF_PIPELINE_BARRIER_BUFFER,
  BF_PIPELINE_BARRIER_IMAGE,

} bfPipelineBarrierType;

typedef struct
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

  } info;

} bfPipelineBarrier;

BF_GFX_API bfPipelineBarrier bfPipelineBarrier_memory(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access);
BF_GFX_API bfPipelineBarrier bfPipelineBarrier_buffer(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size);
BF_GFX_API bfPipelineBarrier bfPipelineBarrier_image(bfGfxAccessFlagsBits src_access, bfGfxAccessFlagsBits dst_access, bfTextureHandle image, bfGfxImageLayout new_layout);

BF_GFX_API bfWindowSurfaceHandle bfGfxCmdList_window(bfGfxCommandListHandle self);
BF_GFX_API void                  bfGfxCmdList_setDefaultPipeline(bfGfxCommandListHandle self);
BF_GFX_API bfBool32              bfGfxCmdList_begin(bfGfxCommandListHandle self);  // True if no error
BF_GFX_API void                  bfGfxCmdList_executionBarrier(bfGfxCommandListHandle self, bfGfxPipelineStageBits src_stage, bfGfxPipelineStageBits dst_stage, bfBool32 reads_same_pixel);
BF_GFX_API void                  bfGfxCmdList_pipelineBarriers(bfGfxCommandListHandle self, bfGfxPipelineStageBits src_stage, bfGfxPipelineStageBits dst_stage, const bfPipelineBarrier* barriers, uint32_t num_barriers, bfBool32 reads_same_pixel);
BF_GFX_API void                  bfGfxCmdList_setRenderpass(bfGfxCommandListHandle self, bfRenderpassHandle renderpass);
BF_GFX_API void                  bfGfxCmdList_setRenderpassInfo(bfGfxCommandListHandle self, const bfRenderpassInfo* renderpass_info);
BF_GFX_API void                  bfGfxCmdList_setClearValues(bfGfxCommandListHandle self, const bfClearValue* clear_values);
BF_GFX_API void                  bfGfxCmdList_setAttachments(bfGfxCommandListHandle self, bfTextureHandle* attachments);
BF_GFX_API void                  bfGfxCmdList_setRenderAreaAbs(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height);
BF_GFX_API void                  bfGfxCmdList_setRenderAreaRel(bfGfxCommandListHandle self, float x, float y, float width, float height);  // Normalized [0.0f, 1.0f] coords
BF_GFX_API void                  bfGfxCmdList_beginRenderpass(bfGfxCommandListHandle self);
BF_GFX_API void                  bfGfxCmdList_nextSubpass(bfGfxCommandListHandle self);
BF_GFX_API void                  bfGfxCmdList_setDrawMode(bfGfxCommandListHandle self, bfDrawMode draw_mode);
BF_GFX_API void                  bfGfxCmdList_setFrontFace(bfGfxCommandListHandle self, bfFrontFace front_face);
BF_GFX_API void                  bfGfxCmdList_setCullFace(bfGfxCommandListHandle self, bfCullFaceFlags cull_face);
BF_GFX_API void                  bfGfxCmdList_setDepthTesting(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setDepthWrite(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setDepthTestOp(bfGfxCommandListHandle self, bfCompareOp op);
BF_GFX_API void                  bfGfxCmdList_setStencilTesting(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setPrimitiveRestart(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setRasterizerDiscard(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setDepthBias(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setSampleShading(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setAlphaToCoverage(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setAlphaToOne(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setLogicOp(bfGfxCommandListHandle self, bfLogicOp op);
BF_GFX_API void                  bfGfxCmdList_setPolygonFillMode(bfGfxCommandListHandle self, bfPolygonFillMode fill_mode);
BF_GFX_API void                  bfGfxCmdList_setColorWriteMask(bfGfxCommandListHandle self, uint32_t output_attachment_idx, uint8_t color_mask);
BF_GFX_API void                  bfGfxCmdList_setColorBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendOp op);
BF_GFX_API void                  bfGfxCmdList_setBlendSrc(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void                  bfGfxCmdList_setBlendDst(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void                  bfGfxCmdList_setAlphaBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendOp op);
BF_GFX_API void                  bfGfxCmdList_setBlendSrcAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void                  bfGfxCmdList_setBlendDstAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor);
BF_GFX_API void                  bfGfxCmdList_setStencilFailOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op);
BF_GFX_API void                  bfGfxCmdList_setStencilPassOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op);
BF_GFX_API void                  bfGfxCmdList_setStencilDepthFailOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op);
BF_GFX_API void                  bfGfxCmdList_setStencilCompareOp(bfGfxCommandListHandle self, bfStencilFace face, bfCompareOp op);
BF_GFX_API void                  bfGfxCmdList_setStencilCompareMask(bfGfxCommandListHandle self, bfStencilFace face, uint8_t cmp_mask);
BF_GFX_API void                  bfGfxCmdList_setStencilWriteMask(bfGfxCommandListHandle self, bfStencilFace face, uint8_t write_mask);
BF_GFX_API void                  bfGfxCmdList_setStencilReference(bfGfxCommandListHandle self, bfStencilFace face, uint8_t ref_mask);
BF_GFX_API void                  bfGfxCmdList_setDynamicStates(bfGfxCommandListHandle self, uint16_t dynamic_states);
BF_GFX_API void                  bfGfxCmdList_setViewport(bfGfxCommandListHandle self, float x, float y, float width, float height, const float depth[2]);
BF_GFX_API void                  bfGfxCmdList_setScissorRect(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height);
BF_GFX_API void                  bfGfxCmdList_setBlendConstants(bfGfxCommandListHandle self, const float constants[4]);
BF_GFX_API void                  bfGfxCmdList_setLineWidth(bfGfxCommandListHandle self, float value);
BF_GFX_API void                  bfGfxCmdList_setDepthClampEnabled(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setDepthBoundsTestEnabled(bfGfxCommandListHandle self, bfBool32 value);
BF_GFX_API void                  bfGfxCmdList_setDepthBounds(bfGfxCommandListHandle self, float min, float max);
BF_GFX_API void                  bfGfxCmdList_setDepthBiasConstantFactor(bfGfxCommandListHandle self, float value);
BF_GFX_API void                  bfGfxCmdList_setDepthBiasClamp(bfGfxCommandListHandle self, float value);
BF_GFX_API void                  bfGfxCmdList_setDepthBiasSlopeFactor(bfGfxCommandListHandle self, float value);
BF_GFX_API void                  bfGfxCmdList_setMinSampleShading(bfGfxCommandListHandle self, float value);
BF_GFX_API void                  bfGfxCmdList_setSampleMask(bfGfxCommandListHandle self, uint32_t sample_mask);
BF_GFX_API void                  bfGfxCmdList_bindVertexDesc(bfGfxCommandListHandle self, bfVertexLayoutSetHandle vertex_set_layout);
BF_GFX_API void                  bfGfxCmdList_bindVertexBuffers(bfGfxCommandListHandle self, uint32_t first_binding, bfBufferHandle* buffers, uint32_t num_buffers, const uint64_t* offsets);
BF_GFX_API void                  bfGfxCmdList_bindIndexBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, uint64_t offset, bfGfxIndexType idx_type);
BF_GFX_API void                  bfGfxCmdList_bindProgram(bfGfxCommandListHandle self, bfShaderProgramHandle shader);
BF_GFX_API void                  bfGfxCmdList_bindDescriptorSets(bfGfxCommandListHandle self, uint32_t binding, bfDescriptorSetHandle* desc_sets, uint32_t num_desc_sets);  // Call after pipeline is setup.
BF_GFX_API void                  bfGfxCmdList_bindDescriptorSet(bfGfxCommandListHandle self, uint32_t set_index, const bfDescriptorSetInfo* desc_set_info);                 // Call after pipeline is setup.
BF_GFX_API void                  bfGfxCmdList_draw(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices);                                              // Draw Cmds
BF_GFX_API void                  bfGfxCmdList_drawInstanced(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices, uint32_t first_instance, uint32_t num_instances);
BF_GFX_API void                  bfGfxCmdList_drawIndexed(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset);
BF_GFX_API void                  bfGfxCmdList_drawIndexedInstanced(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset, uint32_t first_instance, uint32_t num_instances);
BF_GFX_API void                  bfGfxCmdList_executeSubCommands(bfGfxCommandListHandle self, bfGfxCommandListHandle* commands, uint32_t num_commands);  // General
BF_GFX_API void                  bfGfxCmdList_endRenderpass(bfGfxCommandListHandle self);
BF_GFX_API void                  bfGfxCmdList_end(bfGfxCommandListHandle self);
BF_GFX_API void                  bfGfxCmdList_updateBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size, const void* data);  // Outside Renderpass
BF_GFX_API void                  bfGfxCmdList_submit(bfGfxCommandListHandle self);

// TODO: This function should not be in this file...
BF_GFX_API char* LoadFileIntoMemory(const char* filename, long* out_size);

#if __cplusplus
}
#endif

#if __cplusplus >= 201703L

template<typename T>
static constexpr bfGfxIndexType bfIndexTypeFromT()
{
  // TODO(SR): This is not a very robust check, but I mean don't mess up.

  static_assert(sizeof(T) == 2 || sizeof(T) == 4, "An index type must either be a uint16 or a uint32");

  return sizeof(T) == 2 ? BF_INDEX_TYPE_UINT16 : BF_INDEX_TYPE_UINT32;
}

#endif

#endif /* BF_GFX_API_H */
