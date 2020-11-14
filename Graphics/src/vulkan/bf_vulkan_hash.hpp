#ifndef BIFROST_VULKAN_HASH_HPP
#define BIFROST_VULKAN_HASH_HPP

#include "bf/bf_gfx_api.h" /*                       */

#include <cstddef> /* size_t                */
#include <cstdint> /* uint64_t              */

struct bfDescriptorSetLayoutInfo_t;
typedef struct bfDescriptorSetLayoutInfo_t bfDescriptorSetLayoutInfo;

namespace bf::vk
{
  std::uint64_t hash(std::uint64_t self, const bfPipelineCache* pipeline);
  std::uint64_t hash(std::uint64_t self, bfTextureHandle* attachments, std::size_t num_attachments);
  std::uint64_t hash(const bfDescriptorSetLayoutInfo& parent, const bfDescriptorSetInfo* desc_set_info);
}  // namespace bf::vk

#endif /* BIFROST_VULKAN_HASH_HPP */
