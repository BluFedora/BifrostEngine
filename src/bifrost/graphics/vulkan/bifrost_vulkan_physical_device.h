#ifndef BIFROST_VULKAN_PHYSICAL_DEVICE_H
#define BIFROST_VULKAN_PHYSICAL_DEVICE_H

#include <vulkan/vulkan.h>

#if __cplusplus
extern "C" {
#endif
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
  uint32_t                 present_family_index;

} VulkanQueueArray;

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