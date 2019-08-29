#ifndef BIFROST_VULKAN_MATERIAL_POOL_H
#define BIFROST_VULKAN_MATERIAL_POOL_H

#include "bifrost/graphics/bifrost_gfx_api.h"
#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif
// Material Pool
typedef struct DescriptorLink_t
{
  VkDescriptorPool         handle;
  uint32_t                 num_textures_left;
  uint32_t                 num_uniforms_left;
  uint32_t                 num_descsets_left;
  struct DescriptorLink_t* next;

} DescriptorLink;

typedef struct MaterialPoolCreateParams_t
{
  const bfGfxDevice* logical_device;
  uint32_t           num_textures_per_link;
  uint32_t           num_uniforms_per_link;
  uint32_t           num_descsets_per_link;

} MaterialPoolCreateParams;

typedef struct MaterialPool_t
{
  MaterialPoolCreateParams super;
  DescriptorLink*          head;

} MaterialPool;

MaterialPool* MaterialPool_new(const MaterialPoolCreateParams* const params);
void          MaterialPool_delete(MaterialPool* const self);

typedef MaterialPool VulkanDescriptorPool;
#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_MATERIAL_POOL_H */
