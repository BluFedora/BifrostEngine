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
VkShaderStageFlagBits bfVkConvertShaderStage(BifrostShaderStageFlags flags);
VkPrimitiveTopology   bfVkConvertTopology(BifrostDrawMode draw_mode);
VkViewport            bfVkConvertViewport(const BifrostViewport* viewport);
VkRect2D              bfVkConvertScissorRect(const BifrostScissorRect* viewport);
VkPolygonMode         bfVkConvertPolygonMode(BifrostPolygonFillMode polygon_mode);
VkCullModeFlags       bfVkConvertCullModeFlags(uint32_t cull_face_flags);  // BifrostCullFaceFlags
VkFrontFace           bfVkConvertFrontFace(BifrostFrontFace front_face);
VkFormat              bfVkConvertVertexFormatAttrib(BifrostVertexFormatAttribute vertex_format_attrib);
VkFlags               bfVkConvertBufferUsageFlags(uint16_t flags);  // bfBufferUsageFlags
VkFlags               bfVkConvertBufferPropertyFlags(uint16_t flags);  // bfBufferProperyFlags
VkImageType           bfVkConvertTextureType(BifrostTextureType type);
VkFilter              bfVkConvertSamplerFilterMode(BifrostSamplerFilterMode mode);
VkSamplerAddressMode  bfVkConvertSamplerAddressMode(BifrostSamplerAddressMode mode);
VkCompareOp           bfVkConvertCompareOp(BifrostCompareOp op);
VkStencilOp           bfVkConvertStencilOp(BifrostStencilOp op);
VkLogicOp             bfVkConvertLogicOp(BifrostLogicOp op);
VkBlendFactor         bfVkConvertBlendFactor(BifrostBlendFactor factor);
VkBlendOp             bfVkConvertBlendOp(BifrostBlendOp factor);
VkFlags               bfVkConvertColorMask(uint16_t flags); // BifrostColorMask

 // Internal API

VkImageView bfCreateImageView(
 VkDevice           device,
 VkImage            image,
 VkImageViewType    view_type,
 VkFormat           format,
 VkImageAspectFlags aspect_flags,
 uint32_t           base_mip_level,
 uint32_t           base_array_layer,
 uint32_t           mip_levels,
 uint32_t           layer_count);
VkImageView bfCreateImageView2D(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_CONVERSIONS_H */
