#ifndef BIFROST_PLATFORM_VULKAN_H
#define BIFROST_PLATFORM_VULKAN_H

#include "bifrost_platform.h"

#if !defined(VULKAN_H_)
#include <stdint.h> /* uint64_t */

#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;

#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined(_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T* object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)

enum VkResult;
#endif

#if __cplusplus
extern "C" {
#endif

 // return non 0 on success
BIFROST_PLATFORM_API int bfWindow_createVulkanSurface(BifrostWindow* self, VkInstance instance, VkSurfaceKHR* out);

#if __cplusplus
}
#endif

#endif  // BIFROST_PLATFORM_VULKAN_H
