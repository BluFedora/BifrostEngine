#ifndef BIFROST_VULKAN_PHYSICAL_DEVICE_H
#define BIFROST_VULKAN_PHYSICAL_DEVICE_H

#include "bf/bf_gfx_api.h"

#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif

typedef struct VulkanQueueArray
{
  VkQueueFamilyProperties* queues;
  uint32_t                 size;
  uint32_t                 family_index[BF_GFX_QUEUE_MAX];

} VulkanQueueArray;

typedef struct VulkanSwapchainInfo
{
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR*      formats;
  uint32_t                 num_formats;
  VkPresentModeKHR*        present_modes;
  uint32_t                 num_present_modes;

} VulkanSwapchainInfo;

typedef struct VulkanSwapchainImageList
{
  bfTexture* images;
  uint32_t   size;

} VulkanSwapchainImageList;

typedef struct VulkanSwapchain
{
  VkSwapchainKHR           handle;
  VkSurfaceFormatKHR       format;
  VkPresentModeKHR         present_mode;
  VkExtent2D               extents;
  VulkanSwapchainImageList img_list;
  VkCommandBuffer*         command_buffers;
  VkFence*                 in_flight_fences;
  VkFence*                 in_flight_images;

} VulkanSwapchain;

typedef struct VulkanExtensionList
{
  VkExtensionProperties* extensions;
  uint32_t               size;

} VulkanExtensionList;

typedef struct VulkanPhysicalDevice
{
  bfGfxContextHandle               parent;
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