#include "bifrost_vulkan_material_pool.h"

#include "bifrost_vulkan_logical_device.h"

#include <stdlib.h>
#include <string.h>  // memcpy

static DescriptorLink* create_link(const MaterialPoolCreateParams* const pool, DescriptorLink* next)
{
  DescriptorLink* link = (DescriptorLink*)malloc(sizeof(DescriptorLink));

  if (link)
  {
    link->num_textures_left = pool->num_textures_per_link;
    link->num_uniforms_left = pool->num_uniforms_per_link;
    link->num_descsets_left = pool->num_descsets_per_link;
    link->next              = next;

    // Type : Num to Alloc of that Type
    VkDescriptorPoolSize pool_sizes[2];

    pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[0].descriptorCount = pool->num_textures_per_link;

    pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[1].descriptorCount = pool->num_uniforms_per_link;

    VkDescriptorPoolCreateInfo pool_create_info;
    pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_create_info.pNext         = NULL;
    pool_create_info.flags         = 0x0;  //VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT - If I wanted to free individual sets
    pool_create_info.maxSets       = pool->num_descsets_per_link;
    pool_create_info.poolSizeCount = bfCArraySize(pool_sizes);
    pool_create_info.pPoolSizes    = pool_sizes;

    vkCreateDescriptorPool(
     pool->logical_device->handle,
     &pool_create_info,
     NULL,
     &link->handle);
  }

  return link;
}

static void free_link(const bfGfxDevice* device, DescriptorLink* const link)
{
  vkDestroyDescriptorPool(device->handle, link->handle, NULL);
  free(link);
}

BifrostDescriptorPool* MaterialPool_new(const MaterialPoolCreateParams* const params)
{
  BifrostDescriptorPool* const self = (BifrostDescriptorPool*)malloc(sizeof(BifrostDescriptorPool));

  if (self)
  {
    memcpy(self, params, sizeof(MaterialPoolCreateParams));
    self->head = create_link(params, NULL);
  }

  return self;
}

void MaterialPool_alloc(BifrostDescriptorPool* const self, bfDescriptorSetHandle desc_set)
{
  bfShaderProgramHandle shader = desc_set->shader_program;
  bfDescriptorSetLayoutInfo* info   = shader->desc_set_layout_infos + desc_set->set_index;

  DescriptorLink* link = self->head;

  if (link->num_textures_left < info->num_image_samplers || link->num_uniforms_left < info->num_uniforms || link->num_descsets_left <= 0)
  {
    MaterialPoolCreateParams params;
    params.logical_device        = self->super.logical_device;
    params.num_textures_per_link = max(info->num_image_samplers, self->super.num_textures_per_link);
    params.num_uniforms_per_link = max(info->num_uniforms, self->super.num_uniforms_per_link);
    params.num_descsets_per_link = self->super.num_descsets_per_link;

    self->head = link = create_link(&params, link);
  }

  VkDescriptorSetAllocateInfo alloc_info;
  alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.pNext              = nullptr;
  alloc_info.descriptorPool     = link->handle;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts        = shader->desc_set_layouts + desc_set->set_index;

  vkAllocateDescriptorSets(self->super.logical_device->handle, &alloc_info, &desc_set->handle);

  link->num_textures_left -= info->num_image_samplers;
  link->num_uniforms_left -= info->num_uniforms;
  --link->num_descsets_left;
}

void MaterialPool_delete(BifrostDescriptorPool* const self)
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
