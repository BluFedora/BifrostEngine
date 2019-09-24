#ifndef BIFROST_VULKAN_CONVERSIONS_H
#define BIFROST_VULKAN_CONVERSIONS_H

#include "bifrost/graphics/bifrost_gfx_api.h"
#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif
VkFormat              bfVkConvertFormat(BifrostImageFormat format);
VkImageLayout         bfVkConvertImgLayout(BifrostImageLayout layout);
VkSampleCountFlags    bfVkConvertSampleFlags(uint32_t flags);          // BifrostSampleFlags
VkSampleCountFlagBits bfVkConvertSampleCount(BifrostSampleFlags bit);
VkClearValue          bfVKConvertClearColor(const BifrostClearValue* color);
VkIndexType           bfVkConvertIndexType(BifrostIndexType idx_type);
VkShaderStageFlagBits bfVkConvertShaderType(BifrostShaderType type);
VkPrimitiveTopology   bfVkConvertTopology(BifrostDrawMode draw_mode);
VkViewport            bfVkConvertViewport(const BifrostViewport* viewport);
VkRect2D              bfVkConvertScissorRect(const BifrostScissorRect* viewport);
VkPolygonMode         bfVkConvertPolygonMode(BifrostPolygonFillMode polygon_mode);
VkCullModeFlags       bfVkConvertCullModeFlags(uint32_t cull_face_flags);  // BifrostCullFaceFlags
VkFrontFace           bfVkConvertFrontFace(BifrostFrontFace front_face);
#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_CONVERSIONS_H */
