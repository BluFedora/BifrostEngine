#ifndef BIFROST_VULKAN_PHYSICAL_DEVICE_H
#define BIFROST_VULKAN_PHYSICAL_DEVICE_H

#include "bifrost/graphics/bifrost_gfx_api.h"
#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif

BIFROST_DEFINE_HANDLE(Texture)
{
  // CPU Side Data
  BifrostTextureType image_type;
  int32_t            image_width;
  int32_t            image_height;
  uint32_t           image_depth;
  uint32_t           image_miplevels;
  // GPU Side Data
  VkImage            tex_image;
  VkDeviceMemory     tex_memory;
  VkImageView        tex_view;
  VkSampler          tex_sampler;
  BifrostImageLayout tex_layout;
  BifrostImageFormat tex_format;
  BifrostSampleFlags tex_samples;
};

typedef struct VulkanExtensionList_t
{
  VkExtensionProperties* extensions;
  uint32_t               size;

} VulkanExtensionList;

typedef struct VulkanQueueArray_t
{
  VkQueueFamilyProperties* queues;
  uint32_t                 size;
  uint32_t                 graphics_family_index;
  uint32_t                 compute_family_index;
  uint32_t                 transfer_family_index;
  uint32_t                 present_family_index;

} VulkanQueueArray;

typedef struct VulkanSwapchainInfo_t
{
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR*      formats;
  uint32_t                 num_formats;
  VkPresentModeKHR*        present_modes;
  uint32_t                 num_present_modes;

} VulkanSwapchainInfo;

typedef struct VulkanSwapchainImageList_t
{
  bfTexture* images;
  uint32_t   size;

} VulkanSwapchainImageList;

typedef struct VulkanSwapchain_t
{
  VkSwapchainKHR           handle;
  VkSurfaceFormatKHR       format;
  VkPresentModeKHR         present_mode;
  VkExtent2D               extents;
  VulkanSwapchainImageList img_list;
  VkCommandBuffer*         command_buffers;
  VkFence*                 fences;

} VulkanSwapchain;

typedef struct VulkanPhysicalDevice_t
{
  VkPhysicalDevice                 handle;
  VkPhysicalDeviceMemoryProperties memory_properties;
  VkPhysicalDeviceProperties       device_properties;
  VkPhysicalDeviceFeatures         device_features;
  VulkanQueueArray                 queue_list;
  VulkanExtensionList              extension_list;

} VulkanPhysicalDevice;
#if __cplusplus
}
#endif

#endif /* BIFROST_VULKAN_PHYSICAL_DEVICE_H */