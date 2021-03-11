#ifndef BIFROST_VULKAN_MATERIAL_POOL_H
#define BIFROST_VULKAN_MATERIAL_POOL_H

#include "bf/bf_gfx_api.h"

#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif
typedef struct DescriptorLink DescriptorLink;
typedef struct DescriptorLink
{
  VkDescriptorPool handle;
  uint32_t         num_textures_left;
  uint32_t         num_uniforms_left;
  uint32_t         num_descsets_left;
  uint32_t         num_active_desc_sets;
  DescriptorLink*  prev;
  DescriptorLink*  next;

} DescriptorLink;

typedef struct MaterialPoolCreateParams
{
  const bfGfxDevice* logical_device;
  uint32_t           num_textures_per_link;
  uint32_t           num_uniforms_per_link;
  uint32_t           num_descsets_per_link;

} MaterialPoolCreateParams;

typedef struct MaterialPool
{
  MaterialPoolCreateParams super;
  DescriptorLink*          head;

} MaterialPool;

MaterialPool* MaterialPool_new(const MaterialPoolCreateParams* params);
void          MaterialPool_alloc(MaterialPool* self, bfDescriptorSetHandle desc_set);
void          MaterialPool_free(MaterialPool* self, bfDescriptorSetHandle desc_set);
void          MaterialPool_delete(MaterialPool* self);

typedef MaterialPool VulkanDescriptorPool;
#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_MATERIAL_POOL_H */
