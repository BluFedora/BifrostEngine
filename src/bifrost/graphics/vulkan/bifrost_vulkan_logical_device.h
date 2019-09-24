#ifndef BIFROST_VULKAN_LOGICAL_DEVICE_H
#define BIFROST_VULKAN_LOGICAL_DEVICE_H

#include "bifrost/graphics/bifrost_gfx_api.h"
#include "bifrost_vulkan_hash.hpp"
#include "bifrost_vulkan_physical_device.h"

BIFROST_DEFINE_HANDLE(GfxDevice)
{
  VulkanPhysicalDevice*                       parent;
  VkDevice                                    handle;
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

BIFROST_DEFINE_HANDLE(GfxCommandList)
{
  bfGfxDeviceHandle   parent;
  VkCommandBuffer     handle;
  VkRect2D            render_area;
  bfRenderpassHandle  renderpass;
  bfFramebufferHandle framebuffer;
  bfPipelineHandle    pipeline;
  bfPipelineCache     pipeline_state;
  VkClearValue        clear_colors[BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS];
};

BIFROST_DEFINE_HANDLE(Buffer)
{
  BifrostGfxObjectBase super;
  // PoolAllocator* alloc_pool;
  VkBuffer handle;
  // Allocation     alloc_info;  // This has the aligned size.
  bfBufferSize real_size;
};

BIFROST_DEFINE_HANDLE(ShaderModule)
{
  BifrostShaderType type;
  VkShaderModule    handle;
  char              entry_point[BIFROST_GFX_SHADER_ENTRY_POINT_NAME_LENGTH];
};

typedef struct
{
  size_t                size;
  bfShaderModuleHandle* elements;

} bfShaderModuleList;

BIFROST_DEFINE_HANDLE(ShaderProgram)
{
  BifrostGfxObjectBase super;
  VkPipelineLayout     layout;
  bfShaderModuleList   modules;
};

BIFROST_DEFINE_HANDLE(DescriptorSet)
{
  BifrostGfxObjectBase super;
  VkDescriptorSet      handle;
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