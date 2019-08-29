#include "bifrost_vulkan_material_pool.h"

#include "bifrost_vulkan_logical_device.h"

#include <stdlib.h>
#include <string.h> // memcpy

static DescriptorLink* create_link(const MaterialPoolCreateParams* const pool, DescriptorLink* next)
{
  DescriptorLink* link = (DescriptorLink*)malloc(sizeof(DescriptorLink));
  #if 0
  link->num_textures_left = pool->num_textures_per_link;
  link->num_uniforms_left = pool->num_uniforms_per_link;
  link->num_descsets_left = pool->num_descsets_per_link;
  link->next              = next;

  const VkDescriptorPoolSize pool_sizes[] =
   {
    // Type : Num to Alloc of that Type
    {
     .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
     .descriptorCount = pool->num_textures_per_link,
    },
    {
     .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
     .descriptorCount = pool->num_uniforms_per_link,
    },
   };

  const VkDescriptorPoolCreateInfo pool_create_info =
   {
    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    .pNext         = NULL,
    .flags         = 0,  //VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT - If I wanted to free individual sets
    .maxSets       = pool->num_descsets_per_link,
    .poolSizeCount = 2,
    .pPoolSizes    = pool_sizes,
   };

  vkCreateDescriptorPool(
   pool->logical_device->handle,
   &pool_create_info,
    NULL,
   &link->handle);
  #endif
  return link;
}

static void free_link(const bfGfxDevice* device, DescriptorLink* const link)
{
  vkDestroyDescriptorPool(device->handle, link->handle, NULL);
  free(link);
}

// MaterialPool API

MaterialPool* MaterialPool_new(const MaterialPoolCreateParams* const params)
{
  MaterialPool* const self = (MaterialPool*)malloc(sizeof(MaterialPool));

  memcpy(self, params, sizeof(MaterialPoolCreateParams));
  self->head = create_link(params, NULL);

  return self;
}
/*
static void MaterialPool_alloc(MaterialPool* const self, Material* const material)
{
  DescriptorLink* link = self->head;

  if (
   link->num_textures_left < material->num_textures ||
   link->num_uniforms_left < material->num_uniforms ||
   link->num_descsets_left <= 0)
  {
    const MaterialPoolCreateParams params = {
     .logical_device        = self->super.logical_device,
     .num_textures_per_link = max(material->num_textures, self->super.num_textures_per_link),
     .num_uniforms_per_link = max(material->num_uniforms, self->super.num_uniforms_per_link),
     .num_descsets_per_link = self->super.num_descsets_per_link};

    self->head = link = create_link(&params, link);
  }

  const VkDescriptorSetAllocateInfo alloc_info = {
   .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
   .descriptorPool     = link->handle,
   .descriptorSetCount = 1,
   .pSetLayouts        = &material->parent->program->desc_set_layout};

  vkAllocateDescriptorSets(self->super.logical_device->handle, &alloc_info, &material->handle);

  link->num_textures_left -= material->num_textures;
  link->num_uniforms_left -= material->num_uniforms;
  --link->num_descsets_left;
}
*/
void MaterialPool_delete(MaterialPool* const self)
{
  DescriptorLink* link = self->head;

  while (link)
  {
    DescriptorLink* const next = link->next;
    free_link(self->super.logical_device, link);
    link = next;
  }

  free(self);
}
