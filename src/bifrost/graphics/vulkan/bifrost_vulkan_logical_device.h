#ifndef BIFROST_VULKAN_LOGICAL_DEVICE_H
#define BIFROST_VULKAN_LOGICAL_DEVICE_H

#include "bifrost/graphics/bifrost_gfx_api.h"
#include "bifrost_vulkan_hash.hpp"
#include "bifrost_vulkan_physical_device.h"
#include "bifrost_vulkan_mem_allocator.h"
#include "bifrost_vulkan_material_pool.h"

BIFROST_DEFINE_HANDLE(GfxDevice)
{
  VulkanPhysicalDevice*                       parent;
  VkDevice                                    handle;
  PoolAllocator                               device_memory_allocator;
  VulkanDescriptorPool*                       descriptor_pool;
  VkQueue                                     queues[BIFROST_GFX_QUEUE_MAX];
  bifrost::vk::ObjectHashCache<bfRenderpass>  cache_renderpass;
  bifrost::vk::ObjectHashCache<bfPipeline>    cache_pipeline;
  bifrost::vk::ObjectHashCache<bfFramebuffer> cache_framebuffer;
};

#if __cplusplus
extern "C" {
#endif
BIFROST_DEFINE_HANDLE(Renderpass)
{
  BifrostGfxObjectBase super;
  VkRenderPass         handle;
  bfRenderpassInfo     info;
  uint64_t             hash_code;
};

BIFROST_DEFINE_HANDLE(Framebuffer)
{
  VkFramebuffer   handle;
  uint32_t        num_attachments;
  bfTextureHandle attachments[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
};

BIFROST_DEFINE_HANDLE(Pipeline)
{
  VkPipeline handle;
};

struct VulkanWindow final
{
  VkSurfaceKHR        surface;
  VulkanSwapchainInfo swapchain_info;
  VulkanSwapchain     swapchain;
  VkSemaphore*        is_image_available;
  VkSemaphore*        is_render_done;
  uint32_t            image_index;
  bfBool32            swapchain_needs_creation;
};

BIFROST_DEFINE_HANDLE(GfxCommandList)
{
  bfGfxContextHandle  context;
  bfGfxDeviceHandle   parent;
  VkCommandBuffer     handle;
  VkFence             fence;
  VulkanWindow*       window;
  VkRect2D            render_area;
  bfFramebufferHandle framebuffer;
  bfPipelineHandle    pipeline;
  bfPipelineCache     pipeline_state;
  VkClearValue        clear_colors[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
  bfBool32            has_command;
};

BIFROST_DEFINE_HANDLE(Buffer)
{
  BifrostGfxObjectBase super;
  PoolAllocator*       alloc_pool;
  VkBuffer             handle;
  Allocation           alloc_info;  // This has the aligned size.
  bfBufferSize         real_size;
};

BIFROST_DEFINE_HANDLE(ShaderModule)
{
  BifrostGfxObjectBase super;
  bfGfxDeviceHandle    parent;
  BifrostShaderType    type;
  VkShaderModule       handle;
  char                 entry_point[BIFROST_GFX_SHADER_ENTRY_POINT_NAME_LENGTH];
};

typedef struct
{
  size_t               size;
  bfShaderModuleHandle elements[BIFROST_SHADER_TYPE_MAX];

} bfShaderModuleList;

typedef struct
{
  size_t                       num_layout_bindings;
  VkDescriptorSetLayoutBinding layout_bindings[BIFROST_GFX_DESCRIPTOR_SET_LAYOUT_MAX_BINDINGS];
  uint32_t                     num_image_samplers;
  uint32_t                     num_uniforms;

} bfDescriptorSetLayoutInfo;

BIFROST_DEFINE_HANDLE(ShaderProgram)
{
  BifrostGfxObjectBase      super;
  bfGfxDeviceHandle         parent;
  VkPipelineLayout          layout;
  uint32_t                  num_desc_set_layouts;
  VkDescriptorSetLayout     desc_set_layouts[BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS];
  bfDescriptorSetLayoutInfo desc_set_layout_infos[BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS];
  bfShaderModuleList        modules;
  char                      debug_name[BIFROST_GFX_SHADER_PROGRAM_NAME_LENGTH];
};

BIFROST_DEFINE_HANDLE(DescriptorSet)
{
  BifrostGfxObjectBase   super;
  bfShaderProgramHandle  shader_program;
  VkDescriptorSet        handle;
  uint32_t               set_index;
  DescriptorLink*        pool_link;
  VkDescriptorBufferInfo buffer_info[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  VkDescriptorImageInfo  image_info[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  VkBufferView           buffer_view_info[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  VkWriteDescriptorSet   writes[BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES];
  uint16_t               num_buffer_info;
  uint16_t               num_image_info;
  uint16_t               num_buffer_view_info;
  uint16_t               num_writes;
};

BIFROST_DEFINE_HANDLE(VertexLayoutSet)
{
  VkVertexInputBindingDescription   buffer_bindings[BIFROST_GFX_VERTEX_LAYOUT_MAX_BINDINGS];
  VkVertexInputAttributeDescription attrib_bindings[BIFROST_GFX_VERTEX_LAYOUT_MAX_BINDINGS];
  uint8_t                           num_buffer_bindings;
  uint8_t                           num_attrib_bindings;
};

#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_LOGICAL_DEVICE_H */
