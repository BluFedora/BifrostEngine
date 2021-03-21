/******************************************************************************/
/*!
 * @file   bf_gfx_resource.h
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Shared base interface for all graphics objects.
 *
 * @version 0.0.1
 * @date    2021-03-19
 *
 * @copyright Copyright (c) 2021
 */
/******************************************************************************/
#ifndef BF_GFX_RESOURCE_H
#define BF_GFX_RESOURCE_H

#include "bf_gfx_handle.h"
#include "bf_gfx_limits.h"
#include "bf_gfx_types.h"

#include <stdint.h> /* uint64_t, uint32_t */

#if BF_GFX_VULKAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VC_EXTRALEAN
#define WINDOWS_EXTRA_LEAN
#include <vulkan/vulkan.h>
#elif BF_GFX_OPENGL
#include <bf/platform/bf_platform_gl.h>
#endif

#if __cplusplus
extern "C" {
#endif

#define bfFrameCountMax 0xFFFFFFFF
typedef uint32_t bfFrameCount_t;

typedef enum bfGfxObjectType
{
  BF_GFX_OBJECT_BUFFER         = 0,
  BF_GFX_OBJECT_RENDERPASS     = 1,
  BF_GFX_OBJECT_SHADER_MODULE  = 2,
  BF_GFX_OBJECT_SHADER_PROGRAM = 3,
  BF_GFX_OBJECT_DESCRIPTOR_SET = 4,
  BF_GFX_OBJECT_TEXTURE        = 5,
  BF_GFX_OBJECT_FRAMEBUFFER    = 6,
  BF_GFX_OBJECT_PIPELINE       = 7,

} bfGfxObjectType;  // 3 bits worth of data.

typedef struct bfBaseGfxObject bfBaseGfxObject;
struct bfBaseGfxObject
{
  /* bfGfxObjectType */ uint32_t type : 3;
  uint32_t                       id : 29;
  bfFrameCount_t                 last_frame_used;
  bfBaseGfxObject*               next;
  uint64_t                       hash_code;
};

typedef struct bGfxObjectManager
{
  bfBaseGfxObject* gc[k_bfGfxMaxFrameGPUAhead];
  uint32_t         next_id;

} bGfxObjectManager;

static inline void bGfxObjectManager_init(bGfxObjectManager* self)
{
  for (int i = 0u; i < k_bfGfxMaxFrameGPUAhead; ++i)
  {
    self->gc[i] = NULL;
  }

  self->next_id = 0u;
}


static inline void bGfxObjectManager_add(bGfxObjectManager* self, bfBaseGfxObject* obj, bfFrameCount_t current_frame, bfFrameCount_t max_frames_ahead)
{
  const bfFrameCount_t idx = current_frame % max_frames_ahead;

  obj->next     = self->gc[idx];
  self->gc[idx] = obj;

  obj->last_frame_used = current_frame;
}

void bfBaseGfxObject_ctor(bfBaseGfxObject* self, bfGfxObjectType type, bGfxObjectManager* manager);

/* Buffer */

typedef uint64_t bfBufferSize;

#if BF_GFX_VULKAN
typedef struct Allocation
{
  VkDeviceMemory handle;
  uint32_t       type;
  uint32_t       index;
  bfBufferSize   size;  // This is the aligned size.
  bfBufferSize   offset;
  void*          mapped_ptr;

} Allocation;

typedef struct PoolAllocator PoolAllocator;
#endif

BF_DEFINE_GFX_HANDLE(Buffer)
{
  bfBaseGfxObject super;
  bfBufferSize    real_size;

#if BF_GFX_VULKAN
  PoolAllocator*    alloc_pool;
  VkBuffer          handle;
  Allocation        alloc_info;  // This has the aligned size.
  bfBufferUsageBits usage;
#elif BF_GFX_OPENGL
  GLuint                     tex_image; /* For Depth Textures this is an RBO */
  bfTextureSamplerProperties tex_sampler;
  bfGfxSampleFlags           tex_samples;
#endif
};

/* Texture */

BF_DEFINE_GFX_HANDLE(Texture)
{
  // General Metadata
  bfBaseGfxObject   super;
  bfGfxDeviceHandle parent;
  bfTexFeatureFlags flags;

  // CPU Side Data
  bfTextureType image_type;
  int32_t       image_width;
  int32_t       image_height;
  int32_t       image_depth;
  uint32_t      image_miplevels;

  // GPU Side Data
#if BF_GFX_VULKAN
  bfBufferPropertyBits memory_properties;
  VkImage              tex_image;
  VkDeviceMemory       tex_memory;
  VkImageView          tex_view;
  VkSampler            tex_sampler;
  bfGfxImageLayout     tex_layout;
  VkFormat             tex_format;
  bfGfxSampleFlags     tex_samples;
#elif BF_GFX_OPENGL
  GLuint handle;
  GLenum target;
  GLenum usage;
  void*  mapped_ptr;
#endif
};

/* Renderpass */

typedef struct bfAttachmentRefCache
{
  uint32_t         attachment_index;
  bfGfxImageLayout layout;

} bfAttachmentRefCache;

typedef struct bfSubpassCache
{
  uint16_t             num_out_attachment_refs;
  uint16_t             num_in_attachment_refs;
  bfAttachmentRefCache out_attachment_refs[k_bfGfxMaxAttachments];
  bfAttachmentRefCache in_attachment_refs[k_bfGfxMaxAttachments];
  bfAttachmentRefCache depth_attachment;

} bfSubpassCache;

typedef struct bfAttachmentInfo
{
  bfTextureHandle  texture;  // [format, layouts[0], sample_count]
  bfGfxImageLayout final_layout;
  bfBool32         may_alias;

} bfAttachmentInfo;

typedef struct bfSubpassDependency
{
  uint32_t                subpasses[2];             // [src, dst]
  bfGfxPipelineStageFlags pipeline_stage_flags[2];  // [src, dst]
  bfGfxAccessFlags        access_flags[2];          // [src, dst]
  bfBool32                reads_same_pixel;         // should be true in most cases (exception being bluring)

} bfSubpassDependency;

typedef struct bfRenderpassInfo
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

BF_DEFINE_GFX_HANDLE(Renderpass)
{
  bfBaseGfxObject  super;
  bfRenderpassInfo info;

#if BF_GFX_VULKAN
  VkRenderPass handle;
#endif
};

/* Framebuffer */

BF_DEFINE_GFX_HANDLE(Framebuffer)
{
  bfBaseGfxObject super;

#if BF_GFX_VULKAN
  VkFramebuffer handle;
#elif BF_GFX_OPENGL
  GLuint          handle;
  bfTextureHandle attachments[k_bfGfxMaxAttachments];
#endif
};

/* Pipeline */

BF_DEFINE_GFX_HANDLE(Pipeline)
{
  bfBaseGfxObject super;

#if BF_GFX_VULKAN
  VkPipeline handle;
#endif
};

/* Command List */

BF_DEFINE_GFX_HANDLE(GfxCommandList)
{
  bfGfxDeviceHandle     parent;
  bfWindowSurfaceHandle window;
  VkRect2D              render_area;
  bfFramebufferHandle   framebuffer;
  bfPipelineCache       pipeline_state;
  bfPipelineHandle      pipeline;
  uint16_t              dynamic_state_dirty;
  bfBool16              has_command;

#if BF_GFX_VULKAN
  VkClearValue    clear_colors[k_bfGfxMaxAttachments];
  VkCommandBuffer handle;
  VkFence         fence;
  uint32_t        attachment_size[2];
#elif BF_GFX_OPENGL
  bfClearValue       clear_colors[k_bfGfxMaxAttachments];
  bfGfxContextHandle context;
  bfGfxIndexType     index_type;
  uint64_t           index_offset;
#endif
};

/* Shader Program / Module */

BF_DEFINE_GFX_HANDLE(ShaderModule)
{
  bfBaseGfxObject   super;
  bfGfxDeviceHandle parent;
  bfShaderType      type;
  char              entry_point[k_bfGfxShaderEntryPointNameLength];

#if BF_GFX_VULKAN
  VkShaderModule handle;
#elif BF_GFX_OPENGL
  GLuint handle;
#endif
};

#if BF_GFX_VULKAN
typedef struct bfDescriptorSetLayoutInfo
{
  uint32_t                     num_layout_bindings;
  VkDescriptorSetLayoutBinding layout_bindings[k_bfGfxDesfcriptorSetMaxLayoutBindings];
  uint32_t                     num_image_samplers;
  uint32_t                     num_uniforms;

} bfDescriptorSetLayoutInfo;

typedef struct bfShaderModuleList
{
  uint32_t             size;
  bfShaderModuleHandle elements[BF_SHADER_TYPE_MAX];

} bfShaderModuleList;
#elif BF_GFX_OPENGL
struct DescSetInfo
{
  int num_textures;
  int texture_offset;
};
#endif

BF_DEFINE_GFX_HANDLE(ShaderProgram)
{
  bfBaseGfxObject   super;
  bfGfxDeviceHandle parent;
  char              debug_name[k_bfGfxShaderProgramNameLength];

#if BF_GFX_VULKAN
  VkPipelineLayout          layout;
  uint32_t                  num_desc_set_layouts;
  VkDescriptorSetLayout     desc_set_layouts[k_bfGfxDescriptorSets];
  bfDescriptorSetLayoutInfo desc_set_layout_infos[k_bfGfxDescriptorSets];
  bfShaderModuleList        modules;
#elif BF_GFX_OPENGL
  GLuint handle;
  DescSetInfo set_info[k_bfGfxDescriptorSets];
  uint32_t num_sets;
  // StdUnorderedMap<uint32_t, GLint> binding_to_uniform_loc;
#endif
};

BF_DEFINE_GFX_HANDLE(DescriptorSet)
{
  bfBaseGfxObject       super;
  bfShaderProgramHandle shader_program;

#if BF_GFX_VULKAN
  VkDescriptorSet        handle;
  uint32_t               set_index;
  struct DescriptorLink* pool_link;
  VkDescriptorBufferInfo buffer_info[k_bfGfxMaxDescriptorSetWrites];
  VkDescriptorImageInfo  image_info[k_bfGfxMaxDescriptorSetWrites];
  VkBufferView           buffer_view_info[k_bfGfxMaxDescriptorSetWrites];
  VkWriteDescriptorSet   writes[k_bfGfxMaxDescriptorSetWrites];
  uint16_t               num_buffer_info;
  uint16_t               num_image_info;
  uint16_t               num_buffer_view_info;
  uint16_t               num_writes;
#elif BF_GFX_OPENGL
  uint32_t set_index;
  // StdVector<std::pair<GLuint, bfTextureHandle>>                               textures;        /* <Uniform, Texture>              */
  // StdVector<std::tuple<uint32_t, bfBufferSize, bfBufferSize, bfBufferHandle>> ubos;            /* <Binding, Offset, Size, Buffer> */
  // StdVector<std::pair<GLuint, bfTextureHandle>>                               textures_writes; /* <Uniform, Texture>              */
  // StdVector<std::tuple<uint32_t, bfBufferSize, bfBufferSize, bfBufferHandle>> ubos_writes;     /* <Binding, Offset, Size, Buffer> */
#endif
};

/* Descriptor Set */

typedef enum bfDescriptorElementInfoType
{
  BF_DESCRIPTOR_ELEMENT_TEXTURE,
  BF_DESCRIPTOR_ELEMENT_BUFFER,
  BF_DESCRIPTOR_ELEMENT_DYNAMIC_BUFFER,
  BF_DESCRIPTOR_ELEMENT_BUFFER_VIEW,
  BF_DESCRIPTOR_ELEMENT_INPUT_ATTACHMENT,

} bfDescriptorElementInfoType;

typedef struct bfDescriptorElementInfo
{
  bfDescriptorElementInfoType type;
  uint32_t                    binding;
  uint32_t                    array_element_start;
  uint32_t                    num_handles; /* also length of bfDescriptorElementInfo::offsets and bfDescriptorElementInfo::sizes */
  bfGfxBaseHandle             handles[2];
  uint64_t                    offsets[2];
  uint64_t                    sizes[2];

} bfDescriptorElementInfo;

typedef struct bfDescriptorSetInfo
{
  bfDescriptorElementInfo bindings[k_bfGfxDesfcriptorSetMaxLayoutBindings];
  uint32_t                num_bindings;

} bfDescriptorSetInfo;

#if __cplusplus
}
#endif

#endif /* BF_GFX_RESOURCE_H */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2021 Shareef Abdoul-Raheem

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