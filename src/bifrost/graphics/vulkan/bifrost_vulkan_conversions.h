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
VkSampleCountFlagBits bfVkConvertSampleCount(BifrostSampleFlags bit);  // BifrostSampleFlags
VkClearValue          bfVKConvertClearColor(const BifrostClearValue* color);
VkIndexType           bfVkConvertIndexType(BifrostIndexType idx_type);
VkShaderStageFlagBits bfVkConvertShaderType(BifrostShaderType type);
#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_CONVERSIONS_H */
