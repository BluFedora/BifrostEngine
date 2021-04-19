#include "bf/bf_gfx_api.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include "bf/platform/bf_platform_vulkan.h"
#include "vulkan/bf_vulkan_conversions.h"
#include "vulkan/bf_vulkan_logical_device.h"
#include "vulkan/bf_vulkan_material_pool.h"
#include "vulkan/bf_vulkan_mem_allocator.h"

#include <algorithm> /* clamp */
#include <cassert>   /* assert   */
#include <cmath>     /* */
#include <cstddef>   /* size_t   */
#include <cstdint>   /* uint32_t */
#include <cstdlib>   /* */
#include <cstring>   /* strcmp   */

#include <vulkan/vulkan.h>

#include <stb/stb_image.h>

#define BIFROST_USE_DEBUG_CALLBACK 1

#if defined(VK_USE_PLATFORM_MACOS_MVK)
#define BIFROST_USE_VALIDATION_LAYERS 0
#else
#define BIFROST_USE_VALIDATION_LAYERS 1
#endif

#define BIFROST_ENGINE_NAME "BF Engine"
#define BIFROST_ENGINE_VERSION 0
#define BIFROST_VULKAN_CUSTOM_ALLOCATOR nullptr

#include "bf/bf_dbg_logger.h"

// Memory Functions (To Be Replaced)

// ReSharper disable CppZeroConstantCanBeReplacedWithNullptr

template<typename T>
T* allocN(std::size_t num_elements)
{
  return static_cast<T*>(std::calloc(num_elements, sizeof(T)));
}

void freeN(void* ptr)
{
  std::free(ptr);
}

// This has very non c++ type semantics in that it doesn't call ctors or dtors of the elements.
template<typename T>
struct FixedArray
{
 private:
  T*          m_Data;
  std::size_t m_Size;

 public:
  FixedArray(std::size_t size) :
    m_Data(allocN<T>(size)),
    m_Size(size)
  {
  }

  FixedArray() :
    m_Data(nullptr),
    m_Size(0u)
  {
  }

  // A resize will not copy over the elements.
  void setSize(std::size_t new_size)
  {
    if (new_size != m_Size)
    {
      freeN(m_Data);
      m_Data = new_size ? allocN<T>(new_size) : nullptr;
      m_Size = new_size;
    }
  }

  T& operator[](std::size_t index)
  {
    assert(index < m_Size);
    return m_Data[index];
  }

  const T& operator[](std::size_t index) const
  {
    assert(index < m_Size);
    return m_Data[index];
  }

  [[nodiscard]] std::size_t size() const
  {
    return m_Size;
  }

  [[nodiscard]] T* begin() const
  {
    return m_Data;
  }

  [[nodiscard]] T* end() const
  {
    return m_Data + m_Size;
  }

  FixedArray(const FixedArray&) = delete;
  FixedArray(FixedArray&&)      = delete;
  FixedArray& operator=(const FixedArray&) = delete;
  FixedArray& operator=(FixedArray&&) = delete;

  ~FixedArray()
  {
    setSize(0);
  }
};

struct GfxContext
{
  const bfGfxContextCreateParams*  params;                // Only valid during initialization
  std::uint32_t                    max_frames_in_flight;  // TODO(Shareef): Make customizable
  VkInstance                       instance;
  FixedArray<VulkanPhysicalDevice> physical_devices;
  VulkanPhysicalDevice*            physical_device;
  bfGfxDeviceHandle                logical_device;
  VkCommandPool                    command_pools[1];  // TODO(Shareef): One per thread.
  bfFrameCount_t                   frame_count;
  bfFrameCount_t                   frame_index;  // frame_count % max_frames_in_flight
  bGfxObjectManager                obj_man;

#if BIFROST_USE_DEBUG_CALLBACK
  VkDebugReportCallbackEXT debug_callback;
#endif
};

namespace
{
  // Context Setup
  void        gfxContextSetupApp(const bfGfxContextCreateParams* params);
  bfBool32    gfxContextCheckLayers(const char* const needed_layers[], size_t num_layers);
  bfBool32    gfxContextSetDebugCallback(PFN_vkDebugReportCallbackEXT callback);
  const char* gfxContextSetupPhysicalDevices();
  void        gfxContextPrintExtensions(void);
  const char* gfxContextSelectPhysicalDevice();
  const char* gfxContextFindSurfacePresent(bfWindowSurface& window);
  const char* gfxContextCreateLogicalDevice();
  const char* gfxContextInitAllocator();
  const char* gfxContextInitCommandPool(uint16_t thread_index);
  const char* gfxContextInitSemaphores(bfWindowSurface& window);
  const char* gfxContextInitSwapchainInfo(bfWindowSurface& window);
  bool        gfxContextInitSwapchain(bfWindowSurface& window);
  void        gfxContextInitSwapchainImageList(bfWindowSurface& window);
  void        gfxContextInitCmdFences(bfWindowSurface& window);
  void        gfxContextInitCmdBuffers(bfWindowSurface& window);
  void        gfxContextDestroyCmdBuffers(VulkanSwapchain* swapchain);
  void        gfxContextDestroyCmdFences(const VulkanSwapchain* swapchain);
  void        gfxContextDestroySwapchainImageList(VulkanSwapchain* swapchain);
  void        gfxContextDestroySwapchain(VulkanSwapchain* swapchain);
  bool        gfxRecreateSwapchain(bfWindowSurface& window);
}  // namespace

void gfxDestroySwapchain(bfWindowSurface& window);

static VKAPI_ATTR VkBool32 VKAPI_CALL gfxContextDbgCallback(
 VkDebugReportFlagsEXT      flags,
 VkDebugReportObjectTypeEXT objType,
 uint64_t                   obj,
 size_t                     location,
 int32_t                    code,
 const char*                layerPrefix,
 const char*                msg,
 void*                      userData)
{
  bfLogError("\n\n\nvalidation layer: %s", msg);
  // __debugbreak();
  assert(!msg);
  return VK_FALSE;
}

#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

// Context
static GfxContext* g_Ctx = nullptr;

void bfGfxInit(const bfGfxContextCreateParams* params)
{
  g_Ctx                       = xxx_Alloc_<GfxContext>();
  g_Ctx->params               = params;
  g_Ctx->max_frames_in_flight = 2;
  g_Ctx->frame_count          = 0;
  g_Ctx->frame_index          = 0;

  bGfxObjectManager_init(&g_Ctx->obj_man);

  gfxContextSetupApp(params);
  if (!gfxContextSetDebugCallback(&gfxContextDbgCallback))
  {
    bfLogError("Failed to set the debug callback.");
  }
  {
    const auto err = gfxContextSetupPhysicalDevices();

    if (err)
    {
      bfLogError("Failed to '%s'.", err);
    }
  }
  {
    gfxContextPrintExtensions();
  }
  {
    gfxContextSelectPhysicalDevice();
    gfxContextCreateLogicalDevice();
    gfxContextInitAllocator();
    gfxContextInitCommandPool(0);
  }

  g_Ctx->params = nullptr;
}

bfGfxDeviceHandle bfGfxGetDevice(void)
{
  return g_Ctx->logical_device;
}

void bfGfxDestroy(void)
{
  auto device = bfGfxGetDevice();

  bfBaseGfxObject* curr = device->cached_resources;

  while (curr)
  {
    bfBaseGfxObject* next = curr->next;
    bfGfxDevice_release_(g_Ctx->logical_device, curr);
    curr = next;
  }

  bfLogPrint("Number of Vulkan Memory Allocs = %u", device->device_memory_allocator.m_NumAllocations);

  VkPoolAllocatorDtor(&device->device_memory_allocator);

  MaterialPool_delete(device->descriptor_pool);

#if BIFROST_USE_DEBUG_CALLBACK
  const PFN_vkDestroyDebugReportCallbackEXT func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Ctx->instance, "vkDestroyDebugReportCallbackEXT");

  if (func)
  {
    func(g_Ctx->instance, g_Ctx->debug_callback, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  }
#endif

  vkDestroyCommandPool(device->handle, g_Ctx->command_pools[0], BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  vkDestroyDevice(device->handle, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  vkDestroyInstance(g_Ctx->instance, BIFROST_VULKAN_CUSTOM_ALLOCATOR);

  xxx_Free(g_Ctx);
}

bfWindowSurfaceHandle bfGfxCreateWindow(struct bfWindow* bf_window)
{
  bfWindowSurfaceHandle surface = xxx_Alloc_<bfWindowSurface>();

  if (bfWindow_createVulkanSurface(bf_window, g_Ctx->instance, &surface->surface))
  {
    surface->current_cmd_list = nullptr;

    gfxContextFindSurfacePresent(*surface);
    gfxContextInitSwapchainInfo(*surface);
    gfxContextInitSemaphores(*surface);
    gfxRecreateSwapchain(*surface);
  }
  else
  {
    delete surface;
    surface = nullptr;
  }

  return surface;
}

void bfGfxDestroyWindow(bfWindowSurfaceHandle window_handle)
{
  window_handle->swapchain_needs_creation = bfFalse;
  window_handle->swapchain_needs_deletion = bfTrue;
  gfxRecreateSwapchain(*window_handle);

  vkDestroySurfaceKHR(g_Ctx->instance, window_handle->surface, BIFROST_VULKAN_CUSTOM_ALLOCATOR);

  for (std::uint32_t i = 0; i < g_Ctx->max_frames_in_flight; ++i)
  {
    vkDestroySemaphore(g_Ctx->logical_device->handle, window_handle->is_image_available[i], BIFROST_VULKAN_CUSTOM_ALLOCATOR);
    vkDestroySemaphore(g_Ctx->logical_device->handle, window_handle->is_render_done[i], BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  }

  freeN(window_handle->is_image_available);  // window_handle->is_render_done shared an allcoation with 'window_handle->is_render_done'.

  delete window_handle;
}

bfBool32 bfGfxBeginFrame(bfWindowSurfaceHandle window)
{
  if (window->swapchain.extents.width <= 0.0f && window->swapchain.extents.height <= 0.0f)
  {
    return bfFalse;
  }

  if (window->swapchain_needs_creation)
  {
    if (!gfxRecreateSwapchain(*window))
    {
      return bfFalse;
    }
  }

  {
    const VkFence command_fence = window->swapchain.in_flight_fences[g_Ctx->frame_index];

    if (vkWaitForFences(g_Ctx->logical_device->handle, 1, &command_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
    {
      return bfFalse;
    }
  }

  const VkResult err = vkAcquireNextImageKHR(g_Ctx->logical_device->handle,
                                             window->swapchain.handle,
                                             UINT64_MAX,
                                             window->is_image_available[g_Ctx->frame_index],
                                             VK_NULL_HANDLE,
                                             &window->image_index);

  if (!(err == VK_SUCCESS || err == VK_TIMEOUT || err == VK_NOT_READY || err == VK_SUBOPTIMAL_KHR))
  {
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
      gfxDestroySwapchain(*window);
    }

    if (!gfxRecreateSwapchain(*window))
    {
      return bfFalse;
    }

    return bfFalse;
  }

  if (window->swapchain.in_flight_images[window->image_index] != VK_NULL_HANDLE)
  {
    if (vkWaitForFences(g_Ctx->logical_device->handle, 1, &window->swapchain.in_flight_images[window->image_index], VK_TRUE, UINT64_MAX) != VK_SUCCESS)
    {
      return bfFalse;
    }
  }

  window->swapchain.in_flight_images[window->image_index] = window->swapchain.in_flight_fences[g_Ctx->frame_index];

  return bfTrue;
}

bfGfxFrameInfo bfGfxGetFrameInfo()
{
  return {g_Ctx->frame_index, g_Ctx->frame_count, g_Ctx->max_frames_in_flight};
}

template<typename T, typename TCache>
static void bfGfxContext_removeFromCache(TCache& cache, bfBaseGfxObject* object)
{
  cache.remove(object->hash_code, reinterpret_cast<T>(object));
}

void bfGfxEndFrame()
{
  // TODO: This whole set of garbage collection should not get called every frame??

  bfBaseGfxObject* prev         = nullptr;
  bfBaseGfxObject* curr         = g_Ctx->logical_device->cached_resources;
  bfBaseGfxObject* release_list = nullptr;

  while (curr)
  {
    bfBaseGfxObject* next = curr->next;

    if ((g_Ctx->frame_count - curr->last_frame_used & bfFrameCountMax) >= 60)
    {
      if (prev)
      {
        prev->next = next;
      }
      else
      {
        g_Ctx->logical_device->cached_resources = next;
      }

      curr->next   = release_list;
      release_list = curr;

      curr = next;

      if (curr)
      {
        next = curr->next;
      }
    }

    prev = curr;
    curr = next;
  }

  while (release_list)
  {
    bfBaseGfxObject* next = release_list->next;

    switch (release_list->type)
    {
      case BF_GFX_OBJECT_RENDERPASS:
      {
        bfGfxContext_removeFromCache<bfRenderpassHandle>(g_Ctx->logical_device->cache_renderpass, release_list);
        break;
      }
      case BF_GFX_OBJECT_PIPELINE:
      {
        bfGfxContext_removeFromCache<bfPipelineHandle>(g_Ctx->logical_device->cache_pipeline, release_list);
        break;
      }
      case BF_GFX_OBJECT_FRAMEBUFFER:
      {
        bfGfxContext_removeFromCache<bfFramebufferHandle>(g_Ctx->logical_device->cache_framebuffer, release_list);
        break;
      }
      case BF_GFX_OBJECT_DESCRIPTOR_SET:
      {
        bfGfxContext_removeFromCache<bfDescriptorSetHandle>(g_Ctx->logical_device->cache_descriptor_set, release_list);
        break;
      }
        bfInvalidDefaultCase();
    }

    bfGfxDevice_release_(g_Ctx->logical_device, release_list);
    release_list = next;
  }

  ++g_Ctx->frame_count;
  g_Ctx->frame_index = g_Ctx->frame_count % g_Ctx->max_frames_in_flight;
}

// Device

void bfGfxDevice_flush(bfGfxDeviceHandle self)
{
  const auto result = vkDeviceWaitIdle(self->handle);
  assert(result == VK_SUCCESS);
  (void)result;
}

bfGfxCommandListHandle bfGfxRequestCommandList(bfWindowSurfaceHandle window, uint32_t thread_index)
{
  assert(thread_index == 0);
  assert(thread_index < bfCArraySize(window->cmd_list_memory));

  if (window->current_cmd_list)
  {
    return window->current_cmd_list;
  }

  bfGfxCommandListHandle list = window->cmd_list_memory + thread_index;

  list->parent         = g_Ctx->logical_device;
  list->handle         = window->swapchain.command_buffers[window->image_index];
  list->fence          = window->swapchain.in_flight_fences[g_Ctx->frame_index];
  list->window         = window;
  list->render_area    = {};
  list->framebuffer    = nullptr;
  list->pipeline       = nullptr;
  list->pipeline_state = {};
  list->has_command    = bfFalse;
  std::memset(list->clear_colors, 0x0, sizeof(list->clear_colors));
  std::memset(&list->pipeline_state, 0x0, sizeof(list->pipeline_state));  // Constent hashing behavior + Memcmp is used for the cache system.

  vkResetCommandBuffer(list->handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

  bfGfxCmdList_setDefaultPipeline(list);

  window->current_cmd_list = list;

  return list;
}

bfTextureHandle bfGfxDevice_requestSurface(bfWindowSurfaceHandle window)
{
  return &window->swapchain.img_list.images[window->image_index];
}

bfDeviceLimits bfGfxDevice_limits(bfGfxDeviceHandle self)
{
  VkPhysicalDeviceLimits& vk_limits = self->parent->device_properties.limits;
  bfDeviceLimits          limits;

  limits.uniform_buffer_offset_alignment = vk_limits.minUniformBufferOffsetAlignment;

  return limits;
}

// Needed by CmdQueue Submit
void gfxDestroySwapchain(bfWindowSurface& window)
{
  if (!window.swapchain_needs_creation)
  {
    window.swapchain_needs_deletion = bfTrue;
    window.swapchain_needs_creation = bfTrue;
  }
}

namespace
{
  bool gfxRecreateSwapchain(bfWindowSurface& window)
  {
    //assert(window.swapchain_needs_creation);

    if (window.swapchain_needs_deletion)
    {
      VulkanSwapchain& old_swapchain = window.swapchain;

      vkWaitForFences(g_Ctx->logical_device->handle, g_Ctx->max_frames_in_flight, old_swapchain.in_flight_fences, VK_TRUE, UINT64_MAX);

      gfxContextDestroyCmdBuffers(&old_swapchain);
      gfxContextDestroyCmdFences(&old_swapchain);
      gfxContextDestroySwapchainImageList(&old_swapchain);
      gfxContextDestroySwapchain(&old_swapchain);

      window.swapchain_needs_deletion = bfFalse;
    }

    if (window.swapchain_needs_creation)
    {
      if (gfxContextInitSwapchain(window))
      {
        gfxContextInitSwapchainImageList(window);
        gfxContextInitCmdFences(window);
        gfxContextInitCmdBuffers(window);

        return true;
      }
    }

    return false;
  }

  void gfxContextSetupApp(const bfGfxContextCreateParams* params)
  {
    static const char* const VALIDATION_LAYER_NAMES[] = {"VK_LAYER_KHRONOS_validation"};
    static const char* const INSTANCE_EXT_NAMES[]     = {
      VK_KHR_SURFACE_EXTENSION_NAME,

#if defined(VK_USE_PLATFORM_WIN32_KHR)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
      VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
      VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#else
#error "Invalid platform selected for the Vulkan Rendering Backend."
#endif

#if BIFROST_USE_DEBUG_CALLBACK
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
    };

#if BIFROST_USE_VALIDATION_LAYERS
    const bfBool32 layers_are_supported = gfxContextCheckLayers(VALIDATION_LAYER_NAMES,
                                                                bfCArraySize(VALIDATION_LAYER_NAMES));

    if (!layers_are_supported)
    {
      assert(!"This device does not support all of the needed validation layers.");
    }
#endif

    VkApplicationInfo app_info;
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = params->app_name;
    app_info.applicationVersion = params->app_version;
    app_info.pEngineName        = BIFROST_ENGINE_NAME;
    app_info.engineVersion      = BIFROST_ENGINE_VERSION;
    app_info.apiVersion         = VK_API_VERSION_1_1;

    VkInstanceCreateInfo init_info;
    init_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    init_info.pNext            = nullptr;
    init_info.flags            = 0;
    init_info.pApplicationInfo = &app_info;
#if BIFROST_USE_VALIDATION_LAYERS
    init_info.enabledLayerCount   = 1;
    init_info.ppEnabledLayerNames = VALIDATION_LAYER_NAMES;
#else
    init_info.enabledLayerCount = 0;
    init_info.ppEnabledLayerNames = NULL;
#endif
    init_info.enabledExtensionCount   = static_cast<uint32_t>(bfCArraySize(INSTANCE_EXT_NAMES));
    init_info.ppEnabledExtensionNames = INSTANCE_EXT_NAMES;

    const VkResult err = vkCreateInstance(&init_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &g_Ctx->instance);

    if (err)
    {
      static const char* const error_messages[] =
       {
        "Unknown Error.",
        "There was not a compatible Vulkan ICD.",
       };

      bfLogError("gfxContextSetupApp(vkCreateInstance %s)", error_messages[err == VK_ERROR_INCOMPATIBLE_DRIVER]);
    }
  }

  bfBool32 gfxContextCheckLayers(const char* const needed_layers[], size_t num_layers)
  {
    bfBool32 ret = bfTrue;

    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    FixedArray<VkLayerProperties> layers{layer_count};
    vkEnumerateInstanceLayerProperties(&layer_count, &layers[0]);

    for (std::size_t i = 0; i < num_layers; ++i)
    {
      const char* const layer_name   = needed_layers[i];
      bfBool32          layer_exists = bfFalse;

      for (const auto& layer : layers)
      {
        if (std::strcmp(layer.layerName, layer_name) == 0)
        {
          layer_exists = bfTrue;
          break;
        }
      }

      if (!layer_exists)
      {
        ret = bfFalse;
        break;
      }
    }

    bfLogPush("Available Layers:");
    bfLogPrint("------------------------------------------------------------------------------------------------");
    for (const auto& layer : layers)
    {
      bfLogPrint("|%-36s|v%u|%-54s|", layer.layerName, (unsigned)layer.implementationVersion, layer.description);
    }
    bfLogPrint("------------------------------------------------------------------------------------------------");
    bfLogPop();

    return ret;
  }

  bfBool32 gfxContextSetDebugCallback(PFN_vkDebugReportCallbackEXT callback)
  {
#if BIFROST_USE_DEBUG_CALLBACK
    VkDebugReportCallbackCreateInfoEXT create_info;
    create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    create_info.pNext       = nullptr;
    create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    create_info.pfnCallback = callback;
    create_info.pUserData   = nullptr;

    PFN_vkCreateDebugReportCallbackEXT func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Ctx->instance, "vkCreateDebugReportCallbackEXT");

    if (func)
    {
      return func(g_Ctx->instance, &create_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &g_Ctx->debug_callback) == VK_SUCCESS;
    }

    return bfFalse;  //VK_ERROR_EXTENSION_NOT_PRESENT;
#else
    return bfTrue;
#endif
  }

  const char* gfxContextSetupPhysicalDevices()
  {
    std::uint32_t num_devices;
    VkResult      err = vkEnumeratePhysicalDevices(g_Ctx->instance, &num_devices, nullptr);

    if (err)
    {
      return "enumerate devices";
    }

    if (num_devices == 0)
    {
      return "find a Vulkan enabled device.";
    }

    FixedArray<VkPhysicalDevice> device_list{num_devices};
    g_Ctx->physical_devices.setSize(num_devices);

    err = vkEnumeratePhysicalDevices(g_Ctx->instance, &num_devices, &device_list[0]);

    if (err)
    {
      return "enumerate devices";
    }

    bfLogPush("Physical Device Listing (%u)", (unsigned)num_devices);
    size_t index = 0;
    for (auto& device : g_Ctx->physical_devices)
    {
      device.handle = device_list[index];

      vkGetPhysicalDeviceMemoryProperties(device.handle, &device.memory_properties);
      vkGetPhysicalDeviceProperties(device.handle, &device.device_properties);
      vkGetPhysicalDeviceFeatures(device.handle, &device.device_features);

      vkGetPhysicalDeviceQueueFamilyProperties(device.handle, &device.queue_list.size, NULL);
      device.queue_list.queues = allocN<VkQueueFamilyProperties>(device.queue_list.size);
      vkGetPhysicalDeviceQueueFamilyProperties(device.handle, &device.queue_list.size, device.queue_list.queues);

      vkEnumerateDeviceExtensionProperties(device.handle, NULL, &device.extension_list.size, NULL);
      device.extension_list.extensions = allocN<VkExtensionProperties>(device.extension_list.size);
      vkEnumerateDeviceExtensionProperties(device.handle, NULL, &device.extension_list.size, device.extension_list.extensions);

      // bfLogPrint("VULKAN_PHYSICAL_DEVICE[%i / %i]:", (i + 1), devices->size);
      bfLogPrint("---- Device Memory Properties ----");
      bfLogPrint("\t Heap Count:        %i", device.memory_properties.memoryHeapCount);

      for (uint32_t j = 0; j < device.memory_properties.memoryHeapCount; ++j)
      {
        const VkMemoryHeap* const memory_heap = &device.memory_properties.memoryHeaps[j];

        bfLogPrint("\t\t HEAP[%i].flags = %i", j, (int)memory_heap->flags);
        bfLogPrint("\t\t HEAP[%i].size  = %u", j, (uint32_t)memory_heap->size);

        if (memory_heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_HEAP_DEVICE_LOCAL_BIT = true;");
        }

        if (memory_heap->flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_HEAP_MULTI_INSTANCE_BIT = true;");
        }
      }

      bfLogPrint("\t Memory Type Count: %i", device.memory_properties.memoryTypeCount);

      for (uint32_t j = 0; j < device.memory_properties.memoryTypeCount; ++j)
      {
        const VkMemoryType* const memory_type = &device.memory_properties.memoryTypes[j];

        bfLogPrint("\t\t MEM_TYPE[%2i].heapIndex     = %u", j, memory_type->heapIndex);
        bfLogPrint("\t\t MEM_TYPE[%2i].propertyFlags = %u", j, memory_type->propertyFlags);

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = true;");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = true;");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = true;");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_PROPERTY_HOST_CACHED_BIT = true;");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT = true;");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
        {
          bfLogPrint("\t\t\t VK_MEMORY_PROPERTY_PROTECTED_BIT = true;");
        }
      }

      bfLogPrint("------- Device  Properties -------");

      bfLogPrint("\t API VERSION: %u.%u.%u",
                 VK_VERSION_MAJOR(device.device_properties.apiVersion),
                 VK_VERSION_MINOR(device.device_properties.apiVersion),
                 VK_VERSION_PATCH(device.device_properties.apiVersion));
      bfLogPrint("\t API VERSION: %u.%u.%u",
                 VK_VERSION_MAJOR(device.device_properties.driverVersion),
                 VK_VERSION_MINOR(device.device_properties.driverVersion),
                 VK_VERSION_PATCH(device.device_properties.driverVersion));
      bfLogPrint("\t DRIVER VERSION: %u", device.device_properties.driverVersion);
      bfLogPrint("\t Device ID: %u", device.device_properties.deviceID);
      bfLogPrint("\t Vendor ID: %u", device.device_properties.vendorID);

      switch (device.device_properties.deviceType)
      {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        {
          bfLogPrint("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_OTHER");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        {
          bfLogPrint("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        {
          bfLogPrint("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        {
          bfLogPrint("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
        {
          bfLogPrint("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_CPU");
          break;
        }
        default:
        {
          bfLogPrint("\t DEVICE_TYPE = DEVICE_TYPE_UNKNOWN");
          break;
        }
      }

      bfLogPrint("\t DEVICE_NAME: \"%s\"", device.device_properties.deviceName);

      bfLogPrint("\t PIPELINE_CACHE_UUID:");
      for (uint32_t j = 0; j < VK_UUID_SIZE; ++j)
      {
        bfLogPrint("\t\t [%u] = %i", j, (int)device.device_properties.pipelineCacheUUID[j]);
      }

      // TODO: Device Limits    (VkPhysicalDeviceLimits)(limits)
      // TODO: sparseProperties (VkPhysicalDeviceSparseProperties)(sparseProperties)

      ++index;
    }
    bfLogPop();

    return nullptr;
  }

  void gfxContextPrintExtensions()
  {
    uint32_t num_extensions;
    vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, NULL);

    if (num_extensions)
    {
      VkExtensionProperties* const extension_list = allocN<VkExtensionProperties>(num_extensions);

      vkEnumerateInstanceExtensionProperties(NULL, &num_extensions, extension_list);

      bfLogPrint("VULKAN EXTENSIONS:");
      for (uint32_t i = 0; i < num_extensions; ++i)
      {
        const VkExtensionProperties* extension = &extension_list[i];

        bfLogPrint("\t EXT: { Name: %-50s Version: %-3u }", extension->extensionName, extension->specVersion);
      }

      freeN(extension_list);
    }
  }

  const char* gfxContextSelectPhysicalDevice()
  {
    if (g_Ctx->physical_devices.size() == 0)
    {
      return "Found no Physical devices";
    }

    // TODO(SR): Select device based on "device_features", "device_properties", "deviceType" etc
    g_Ctx->physical_device = &g_Ctx->physical_devices[0];

    return nullptr;
  }

#ifndef CUSTOM_ALLOCATOR
#define CUSTOM_ALLOCATOR nullptr
#endif

#if 0
  const char* gfxContextInitSurface()
  {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR create_info;
    create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext     = NULL;
    create_info.flags     = 0;
    create_info.hinstance = (HINSTANCE)self->params->platform_module;
    create_info.hwnd      = (HWND)self->params->platform_window;

    const VkResult err = vkCreateWin32SurfaceKHR(self->instance, &create_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->main_window.surface);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VkXlibSurfaceCreateInfoKHR create_info = {
     .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
     .pNext = NULL,
     .flags = 0,
     .dpy = self->params->platform_module,
     .window = self->params->platform_window};

    VkResult err = vkCreateXlibSurfaceKHR(self->instance, &create_info, CUSTOM_ALLOCATOR, &self->main_window.surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    // VkMacOSSurfaceCreateInfoMVK create_info;
    // create_info.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    // create_info.pNext = NULL;
    // create_info.flags = 0x0;
    // create_info.pView = (void*)self->params->platform_window;

    // VkResult err = vkCreateMacOSSurfaceMVK(self->instance, &create_info, CUSTOM_ALLOCATOR, &self->main_window.surface);

    VkResult err = glfwCreateWindowSurface(self->instance, (GLFWwindow*)self->params->platform_window, CUSTOM_ALLOCATOR, &self->main_window.surface);
#else  // TODO: Other Platforms
#error "unsupported platform for Vulkan"
#endif

    return err ? "Failed to create Surface" : nullptr;
  }
#endif

  uint32_t findQueueBasic(const VulkanPhysicalDevice* device, uint32_t queue_size, VkQueueFlags flags);

  const char* gfxContextFindSurfacePresent(bfWindowSurface& window)
  {
    VulkanPhysicalDevice* device           = g_Ctx->physical_device;
    const uint32_t        queue_size       = device->queue_list.size;
    VkBool32*             supports_present = allocN<VkBool32>(queue_size);

    for (uint32_t i = 0; i < queue_size; ++i)
    {
      const VkResult error = vkGetPhysicalDeviceSurfaceSupportKHR(device->handle, i, window.surface, &supports_present[i]);

      if (error)
      {
        bfLogPrint("GfxContext_initQueuesAndSurface vkGetPhysicalDeviceSurfaceSupportKHR[%i] -> [%i]", i, (int)error);
      }
    }

    device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS] = UINT32_MAX;
    device->queue_list.family_index[BF_GFX_QUEUE_COMPUTE]  = UINT32_MAX;
    device->queue_list.family_index[BF_GFX_QUEUE_TRANSFER] = UINT32_MAX;
    device->queue_list.family_index[BF_GFX_QUEUE_PRESENT]  = UINT32_MAX;

    for (uint32_t i = 0; i < queue_size; ++i)
    {
      const VkQueueFamilyProperties* const queue = &device->queue_list.queues[i];

      if (queue->queueCount && queue->queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        if (device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS] == UINT32_MAX)
        {
          device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS] = i;
        }

        if (supports_present[i])
        {
          device->queue_list.family_index[BF_GFX_QUEUE_PRESENT] = device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS] = i;
          break;
        }
      }
    }

    device->queue_list.family_index[BF_GFX_QUEUE_COMPUTE]  = findQueueBasic(device, queue_size, VK_QUEUE_COMPUTE_BIT);
    device->queue_list.family_index[BF_GFX_QUEUE_TRANSFER] = findQueueBasic(device, queue_size, VK_QUEUE_TRANSFER_BIT);

    if (device->queue_list.family_index[BF_GFX_QUEUE_PRESENT] == UINT32_MAX)
    {
      // If didn't find a queue that supports both graphics and present, then find a separate present queue.
      for (uint32_t i = 0; i < queue_size; ++i)
      {
        if (supports_present[i])
        {
          device->queue_list.family_index[BF_GFX_QUEUE_PRESENT] = i;
          break;
        }
      }
    }

    freeN(supports_present);

    for (uint32_t i = 0; i < bfCArraySize(device->queue_list.family_index); ++i)
    {
      if (device->queue_list.family_index[i] == UINT32_MAX)
      {
        return "Could not find Queues for Present / Graphics / Compute / Transfer.";
      }
    }

    window.swapchain_needs_creation = true;
    return nullptr;
  }

  VkDeviceQueueCreateInfo makeBasicQCreateInfo(uint32_t queue_index, uint32_t num_queues, const float* queue_priorities)
  {
    VkDeviceQueueCreateInfo create_info;

    create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create_info.pNext            = NULL;
    create_info.flags            = 0x0;
    create_info.queueFamilyIndex = queue_index;
    create_info.queueCount       = num_queues;
    create_info.pQueuePriorities = queue_priorities;

    return create_info;
  }

  const char* gfxContextCreateLogicalDevice()
  {
    static const char* const DEVICE_EXT_NAMES[] =
     {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
     };  // I should be checking if the extensions are supported.

    static const float queue_priorities[] = {0.0f};

    VulkanPhysicalDevice*   device        = g_Ctx->physical_device;
    const uint32_t          gfx_queue_idx = device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS];
    uint32_t                num_queues    = 0;
    VkDeviceQueueCreateInfo queue_create_infos[BF_GFX_QUEUE_MAX];

    const auto addQueue = [gfx_queue_idx, &num_queues, &queue_create_infos](uint32_t queue_index) {
      if (gfx_queue_idx != queue_index)
      {
        queue_create_infos[num_queues++] = makeBasicQCreateInfo(queue_index, 1, queue_priorities);
      }
    };

    queue_create_infos[num_queues++] = makeBasicQCreateInfo(gfx_queue_idx, 1, queue_priorities);
    addQueue(device->queue_list.family_index[BF_GFX_QUEUE_COMPUTE]);
    addQueue(device->queue_list.family_index[BF_GFX_QUEUE_TRANSFER]);
    addQueue(device->queue_list.family_index[BF_GFX_QUEUE_PRESENT]);

    VkPhysicalDeviceFeatures deviceFeatures;  // I should be checking if the features exist for the device in the first place.
    memset(&deviceFeatures, 0x0, sizeof(deviceFeatures));
    //deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo device_info;
    device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext                   = NULL;
    device_info.flags                   = 0;
    device_info.queueCreateInfoCount    = num_queues;
    device_info.pQueueCreateInfos       = queue_create_infos;
    device_info.enabledLayerCount       = 0;
    device_info.ppEnabledLayerNames     = NULL;
    device_info.enabledExtensionCount   = uint32_t(bfCArraySize(DEVICE_EXT_NAMES));
    device_info.ppEnabledExtensionNames = DEVICE_EXT_NAMES;
    device_info.pEnabledFeatures        = &deviceFeatures;

    g_Ctx->logical_device         = xxx_Alloc_<bfGfxDevice>();
    g_Ctx->logical_device->parent = device;

    const VkResult err = vkCreateDevice(device->handle, &device_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &g_Ctx->logical_device->handle);

    if (err)
    {
      delete g_Ctx->logical_device;
      g_Ctx->logical_device = nullptr;
      return "Failed to create device";
    }

    MaterialPoolCreateParams create_material_pool;
    create_material_pool.logical_device        = g_Ctx->logical_device;
    create_material_pool.num_textures_per_link = 32;
    create_material_pool.num_uniforms_per_link = 16;
    create_material_pool.num_descsets_per_link = 8;

    g_Ctx->logical_device->descriptor_pool  = MaterialPool_new(&create_material_pool);
    g_Ctx->logical_device->cached_resources = nullptr;

    for (uint32_t i = 0; i < bfCArraySize(device->queue_list.family_index); ++i)
    {
      // The 0 means grab the first queue of the specified family.
      // the number must be less than "VkDeviceQueueCreateInfo::queueCount"
      vkGetDeviceQueue(g_Ctx->logical_device->handle, device->queue_list.family_index[i], 0, &g_Ctx->logical_device->queues[i]);
    }

    return nullptr;
  }

  const char* gfxContextInitAllocator()
  {
    VkPoolAllocatorCtor(&g_Ctx->logical_device->device_memory_allocator, g_Ctx->logical_device);
    return nullptr;
  }

  const char* gfxContextInitCommandPool(uint16_t thread_index)
  {
    assert(thread_index == 0 && "Current implemetation only supports one thread.");

    const VulkanPhysicalDevice* const phys_device    = g_Ctx->physical_device;
    const bfGfxDeviceHandle           logical_device = g_Ctx->logical_device;

    // This should be per thread.
    VkCommandPoolCreateInfo cmd_pool_info;
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |            // Since these are short lived buffers
                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // Since I Reuse the Buffer each frame
    cmd_pool_info.queueFamilyIndex = phys_device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS];

    const VkResult error = vkCreateCommandPool(
     logical_device->handle,
     &cmd_pool_info,
     BIFROST_VULKAN_CUSTOM_ALLOCATOR,
     &g_Ctx->command_pools[thread_index]);

    return error ? "Failed to create command pool" : nullptr;
  }

  const char* gfxContextInitSemaphores(bfWindowSurface& window)
  {
    VkSemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = NULL;
    semaphore_create_info.flags = 0;

    window.is_image_available = allocN<VkSemaphore>(std::size_t(g_Ctx->max_frames_in_flight) * 2);
    window.is_render_done     = window.is_image_available + g_Ctx->max_frames_in_flight;

    for (std::uint32_t i = 0; i < g_Ctx->max_frames_in_flight; ++i)
    {
      {
        const VkResult err = vkCreateSemaphore(
         g_Ctx->logical_device->handle,
         &semaphore_create_info,
         BIFROST_VULKAN_CUSTOM_ALLOCATOR,
         window.is_image_available + i);

        if (err)
        {
          return "Failed to create a Semaphore";
        }
      }

      {
        const VkResult err = vkCreateSemaphore(
         g_Ctx->logical_device->handle,
         &semaphore_create_info,
         BIFROST_VULKAN_CUSTOM_ALLOCATOR,
         window.is_render_done + i);

        if (err)
        {
          return "Failed to create a Semaphore";
        }
      }
    }

    return nullptr;
  }

  VkSurfaceFormatKHR gfxContextFindSurfaceFormat(const VkSurfaceFormatKHR* const formats, const uint32_t num_formats);
  VkPresentModeKHR   gfxFindSurfacePresentMode(const VkPresentModeKHR* const present_modes,
                                               const uint32_t                num_present_modes);

  const char* gfxContextInitSwapchainInfo(bfWindowSurface& window)
  {
    VulkanPhysicalDevice* const device = g_Ctx->physical_device;
    VulkanSwapchainInfo* const  info   = &window.swapchain_info;

    // NOTE(Shareef)
    //    Not needed since when the SwapChain is Recreated this information needs to be queried again regardless
    // vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->handle,  context->surface, &swapchain_info->capabilities);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->handle, window.surface, &info->num_formats, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->handle, window.surface, &info->num_present_modes, nullptr);

    info->formats       = allocN<VkSurfaceFormatKHR>(info->num_formats);
    info->present_modes = allocN<VkPresentModeKHR>(info->num_present_modes);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device->handle, window.surface, &info->num_formats, info->formats);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->handle, window.surface, &info->num_present_modes, info->present_modes);

    window.swapchain.format = gfxContextFindSurfaceFormat(info->formats, info->num_formats);

    return nullptr;
  }

  VkExtent2D gfxFindSurfaceExtents(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

  bool gfxContextInitSwapchain(bfWindowSurface& window)
  {
    auto& swapchain_info = window.swapchain_info;

    VulkanPhysicalDevice* const physical_device = g_Ctx->physical_device;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->handle, window.surface, &swapchain_info.capabilities);

    const VkPresentModeKHR surface_present_mode = gfxFindSurfacePresentMode(swapchain_info.present_modes, swapchain_info.num_present_modes);
    const VkExtent2D       surface_extents      = gfxFindSurfaceExtents(swapchain_info.capabilities, 0, 0);

    uint32_t                      image_count   = swapchain_info.capabilities.minImageCount + 1;
    VkSurfaceTransformFlagBitsKHR pre_transform = swapchain_info.capabilities.currentTransform;

    if (surface_extents.width == 0 || surface_extents.height == 0)
    {
      window.swapchain_needs_creation = true;
      return false;
    }

    // A value of 0 for maxImageCount means that there is no limit besides memory requirements
    if (swapchain_info.capabilities.maxImageCount > 0 && image_count > swapchain_info.capabilities.maxImageCount)
    {
      image_count = swapchain_info.capabilities.maxImageCount;
    }

    // We can rotate, flip , etc if that transform type is supported
    // On Windows / Or Just My Machine This is the only transform supported
    if (swapchain_info.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
      pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }

    VkCompositeAlphaFlagBitsKHR composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR composite_alpha_flags[4] =
     {
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
     };

    for (const auto& composite_alpha_flag : composite_alpha_flags)
    {
      if (swapchain_info.capabilities.supportedCompositeAlpha & composite_alpha_flag)
      {
        composite_alpha = composite_alpha_flag;
        break;
      }
    }

    VkSwapchainCreateInfoKHR swapchain_ci;

    std::memset(&swapchain_ci, 0x0, sizeof(swapchain_ci));

    swapchain_ci.sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.flags              = 0x0;
    swapchain_ci.pNext              = nullptr;
    swapchain_ci.surface            = window.surface;
    swapchain_ci.minImageCount      = image_count;
    swapchain_ci.imageFormat        = window.swapchain.format.format;
    swapchain_ci.imageExtent.width  = surface_extents.width;
    swapchain_ci.imageExtent.height = surface_extents.height;
    swapchain_ci.preTransform       = pre_transform;
    swapchain_ci.compositeAlpha     = composite_alpha;
    swapchain_ci.imageArrayLayers   = 1;
    swapchain_ci.presentMode        = surface_present_mode;
    swapchain_ci.oldSwapchain       = VK_NULL_HANDLE;
    swapchain_ci.clipped            = VK_TRUE;  // If another window covers this on then do you not render those pixel
    swapchain_ci.imageColorSpace    = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_ci.imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_ci.imageSharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    // Assumes "physical_device->queue_list.graphics_family_index == physical_device->queue_list.present_family_index"
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices   = nullptr;

    const uint32_t queue_family_indices[2] =
     {
      physical_device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS],
      physical_device->queue_list.family_index[BF_GFX_QUEUE_PRESENT],
     };

    if (physical_device->queue_list.family_index[BF_GFX_QUEUE_GRAPHICS] != physical_device->queue_list.family_index[BF_GFX_QUEUE_PRESENT])
    {
      // If the graphics and present queues are from different queue families,
      // we either have to explicitly transfer ownership of images between
      // the queues, or we have to create the swapchain with imageSharingMode
      // as VK_SHARING_MODE_CONCURRENT
      swapchain_ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swapchain_ci.queueFamilyIndexCount = 2;
      swapchain_ci.pQueueFamilyIndices   = queue_family_indices;
    }

    const VkResult err = vkCreateSwapchainKHR(g_Ctx->logical_device->handle, &swapchain_ci, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &window.swapchain.handle);

    window.swapchain.extents        = surface_extents;
    window.swapchain.present_mode   = surface_present_mode;
    window.swapchain_needs_creation = false;

    if (err)
    {
      // std::printf("GfxContext_initSwapchain %s %s", "vkCreateSwapchainKHR", "Failed to Create Swapchain");
    }

    return true;
  }

  void gfxContextInitSwapchainImageList(bfWindowSurface& window)
  {
    bfGfxDeviceHandle      logical_device = g_Ctx->logical_device;
    VulkanSwapchain* const swapchain      = &window.swapchain;

    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &swapchain->img_list.size, NULL);

    FixedArray<VkImage> temp_images{swapchain->img_list.size};
    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &swapchain->img_list.size, &temp_images[0]);

    swapchain->img_list.images = allocN<bfTexture>(swapchain->img_list.size);

    for (uint32_t i = 0; i < swapchain->img_list.size; ++i)
    {
      bfTexture* const image = swapchain->img_list.images + i;

      bfBaseGfxObject_ctor(&image->super, BF_GFX_OBJECT_TEXTURE, &g_Ctx->obj_man);

      image->image_type      = BF_TEX_TYPE_2D;
      image->image_width     = int32_t(swapchain->extents.width);
      image->image_height    = int32_t(swapchain->extents.height);
      image->image_depth     = 1;
      image->image_miplevels = 1;
      image->tex_memory      = VK_NULL_HANDLE;
      image->tex_view        = bfCreateImageView2D(logical_device->handle, temp_images[i], swapchain->format.format, VK_IMAGE_ASPECT_COLOR_BIT, image->image_miplevels);
      image->tex_sampler     = VK_NULL_HANDLE;
      image->tex_layout      = BF_IMAGE_LAYOUT_UNDEFINED;
      image->tex_format      = swapchain->format.format;
      image->tex_samples     = BF_SAMPLE_1;
    }
  }

  void gfxContextInitCmdFences(bfWindowSurface& window)
  {
    bfGfxDeviceHandle logical_device = g_Ctx->logical_device;

    const std::uint32_t num_in_flight_fences = g_Ctx->max_frames_in_flight;
    const std::uint32_t num_in_images_fences = window.swapchain.img_list.size;
    window.swapchain.in_flight_fences        = allocN<VkFence>(num_in_flight_fences + num_in_images_fences);
    window.swapchain.in_flight_images        = window.swapchain.in_flight_fences + num_in_flight_fences;

    for (uint32_t i = 0; i < num_in_flight_fences; ++i)
    {
      VkFenceCreateInfo fence_create_info;
      fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_create_info.pNext = NULL;
      fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      vkCreateFence(logical_device->handle, &fence_create_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, window.swapchain.in_flight_fences + i);
    }

    for (uint32_t i = 0; i < num_in_images_fences; ++i)
    {
      window.swapchain.in_flight_images[i] = VK_NULL_HANDLE;
    }
  }

  void gfxContextCreateCommandBuffers(std::uint32_t num_buffers, VkCommandBuffer* result)
  {
    const bfGfxDeviceHandle device = g_Ctx->logical_device;

    VkCommandBufferAllocateInfo cmd_alloc_info;
    cmd_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext              = nullptr;
    cmd_alloc_info.commandPool        = g_Ctx->command_pools[0];  // TODO: Threaded pool...
    cmd_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = num_buffers;

    const VkResult error = vkAllocateCommandBuffers(device->handle, &cmd_alloc_info, result);
    assert(error == VK_SUCCESS);
  }

  struct TempCommandBuffer final
  {
    VkCommandBuffer handle;
  };

  VkCommandBuffer* gfxContextCreateCommandBuffers(std::uint32_t num_buffers)
  {
    VkCommandBuffer* result = allocN<VkCommandBuffer>(num_buffers);
    gfxContextCreateCommandBuffers(num_buffers, result);

    return result;
  }

  void gfxContextDestroyCommandBuffers(std::uint32_t num_buffers, VkCommandBuffer* buffers, bool free_array = true)
  {
    if (buffers)
    {
      const bfGfxDeviceHandle logical_device = g_Ctx->logical_device;

      vkFreeCommandBuffers(logical_device->handle, g_Ctx->command_pools[0], num_buffers, buffers);

      if (free_array)
      {
        freeN(buffers);
      }
    }
  }

  TempCommandBuffer gfxContextBeginTransientCommandBuffer()
  {
    VkCommandBuffer result;
    gfxContextCreateCommandBuffers(1, &result);

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext            = nullptr;
    begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(result, &begin_info);

    return {result};
  }

  void gfxContextEndTransientCommandBuffer(TempCommandBuffer buffer, bfGfxQueueType queue_type = BF_GFX_QUEUE_GRAPHICS, bool wait_for_finish = true)
  {
    const VkQueue queue = g_Ctx->logical_device->queues[queue_type];

    vkEndCommandBuffer(buffer.handle);

    VkSubmitInfo submit_info;
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = nullptr;
    submit_info.waitSemaphoreCount   = 0;
    submit_info.pWaitSemaphores      = nullptr;
    submit_info.pWaitDstStageMask    = nullptr;
    submit_info.commandBufferCount   = 1;
    submit_info.pCommandBuffers      = &buffer.handle;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores    = nullptr;

    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);

    if (wait_for_finish)
    {
      vkQueueWaitIdle(queue);
      gfxContextDestroyCommandBuffers(1, &buffer.handle, false);
    }  // else the caller is responsible for freeing
  }

  void gfxContextInitCmdBuffers(bfWindowSurface& window)
  {
    window.swapchain.command_buffers = gfxContextCreateCommandBuffers(window.swapchain.img_list.size);
  }

  void gfxContextDestroyCmdBuffers(VulkanSwapchain* swapchain)
  {
    gfxContextDestroyCommandBuffers(swapchain->img_list.size, swapchain->command_buffers);
    swapchain->command_buffers = nullptr;
  }

  void gfxContextDestroyCmdFences(const VulkanSwapchain* swapchain)
  {
    const bfGfxDeviceHandle logical_device = g_Ctx->logical_device;
    const VkDevice          device         = logical_device->handle;

    for (uint32_t i = 0; i < g_Ctx->max_frames_in_flight; ++i)
    {
      vkDestroyFence(device, swapchain->in_flight_fences[i], BIFROST_VULKAN_CUSTOM_ALLOCATOR);
    }

    freeN(swapchain->in_flight_fences);
    // freeN(swapchain->in_flight_images); Shared allocation w/ 'swapchain->in_flight_fences'.
    //const_cast<VulkanSwapchain*>(swapchain)->in_flight_fences = nullptr;
    //const_cast<VulkanSwapchain*>(swapchain)->in_flight_images = nullptr;
  }

  void gfxContextDestroySwapchainImageList(VulkanSwapchain* swapchain)
  {
    bfGfxDeviceHandle               logical_device = g_Ctx->logical_device;
    const VkDevice                  device         = logical_device->handle;
    VulkanSwapchainImageList* const image_list     = &swapchain->img_list;

    for (uint32_t i = 0; i < image_list->size; ++i)
    {
      const bfTextureHandle image = image_list->images + i;

      logical_device->cache_framebuffer.forEach([image](bfFramebufferHandle fb, bfFramebufferState& config_data) {
        (void)fb;

        for (uint32_t attachment_index = 0; attachment_index < config_data.num_attachments; ++attachment_index)
        {
          if (config_data.attachments[attachment_index] == image)
          {
            config_data.attachments[attachment_index] = nullptr;
          }
        }
      });

      vkDestroyImageView(device, image->tex_view, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
    }

    freeN(image_list->images);
    image_list->images = nullptr;
  }

  void gfxContextDestroySwapchain(VulkanSwapchain* swapchain)
  {
    if (swapchain->handle)
    {
      bfGfxDeviceHandle logical_device = g_Ctx->logical_device;

      vkDestroySwapchainKHR(logical_device->handle, swapchain->handle, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
      swapchain->handle = VK_NULL_HANDLE;
    }
  }

  // Helper's Helpers

  uint32_t findQueueBasic(const VulkanPhysicalDevice* device, uint32_t queue_size, VkQueueFlags flags)
  {
    uint32_t ret = UINT32_MAX;

    for (uint32_t i = 0; i < queue_size; ++i)
    {
      const VkQueueFamilyProperties* const queue = &device->queue_list.queues[i];

      if (queue->queueCount && queue->queueFlags & flags)
      {
        if (ret == UINT32_MAX)
        {
          ret = i;
          break;
        }
      }
    }

    return ret;
  }

  VkSurfaceFormatKHR gfxContextFindSurfaceFormat(const VkSurfaceFormatKHR* const formats, const uint32_t num_formats)
  {
    assert(num_formats >= 1);

    if (num_formats == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
      // C Style: return (VkSurfaceFormatKHR){VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
      return VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    const VkSurfaceFormatKHR* const formats_end = formats + num_formats;
    const VkSurfaceFormatKHR*       format      = formats;

    while (format != formats_end)
    {
      if (format->format == VK_FORMAT_B8G8R8A8_UNORM && format->colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        return *format;
      }

      ++format;
    }

    return formats[0];
  }

  VkPresentModeKHR gfxFindSurfacePresentMode(const VkPresentModeKHR* const present_modes,
                                             const uint32_t                num_present_modes)
  {
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;  // Guaranteed to exist according to the standard.

#if !defined(BIFROST_GRAPHICS_POWER_SAVER)
    const VkPresentModeKHR* const present_mode_end = present_modes + num_present_modes;
    const VkPresentModeKHR*       present_mode     = present_modes;

    while (present_mode != present_mode_end)
    {
      if (*present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
      {
        return *present_mode;
      }

      if (*present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        best_mode = *present_mode;
      }

      ++present_mode;
    }
#else
    (void)present_modes;
    (void)num_present_modes;
#endif

    return best_mode;
  }

  VkExtent2D gfxFindSurfaceExtents(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
  {
    if (capabilities.currentExtent.width != UINT32_MAX && capabilities.currentExtent.height != UINT32_MAX)
    {
      return capabilities.currentExtent;
    }

    return {
     std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
     std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };
  }

}  // namespace

extern "C" VkBool32 memory_type_from_properties(VkPhysicalDeviceMemoryProperties mem_props, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex)
{
  // Search memtypes to find first index with those properties
  for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++)
  {
    // Check that the first bit is set
    if ((typeBits & 1) == 1)
    {
      // Type is available, does it match user properties?
      if ((mem_props.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask)
      {
        *typeIndex = i;
        return VK_TRUE;
      }
    }
    // Move the checked bit over
    typeBits >>= 1;
  }

  return VK_FALSE;
}

/* Buffers */
bfBufferHandle bfGfxDevice_newBuffer(bfGfxDeviceHandle self_, const bfBufferCreateParams* params)
{
  bfBufferHandle self = xxx_AllocGfxObject<bfBuffer>(BF_GFX_OBJECT_BUFFER, &g_Ctx->obj_man);

  self->alloc_pool            = &self_->device_memory_allocator;
  self->alloc_info.mapped_ptr = NULL;
  self->real_size             = params->allocation.size;
  self->usage                 = params->usage;

  VkBufferCreateInfo buffer_info;
  buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.pNext       = NULL;
  buffer_info.flags       = 0;
  buffer_info.size        = params->allocation.size;
  buffer_info.usage       = bfVkConvertBufferUsageFlags(params->usage);
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // Used if: VK_SHARING_MODE_CONCURRENT
  // Note: VK_SHARING_MODE_CONCURRENT may be slower
  buffer_info.queueFamilyIndexCount = 0;
  buffer_info.pQueueFamilyIndices   = NULL;

  const VkResult error = vkCreateBuffer(self_->handle, &buffer_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->handle);
  assert(error == VK_SUCCESS);
  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(self_->handle, self->handle, &mem_requirements);

  bfAllocationCreateInfo buffer_create_info = params->allocation;
  buffer_create_info.size                   = mem_requirements.size;

  uint32_t memory_type_index;

  memory_type_from_properties(
   self_->parent->memory_properties,
   mem_requirements.memoryTypeBits,
   bfVkConvertBufferPropertyFlags(params->allocation.properties),
   &memory_type_index);

  VkPoolAllocator_alloc(self->alloc_pool,
                        &buffer_create_info,
                        params->usage & BF_BUFFER_USAGE_PERSISTENTLY_MAPPED_BUFFER,
                        memory_type_index,
                        &self->alloc_info);
  vkBindBufferMemory(self_->handle, self->handle, self->alloc_info.handle, self->alloc_info.offset);

  return self;
}

bfBufferSize bfBuffer_size(bfBufferHandle self)
{
  return self->real_size;
}

bfBufferSize bfBuffer_offset(bfBufferHandle self)
{
  return self->alloc_info.offset;
}

void* bfBuffer_mappedPtr(bfBufferHandle self)
{
  return self->alloc_info.mapped_ptr;
}

void* bfBuffer_map(bfBufferHandle self, bfBufferSize offset, bfBufferSize size)
{
  if ((self->usage & BF_BUFFER_USAGE_PERSISTENTLY_MAPPED_BUFFER) != 0)
  {
    return (char*)self->alloc_info.mapped_ptr + offset;
  }
  else
  {
    assert(self->alloc_info.mapped_ptr == NULL && "Buffer_map attempt to map an already mapped buffer.");

    if (self->alloc_info.mapped_ptr == NULL)
    {
      vkMapMemory(self->alloc_pool->m_LogicalDevice->handle, self->alloc_info.handle, offset, size, 0, &self->alloc_info.mapped_ptr);
    }
  }

  return self->alloc_info.mapped_ptr;
}

// TODO(SR): This allocation happens too frequently to be so recklesssssss.
VkMappedMemoryRange* bfBuffer__makeRangesN(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges)
{
  VkMappedMemoryRange* const ranges = allocN<VkMappedMemoryRange>(num_ranges);

  for (uint32_t i = 0; i < num_ranges; ++i)
  {
    ranges[i].sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    ranges[i].pNext  = nullptr;
    ranges[i].memory = self->alloc_info.handle;
    ranges[i].offset = self->alloc_info.offset + offsets[i];
    ranges[i].size   = sizes[i];
  }

  return ranges;
}

void bfBuffer_invalidateRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges)
{
  VkMappedMemoryRange* const ranges = bfBuffer__makeRangesN(self, offsets, sizes, num_ranges);
  vkInvalidateMappedMemoryRanges(self->alloc_pool->m_LogicalDevice->handle, num_ranges, ranges);
  freeN(ranges);
}

void bfBuffer_copyCPU(bfBufferHandle self, bfBufferSize dst_offset, const void* data, bfBufferSize num_bytes)
{
  memcpy(static_cast<unsigned char*>(self->alloc_info.mapped_ptr) + dst_offset, data, (size_t)num_bytes);
}

void bfBuffer_copyGPU(bfBufferHandle src, bfBufferSize src_offset, bfBufferHandle dst, bfBufferSize dst_offset, bfBufferSize num_bytes)
{
  const auto transient_cmd = gfxContextBeginTransientCommandBuffer();
  {
    VkBufferCopy copy_region;
    copy_region.srcOffset = src_offset + src->alloc_info.offset;
    copy_region.dstOffset = dst_offset + dst->alloc_info.offset;
    copy_region.size      = num_bytes;

    vkCmdCopyBuffer(transient_cmd.handle, src->handle, dst->handle, 1, &copy_region);
  }
  gfxContextEndTransientCommandBuffer(transient_cmd);
}

void bfBuffer_flushRanges(bfBufferHandle self, const bfBufferSize* offsets, const bfBufferSize* sizes, uint32_t num_ranges)
{
  VkMappedMemoryRange* const ranges = bfBuffer__makeRangesN(self, offsets, sizes, num_ranges);

  vkFlushMappedMemoryRanges(self->alloc_pool->m_LogicalDevice->handle, num_ranges, ranges);

  freeN(ranges);
}

void bfBuffer_unMap(bfBufferHandle self)
{
  if (!(self->usage & BF_BUFFER_USAGE_PERSISTENTLY_MAPPED_BUFFER))
  {
    vkUnmapMemory(self->alloc_pool->m_LogicalDevice->handle, self->alloc_info.handle);
    self->alloc_info.mapped_ptr = nullptr;
  }
}

/* Shader Program + Module */
bfShaderModuleHandle bfGfxDevice_newShaderModule(bfGfxDeviceHandle self_, bfShaderType type)
{
  bfShaderModuleHandle self = xxx_AllocGfxObject<bfShaderModule>(BF_GFX_OBJECT_SHADER_MODULE, &g_Ctx->obj_man);

  self->parent         = self_;
  self->type           = type;
  self->handle         = VK_NULL_HANDLE;
  self->entry_point[0] = '\0';

  return self;
}

bfShaderProgramHandle bfGfxDevice_newShaderProgram(bfGfxDeviceHandle self_, const bfShaderProgramCreateParams* params)
{
  bfShaderProgramHandle self = xxx_AllocGfxObject<bfShaderProgram>(BF_GFX_OBJECT_SHADER_PROGRAM, &g_Ctx->obj_man);

  assert(params->num_desc_sets <= k_bfGfxDescriptorSets);

  self->parent               = self_;
  self->layout               = VK_NULL_HANDLE;
  self->num_desc_set_layouts = params->num_desc_sets;
  self->modules.size         = 0;

  for (uint32_t i = 0; i < self->num_desc_set_layouts; ++i)
  {
    self->desc_set_layouts[i]                          = VK_NULL_HANDLE;
    self->desc_set_layout_infos[i].num_layout_bindings = 0;
    self->desc_set_layout_infos[i].num_image_samplers  = 0;
    self->desc_set_layout_infos[i].num_uniforms        = 0;
  }

  std::strncpy(self->debug_name, params->debug_name ? params->debug_name : "NO_DEBUG_NAME", bfCArraySize(self->debug_name));
  self->debug_name[bfCArraySize(self->debug_name) - 1] = '\0';

  return self;
}

bfShaderType bfShaderModule_type(bfShaderModuleHandle self)
{
  return self->type;
}

bfBool32 bfShaderModule_loadData(bfShaderModuleHandle self, const char* source, size_t source_length)
{
  assert(source && source_length && "bfShaderModule_loadData invalid parameters");

  VkShaderModuleCreateInfo create_info;
  create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext    = nullptr;
  create_info.flags    = 0;
  create_info.codeSize = source_length;
  create_info.pCode    = reinterpret_cast<const uint32_t*>(source);

  std::strncpy(self->entry_point, "main", 5);

  const VkResult error = vkCreateShaderModule(
   self->parent->handle,
   &create_info,
   BIFROST_VULKAN_CUSTOM_ALLOCATOR,
   &self->handle);

  return error == VK_SUCCESS;
}

void bfShaderProgram_addModule(bfShaderProgramHandle self, bfShaderModuleHandle module)
{
  for (std::size_t i = 0; i < self->modules.size; ++i)
  {
    if (self->modules.elements[i] == module || self->modules.elements[i]->type == module->type)
    {
      self->modules.elements[i] = module;
      return;
    }
  }

  self->modules.elements[self->modules.size++] = module;
}

void bfShaderProgram_link(bfShaderProgramHandle self)
{
  /* No-OP Dy Design */
}

void bfShaderProgram_addAttribute(bfShaderProgramHandle self, const char* name, uint32_t binding)
{
  (void)self;
  (void)name;
  (void)binding;
  /* NO-OP */
}

void bfShaderProgram_addUniformBuffer(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, bfShaderStageBits stages)
{
  (void)name;  // NOTE(Shareef): Vulkan isn't lame like OpenGL thus doesn't use strings for names

  assert(set < self->num_desc_set_layouts);

  bfDescriptorSetLayoutInfo*    desc_set     = self->desc_set_layout_infos + set;
  VkDescriptorSetLayoutBinding* desc_binding = desc_set->layout_bindings + desc_set->num_layout_bindings;

  desc_binding->binding            = binding;
  desc_binding->descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  desc_binding->descriptorCount    = how_many;
  desc_binding->stageFlags         = bfVkConvertShaderStage(stages);
  desc_binding->pImmutableSamplers = nullptr;

  ++desc_set->num_layout_bindings;
  ++desc_set->num_uniforms;
}

void bfShaderProgram_addImageSampler(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, bfShaderStageBits stages)
{
  (void)name;  // NOTE(Shareef): Vulkan isn't lame like OpenGL thus doesn't use strings for names

  assert(set < self->num_desc_set_layouts);

  bfDescriptorSetLayoutInfo*    desc_set     = self->desc_set_layout_infos + set;
  VkDescriptorSetLayoutBinding* desc_binding = desc_set->layout_bindings + desc_set->num_layout_bindings;

  desc_binding->binding            = binding;
  desc_binding->descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  desc_binding->descriptorCount    = how_many;
  desc_binding->stageFlags         = bfVkConvertShaderStage(stages);
  desc_binding->pImmutableSamplers = nullptr;

  ++desc_set->num_layout_bindings;
  ++desc_set->num_image_samplers;
}

void bfShaderProgram_compile(bfShaderProgramHandle self)
{
  for (uint32_t i = 0; i < self->num_desc_set_layouts; ++i)
  {
    VkDescriptorSetLayout*     desc_set      = self->desc_set_layouts + i;
    bfDescriptorSetLayoutInfo* desc_set_info = self->desc_set_layout_infos + i;

    VkDescriptorSetLayoutCreateInfo desc_set_create_info;

    desc_set_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc_set_create_info.pNext        = nullptr;
    desc_set_create_info.flags        = 0x0;
    desc_set_create_info.bindingCount = desc_set_info->num_layout_bindings;
    desc_set_create_info.pBindings    = desc_set_info->layout_bindings;

    vkCreateDescriptorSetLayout(
     self->parent->handle,
     &desc_set_create_info,
     BIFROST_VULKAN_CUSTOM_ALLOCATOR,
     desc_set);
  }

  VkPipelineLayoutCreateInfo pipeline_layout_info;
  std::memset(&pipeline_layout_info, 0, sizeof(pipeline_layout_info));
  pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount         = self->num_desc_set_layouts;
  pipeline_layout_info.pSetLayouts            = self->desc_set_layouts;
  pipeline_layout_info.pushConstantRangeCount = 0;        // Optional
  pipeline_layout_info.pPushConstantRanges    = nullptr;  // Optional

  vkCreatePipelineLayout(
   self->parent->handle,
   &pipeline_layout_info,
   BIFROST_VULKAN_CUSTOM_ALLOCATOR,
   &self->layout);
}

bfDescriptorSetHandle bfShaderProgram_createDescriptorSet(bfShaderProgramHandle self_, uint32_t index)
{
  assert(index < self_->num_desc_set_layouts);

  bfDescriptorSetHandle self = xxx_AllocGfxObject<bfDescriptorSet>(BF_GFX_OBJECT_DESCRIPTOR_SET, &g_Ctx->obj_man);

  self->shader_program       = self_;
  self->set_index            = index;
  self->handle               = VK_NULL_HANDLE;
  self->num_buffer_info      = 0;
  self->num_image_info       = 0;
  self->num_buffer_view_info = 0;
  self->num_writes           = 0;

  MaterialPool_alloc(self_->parent->descriptor_pool, self);

  return self;
}

static VkWriteDescriptorSet* bfDescriptorSet__checkForFlush(
 bfDescriptorSetHandle self,
 VkDescriptorType      type,
 uint32_t              binding,
 uint32_t              array_element_start,
 uint32_t              num_buffer_info,
 uint32_t              num_image_info,
 uint32_t              num_buffer_view_info)
{
  if (self->num_buffer_info + num_buffer_info > bfCArraySize(self->buffer_info) ||
      self->num_image_info + num_image_info > bfCArraySize(self->image_info) ||
      self->num_buffer_view_info + num_buffer_view_info > bfCArraySize(self->buffer_view_info) ||
      self->num_writes > k_bfGfxMaxDescriptorSetWrites)
  {
    bfDescriptorSet_flushWrites(self);
  }

  VkWriteDescriptorSet* const write = self->writes + self->num_writes;

  write->sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write->pNext            = nullptr;
  write->dstSet           = self->handle;
  write->dstBinding       = binding;
  write->dstArrayElement  = array_element_start;
  write->descriptorType   = type;
  write->descriptorCount  = std::max(num_buffer_info, std::max(num_image_info, num_buffer_view_info));  // Mutually Exlusive
  write->pBufferInfo      = num_buffer_info > 0 ? self->buffer_info + self->num_buffer_info : nullptr;
  write->pImageInfo       = num_image_info > 0 ? self->image_info + self->num_image_info : nullptr;
  write->pTexelBufferView = num_buffer_view_info > 0 ? self->buffer_view_info + self->num_buffer_view_info : nullptr;

  self->num_buffer_info += num_buffer_info;
  self->num_image_info += num_image_info;
  self->num_buffer_view_info += num_buffer_view_info;
  ++self->num_writes;

  return write;
}

void bfDescriptorSet_setCombinedSamplerTextures(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, bfTextureHandle* textures, uint32_t num_textures)
{
  VkWriteDescriptorSet*  write       = bfDescriptorSet__checkForFlush(self, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, binding, array_element_start, 0, num_textures, 0);
  VkDescriptorImageInfo* image_infos = const_cast<VkDescriptorImageInfo*>(write->pImageInfo);

  for (uint32_t i = 0; i < num_textures; ++i)
  {
    image_infos[i].sampler     = textures[i]->tex_sampler;
    image_infos[i].imageView   = textures[i]->tex_view;
    image_infos[i].imageLayout = bfVkConvertImgLayout(textures[i]->tex_layout);
  }
}

void bfDescriptorSet_setUniformBuffers(bfDescriptorSetHandle self, uint32_t binding, const bfBufferSize* offsets, const bfBufferSize* sizes, bfBufferHandle* buffers, uint32_t num_buffers)
{
  VkWriteDescriptorSet*   write        = bfDescriptorSet__checkForFlush(self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding, 0, num_buffers, 0, 0);
  VkDescriptorBufferInfo* buffer_infos = const_cast<VkDescriptorBufferInfo*>(write->pBufferInfo);

  for (uint32_t i = 0; i < num_buffers; ++i)
  {
    buffer_infos[i].buffer = buffers[i]->handle;
    buffer_infos[i].offset = offsets[i];
    buffer_infos[i].range  = sizes[i];
  }
}

void bfDescriptorSet_flushWrites(bfDescriptorSetHandle self)
{
  vkUpdateDescriptorSets(
   self->shader_program->parent->handle,
   self->num_writes,
   self->writes,
   0,
   nullptr);

  self->num_buffer_info      = 0;
  self->num_image_info       = 0;
  self->num_buffer_view_info = 0;
  self->num_writes           = 0;
}

/* Texture */
bfTextureHandle bfGfxDevice_newTexture(bfGfxDeviceHandle self_, const bfTextureCreateParams* params)
{
  bfTextureHandle self = xxx_AllocGfxObject<bfTexture>(BF_GFX_OBJECT_TEXTURE, &g_Ctx->obj_man);

  self->parent            = self_;
  self->flags             = params->flags;
  self->image_type        = params->type;
  self->image_width       = params->width;
  self->image_height      = params->height;
  self->image_depth       = params->depth;
  self->image_miplevels   = params->generate_mipmaps;
  self->tex_image         = VK_NULL_HANDLE;
  self->tex_memory        = VK_NULL_HANDLE;
  self->tex_view          = VK_NULL_HANDLE;
  self->tex_sampler       = VK_NULL_HANDLE;
  self->tex_layout        = BF_IMAGE_LAYOUT_UNDEFINED;
  self->tex_format        = bfVkConvertFormat(params->format);
  self->tex_samples       = params->sample_count;
  self->memory_properties = params->memory_properties;

  if (self->image_miplevels)
  {
    // Vulkan Spec requires blit feature on the format
    // to use the 'vkCmdBlitImage' command on it.

    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(self_->parent->handle, self->tex_format, &format_properties);

    if (!(format_properties.linearTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)))
    {
      assert(!"This is not a real error, just a warning but I do not want to forget to add a warning.");
      self->image_miplevels = 0;
    }
  }

  return self;
}

uint32_t bfTexture_width(bfTextureHandle self)
{
  return self->image_width;
}

uint32_t bfTexture_height(bfTextureHandle self)
{
  return self->image_height;
}

uint32_t bfTexture_depth(bfTextureHandle self)
{
  return self->image_depth;
}

uint32_t bfTexture_numMipLevels(bfTextureHandle self)
{
  return self->image_miplevels;
}

bfGfxImageLayout bfTexture_layout(bfTextureHandle self)
{
  return self->tex_layout;
}

void setImageLayout(VkCommandBuffer cmd_buffer, VkImage image, VkImageAspectFlags aspects, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels)
{
  VkImageMemoryBarrier image_barrier;
  image_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  image_barrier.pNext                           = nullptr;
  image_barrier.srcAccessMask                   = 0x0;
  image_barrier.dstAccessMask                   = 0x0;
  image_barrier.oldLayout                       = old_layout;
  image_barrier.newLayout                       = new_layout;
  image_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  image_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  image_barrier.image                           = image;
  image_barrier.subresourceRange.aspectMask     = aspects;
  image_barrier.subresourceRange.baseMipLevel   = 0;
  image_barrier.subresourceRange.levelCount     = mip_levels;
  image_barrier.subresourceRange.baseArrayLayer = 0;
  image_barrier.subresourceRange.layerCount     = 1;

  switch (old_layout)
  {
    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      image_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      image_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      image_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      image_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    default:
      // image_barrier.srcAccessMask = 0x0;
      break;
  }

  switch (new_layout)
  {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      image_barrier.srcAccessMask |= VK_ACCESS_TRANSFER_READ_BIT;
      image_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      image_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      image_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      image_barrier.dstAccessMask |=
       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      image_barrier.srcAccessMask = /*VK_ACCESS_HOST_WRITE_BIT | */ VK_ACCESS_TRANSFER_WRITE_BIT;
      image_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    default:
      assert(0);
      break;
  }

  VkPipelineStageFlags src_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dst_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    src_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dst_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    src_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dst_flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (old_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    src_flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dst_flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }

  vkCmdPipelineBarrier(
   cmd_buffer,
   src_flags,
   dst_flags,
   0,
   0,
   nullptr,
   0,
   nullptr,
   1,
   &image_barrier);
}

VkImageAspectFlags bfTexture__aspect(bfTextureHandle self)
{
  VkImageAspectFlags aspects = 0x0;

  if (self->flags & BF_TEX_IS_DEPTH_ATTACHMENT)
  {
    aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;

    if (self->flags & BF_TEX_IS_STENCIL_ATTACHMENT)
    {
      aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else if (self->flags & BF_TEX_IS_COLOR_ATTACHMENT)
  {
    aspects |= VK_IMAGE_ASPECT_COLOR_BIT;
  }
  else  // Sampled Texture
  {
    aspects |= VK_IMAGE_ASPECT_COLOR_BIT;
  }

  return aspects;
}

static void bfTexture__setLayout(bfTextureHandle self, bfGfxImageLayout layout)
{
  const auto transient_cmd = gfxContextBeginTransientCommandBuffer();
  setImageLayout(transient_cmd.handle, self->tex_image, bfTexture__aspect(self), bfVkConvertImgLayout(self->tex_layout), bfVkConvertImgLayout(layout), self->image_miplevels);
  gfxContextEndTransientCommandBuffer(transient_cmd);
  self->tex_layout = layout;
}

static void bfTexture__createImage(bfTextureHandle self)
{
  if (self->tex_image)
  {
    return;
  }

  VkImageCreateInfo create_image;
  create_image.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  create_image.pNext                 = nullptr;
  create_image.flags                 = 0x0;
  create_image.imageType             = bfVkConvertTextureType(self->image_type);
  create_image.format                = self->tex_format;
  create_image.extent.width          = self->image_width;
  create_image.extent.height         = self->image_height;
  create_image.extent.depth          = self->image_depth;
  create_image.mipLevels             = self->image_miplevels;
  create_image.arrayLayers           = 1;  // TODO:
  create_image.samples               = bfVkConvertSampleCount(self->tex_samples);
  create_image.tiling                = self->flags & BF_TEX_IS_LINEAR ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
  create_image.usage                 = 0x0;
  create_image.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
  create_image.queueFamilyIndexCount = 0;
  create_image.pQueueFamilyIndices   = nullptr;
  create_image.initialLayout         = bfVkConvertImgLayout(self->tex_layout);

  if (self->flags & BF_TEX_IS_MULTI_QUEUE)
  {
    create_image.sharingMode = VK_SHARING_MODE_CONCURRENT;
  }

  // TODO: Make this a function? BGN
  if (self->flags & BF_TEX_IS_TRANSFER_SRC || self->image_miplevels > 1)
  {
    create_image.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if (self->flags & BF_TEX_IS_TRANSFER_DST)
  {
    create_image.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  if (self->flags & BF_TEX_IS_SAMPLED)
  {
    create_image.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  if (self->flags & BF_TEX_IS_STORAGE)
  {
    create_image.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
  }

  if (self->flags & BF_TEX_IS_COLOR_ATTACHMENT)
  {
    create_image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  if (self->flags & (BF_TEX_IS_DEPTH_ATTACHMENT | BF_TEX_IS_STENCIL_ATTACHMENT))
  {
    create_image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  if (self->flags & BF_TEX_IS_TRANSIENT)
  {
    create_image.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
  }

  if (self->flags & BF_TEX_IS_INPUT_ATTACHMENT)
  {
    create_image.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }
  // TODO: Make this a function? END

  const VkResult error = vkCreateImage(self->parent->handle, &create_image, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->tex_image);
  assert(error == VK_SUCCESS);
}

static void bfTexture__allocMemory(bfTextureHandle self)
{
  if (self->tex_memory)
  {
    return;
  }

  // TODO: Switch to Pool Allocator?

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(self->parent->handle, self->tex_image, &memRequirements);

  VkMemoryAllocateInfo alloc_info;
  alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext          = nullptr;
  alloc_info.allocationSize = memRequirements.size;
  // alloc_info.memoryTypeIndex = 0;

  memory_type_from_properties(self->parent->parent->memory_properties, memRequirements.memoryTypeBits, bfVkConvertBufferPropertyFlags(self->memory_properties), &alloc_info.memoryTypeIndex);

  vkAllocateMemory(self->parent->handle, &alloc_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->tex_memory);
  vkBindImageMemory(self->parent->handle, self->tex_image, self->tex_memory, 0);
}

bfBool32 bfTexture_loadFile(bfTextureHandle self, const char* file)
{
  static constexpr size_t k_NumReqComps = 4;

  int      num_components = 0;
  stbi_uc* texture_data   = stbi_load(file, &self->image_width, &self->image_height, &num_components, STBI_rgb_alpha);

  if (texture_data)
  {
    const size_t num_req_bytes = size_t(self->image_width) * size_t(self->image_height) * k_NumReqComps * sizeof(char);

    bfTexture_loadData(self, reinterpret_cast<const char*>(texture_data), num_req_bytes);

    stbi_image_free(texture_data);

    return bfTrue;
  }

  return bfFalse;
}

bfBool32 bfTexture_loadPNG(bfTextureHandle self, const void* png_bytes, size_t png_bytes_length)
{
  static const size_t k_NumReqComps = 4;

  int      num_components = 0;
  stbi_uc* texture_data   = stbi_load_from_memory((const stbi_uc*)png_bytes, int(png_bytes_length), &self->image_width, &self->image_height, &num_components, STBI_rgb_alpha);

  if (texture_data)
  {
    const size_t num_req_bytes = (size_t)(self->image_width) * (size_t)(self->image_height) * k_NumReqComps * sizeof(char);

    bfTexture_loadData(self, (const char*)texture_data, num_req_bytes);

    stbi_image_free(texture_data);

    return bfTrue;
  }

  return bfFalse;
}

bfBool32 bfTexture_loadDataRange(bfTextureHandle self, const void* pixels, size_t pixels_length, const int32_t offset[3], const uint32_t sizes[3])
{
  const bool is_indefinite =
   self->image_width == k_bfTextureUnknownSize ||
   self->image_height == k_bfTextureUnknownSize ||
   self->image_depth == k_bfTextureUnknownSize;

  assert(!is_indefinite && "Texture_setData: The texture dimensions should be defined by this point.");

  self->image_miplevels = self->image_miplevels ? 1 + uint32_t(std::floor(std::log2(float(std::max(std::max(self->image_width, self->image_height), self->image_depth))))) : 1;

  bfTexture__createImage(self);
  bfTexture__allocMemory(self);

  if (!self->tex_view)
  {
    self->tex_view = bfCreateImageView2D(self->parent->handle, self->tex_image, self->tex_format, bfTexture__aspect(self), self->image_miplevels);
  }

  if (pixels)
  {
    // TODO(SR): This should not be creating a local temp buffer, rather this staging buffer should be some kind of reused resoruce.

    bfBufferCreateParams buffer_params;
    buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
    buffer_params.allocation.size       = pixels_length;
    buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_SRC;

    const bfBufferHandle staging_buffer = bfGfxDevice_newBuffer(self->parent, &buffer_params);

    bfBuffer_map(staging_buffer, 0, k_bfBufferWholeSize);
    bfBuffer_copyCPU(staging_buffer, 0, pixels, pixels_length);
    bfBuffer_unMap(staging_buffer);

    bfTexture_loadBuffer(self, staging_buffer, offset, sizes);
    bfGfxDevice_release(self->parent, staging_buffer);
  }

  return bfTrue;
}

void bfTexture_loadBuffer(bfTextureHandle self, bfBufferHandle buffer, const int32_t offset[3], const uint32_t sizes[3])
{
  bfTexture__setLayout(self, BF_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  const auto transient_cmd = gfxContextBeginTransientCommandBuffer();
  {
    VkBufferImageCopy region;

    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = bfTexture__aspect(self);
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset = {offset[0], offset[1], offset[2]};
    region.imageExtent = {sizes[0], sizes[1], sizes[2]};

    vkCmdCopyBufferToImage(
     transient_cmd.handle,
     buffer->handle,
     self->tex_image,
     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
     1,
     &region);
  }
  gfxContextEndTransientCommandBuffer(transient_cmd);

  if (self->image_miplevels > 1)
  {
    int32_t mip_width  = self->image_width;
    int32_t mip_height = self->image_height;

    const auto copy_cmds = gfxContextBeginTransientCommandBuffer();
    {
      VkImageMemoryBarrier barrier            = {};
      barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.image                           = self->tex_image;
      barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
      barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount     = 1;
      barrier.subresourceRange.levelCount     = 1;

      for (uint32_t i = 1; i < self->image_miplevels; ++i)
      {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(copy_cmds.handle,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        VkImageBlit image_blit{};

        // Source
        image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_blit.srcSubresource.layerCount = 1;
        image_blit.srcSubresource.mipLevel   = i - 1;
        image_blit.srcOffsets[1].x           = mip_width;
        image_blit.srcOffsets[1].y           = mip_height;
        image_blit.srcOffsets[1].z           = 1;

        // Destination
        image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_blit.dstSubresource.layerCount = 1;
        image_blit.dstSubresource.mipLevel   = i;
        image_blit.dstOffsets[1].x           = mip_width > 1 ? mip_width / 2 : 1;
        image_blit.dstOffsets[1].y           = mip_height > 1 ? mip_height / 2 : 1;
        image_blit.dstOffsets[1].z           = 1;

        vkCmdBlitImage(copy_cmds.handle,
                       self->tex_image,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       self->tex_image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1,
                       &image_blit,
                       VK_FILTER_LINEAR);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(copy_cmds.handle,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        if (mip_width > 1)
        {
          mip_width /= 2;
        }

        if (mip_height > 1)
        {
          mip_height /= 2;
        }
      }

      barrier.subresourceRange.baseMipLevel = self->image_miplevels - 1;
      barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(copy_cmds.handle,
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0,
                           0,
                           nullptr,
                           0,
                           nullptr,
                           1,
                           &barrier);
    }
    gfxContextEndTransientCommandBuffer(copy_cmds);

    self->tex_layout = BF_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  else
  {
    bfTexture__setLayout(self, BF_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void bfTexture_setSampler(bfTextureHandle self, const bfTextureSamplerProperties* sampler_properties)
{
  if (self->tex_sampler)
  {
    vkDestroySampler(self->parent->handle, self->tex_sampler, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
    self->tex_sampler = VK_NULL_HANDLE;
  }

  if (sampler_properties)
  {
    VkSamplerCreateInfo sampler_info;
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.pNext = NULL;
    sampler_info.flags = 0x0;
    // Filters
    sampler_info.magFilter = bfVkConvertSamplerFilterMode(sampler_properties->mag_filter);
    sampler_info.minFilter = bfVkConvertSamplerFilterMode(sampler_properties->min_filter);
    // Extra Filtering
    sampler_info.anisotropyEnable = VK_FALSE;
    sampler_info.maxAnisotropy    = 1.0f;
    // Normalized Coords (Should probably always be false)
    sampler_info.unnormalizedCoordinates = VK_FALSE;  // VK_FALSE = [0, 1], VK_TRUE = [0, texture_width]
    sampler_info.compareEnable           = VK_FALSE;
    sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    // MipMapping
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod     = sampler_properties->min_lod;
    sampler_info.maxLod     = sampler_properties->max_lod;
    // Bordering
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // Address Modes
    sampler_info.addressModeU = bfVkConvertSamplerAddressMode(sampler_properties->u_address);
    sampler_info.addressModeV = bfVkConvertSamplerAddressMode(sampler_properties->v_address);
    sampler_info.addressModeW = bfVkConvertSamplerAddressMode(sampler_properties->w_address);

    const VkResult error = vkCreateSampler(self->parent->handle, &sampler_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->tex_sampler);
    assert(error == VK_SUCCESS);  // NOTE(Shareef): maybe return whether or not this was successfull?
  }
}

/* Vertex Binding */
bfVertexLayoutSetHandle bfVertexLayout_new(void)
{
  bfVertexLayoutSetHandle self = xxx_Alloc_<bfVertexLayoutSet>();
  std::memset(self, 0x0, sizeof(bfVertexLayoutSet));
  return self;
}

static void bfVertexLayout_addXBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t sizeof_vertex, VkVertexInputRate input_rate)
{
  assert(self->num_attrib_bindings < k_bfGfxMaxLayoutBindings);

  VkVertexInputBindingDescription* desc = self->buffer_bindings + self->num_buffer_bindings;

  desc->binding   = binding;
  desc->stride    = sizeof_vertex;
  desc->inputRate = input_rate;

  ++self->num_buffer_bindings;
}

void bfVertexLayout_addVertexBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t sizeof_vertex)
{
  bfVertexLayout_addXBinding(self, binding, sizeof_vertex, VK_VERTEX_INPUT_RATE_VERTEX);
}

void bfVertexLayout_addInstanceBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t stride)
{
  bfVertexLayout_addXBinding(self, binding, stride, VK_VERTEX_INPUT_RATE_INSTANCE);
}

void bfVertexLayout_addVertexLayout(bfVertexLayoutSetHandle self, uint32_t binding, bfGfxVertexFormatAttribute format, uint32_t offset)
{
  assert(self->num_attrib_bindings < k_bfGfxMaxLayoutBindings);

  VkVertexInputAttributeDescription* desc = self->attrib_bindings + self->num_attrib_bindings;

  desc->location = self->num_attrib_bindings;
  desc->binding  = binding;
  desc->format   = bfVkConvertVertexFormatAttrib(format);
  desc->offset   = offset;

  ++self->num_attrib_bindings;
}

void bfVertexLayout_delete(bfVertexLayoutSetHandle self)
{
  delete self;
}

// Idk what's happeneing with this code

void UpdateResourceFrame(bfBaseGfxObject* obj)
{
  obj->last_frame_used = g_Ctx->frame_count;
}

// ReSharper restore CppZeroConstantCanBeReplacedWithNullptr

#include "vulkan/bf_vulkan_conversions.h" /* bfVKConvert* */
#include "vulkan/bf_vulkan_hash.hpp" /* hash         */
#include "vulkan/bf_vulkan_logical_device.h"

#include <cassert> /* assert */

#define CUSTOM_ALLOC nullptr

extern VkImageAspectFlags bfTexture__aspect(bfTextureHandle self);

void AddCachedResource(bfGfxDeviceHandle device, bfBaseGfxObject* obj, std::uint64_t hash_code)
{
  obj->hash_code           = hash_code;
  obj->next                = device->cached_resources;
  device->cached_resources = obj;
}

void UpdateResourceFrame(bfBaseGfxObject* obj);

bfWindowSurfaceHandle bfGfxCmdList_window(bfGfxCommandListHandle self)
{
  return self->window;
}

bfBool32 bfGfxCmdList_begin(bfGfxCommandListHandle self)
{
  VkCommandBufferBeginInfo begin_info;
  begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext            = NULL;
  begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  begin_info.pInheritanceInfo = NULL;

  const VkResult error = vkBeginCommandBuffer(self->handle, &begin_info);

  self->dynamic_state_dirty = 0xFFFF;

  assert(error == VK_SUCCESS);

  return error == VK_SUCCESS;
}

void bfGfxCmdList_pipelineBarriers(bfGfxCommandListHandle self, bfGfxPipelineStageBits src_stage, bfGfxPipelineStageBits dst_stage, const bfPipelineBarrier* barriers, uint32_t num_barriers, bfBool32 reads_same_pixel)
{
  std::uint32_t         num_memory_barriers = 0;
  VkMemoryBarrier       memory_barriers[k_bfGfxMaxPipelineBarrierWrites];
  std::uint32_t         num_buffer_barriers = 0;
  VkBufferMemoryBarrier buffer_barriers[k_bfGfxMaxPipelineBarrierWrites];
  std::uint32_t         num_image_barriers = 0;
  VkImageMemoryBarrier  image_barriers[k_bfGfxMaxPipelineBarrierWrites];

  const std::uint32_t* queue_list = self->parent->parent->queue_list.family_index;

  for (uint32_t i = 0; i < num_barriers; ++i)
  {
    const bfPipelineBarrier* const pl_barrier = barriers + i;

    switch (pl_barrier->type)
    {
      case BF_PIPELINE_BARRIER_MEMORY:
      {
        assert(num_memory_barriers < k_bfGfxMaxPipelineBarrierWrites);

        VkMemoryBarrier* const barrier = memory_barriers + num_memory_barriers;
        barrier->sType                 = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier->pNext                 = nullptr;
        barrier->srcAccessMask         = bfVkConvertAccessFlags(pl_barrier->access[0]);
        barrier->dstAccessMask         = bfVkConvertAccessFlags(pl_barrier->access[1]);

        ++num_memory_barriers;
        break;
      }
      case BF_PIPELINE_BARRIER_BUFFER:
      {
        assert(num_buffer_barriers < k_bfGfxMaxPipelineBarrierWrites);

        VkBufferMemoryBarrier* const barrier = buffer_barriers + num_buffer_barriers;
        barrier->sType                       = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier->pNext                       = nullptr;
        barrier->srcAccessMask               = bfVkConvertAccessFlags(pl_barrier->access[0]);
        barrier->dstAccessMask               = bfVkConvertAccessFlags(pl_barrier->access[1]);
        barrier->srcQueueFamilyIndex         = bfConvertQueueIndex(queue_list, pl_barrier->queue_transfer[0]);
        barrier->dstQueueFamilyIndex         = bfConvertQueueIndex(queue_list, pl_barrier->queue_transfer[1]);
        barrier->buffer                      = pl_barrier->buffer.handle->handle;
        barrier->offset                      = pl_barrier->buffer.offset;
        barrier->size                        = pl_barrier->buffer.size;

        ++num_buffer_barriers;
        break;
      }
      case BF_PIPELINE_BARRIER_IMAGE:
      {
        assert(num_image_barriers < k_bfGfxMaxPipelineBarrierWrites);

        VkImageMemoryBarrier* const barrier      = image_barriers + num_image_barriers;
        barrier->sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier->pNext                           = nullptr;
        barrier->srcAccessMask                   = bfVkConvertAccessFlags(pl_barrier->access[0]);
        barrier->dstAccessMask                   = bfVkConvertAccessFlags(pl_barrier->access[1]);
        barrier->oldLayout                       = bfVkConvertImgLayout(pl_barrier->image.layout_transition[0]);
        barrier->newLayout                       = bfVkConvertImgLayout(pl_barrier->image.layout_transition[1]);
        barrier->srcQueueFamilyIndex             = bfConvertQueueIndex(queue_list, pl_barrier->queue_transfer[0]);
        barrier->dstQueueFamilyIndex             = bfConvertQueueIndex(queue_list, pl_barrier->queue_transfer[1]);
        barrier->image                           = pl_barrier->image.handle->tex_image;
        barrier->subresourceRange.aspectMask     = bfTexture__aspect(pl_barrier->image.handle);
        barrier->subresourceRange.baseMipLevel   = pl_barrier->image.base_mip_level;
        barrier->subresourceRange.levelCount     = pl_barrier->image.level_count;
        barrier->subresourceRange.baseArrayLayer = pl_barrier->image.base_array_layer;
        barrier->subresourceRange.layerCount     = pl_barrier->image.layer_count;

        pl_barrier->image.handle->tex_layout = pl_barrier->image.layout_transition[1];

        ++num_image_barriers;
        break;
      }
        bfInvalidDefaultCase();
    }
  }

  vkCmdPipelineBarrier(
   self->handle,
   bfVkConvertPipelineStageFlags(src_stage),
   bfVkConvertPipelineStageFlags(dst_stage),
   reads_same_pixel ? VK_DEPENDENCY_BY_REGION_BIT : 0x0,
   num_memory_barriers,
   memory_barriers,
   num_buffer_barriers,
   buffer_barriers,
   num_image_barriers,
   image_barriers);
}

void bfGfxCmdList_setRenderpass(bfGfxCommandListHandle self, bfRenderpassHandle renderpass)
{
  self->pipeline_state.renderpass = renderpass;
  UpdateResourceFrame(&renderpass->super);
}

void bfGfxCmdList_setRenderpassInfo(bfGfxCommandListHandle self, const bfRenderpassInfo* renderpass_info)
{
  const uint64_t hash_code = bf::gfx_hash::hash(0x0, renderpass_info);

  bfRenderpassHandle rp = self->parent->cache_renderpass.find(hash_code, *renderpass_info);

  if (!rp)
  {
    rp = bfGfxDevice_newRenderpass(self->parent, renderpass_info);
    self->parent->cache_renderpass.insert(hash_code, rp, *renderpass_info);
    AddCachedResource(self->parent, &rp->super, hash_code);
  }

  bfGfxCmdList_setRenderpass(self, rp);
}

void bfGfxCmdList_setClearValues(bfGfxCommandListHandle self, const bfClearValue* clear_values)
{
  const std::size_t num_clear_colors = self->pipeline_state.renderpass->info.num_attachments;

  for (std::size_t i = 0; i < num_clear_colors; ++i)
  {
    const bfClearValue* const src_color = clear_values + i;
    VkClearValue* const       dst_color = self->clear_colors + i;

    *dst_color = bfVKConvertClearColor(src_color);
  }
}

void bfGfxCmdList_setAttachments(bfGfxCommandListHandle self, bfTextureHandle* attachments)
{
  const uint32_t num_attachments = self->pipeline_state.renderpass->info.num_attachments;
  const uint64_t hash_code       = bf::vk::hash(0x0, attachments, num_attachments);

  bfFramebufferState fb_state;
  fb_state.num_attachments = num_attachments;

  for (uint32_t i = 0; i < num_attachments; ++i)
  {
    fb_state.attachments[i] = attachments[i];
  }

  bfFramebufferHandle fb = self->parent->cache_framebuffer.find(hash_code, fb_state);

  if (!fb)
  {
    VkImageView image_views[k_bfGfxMaxAttachments];

    fb = xxx_AllocGfxObject<bfFramebuffer>(BF_GFX_OBJECT_FRAMEBUFFER, &g_Ctx->obj_man);

    for (uint32_t i = 0; i < num_attachments; ++i)
    {
      image_views[i] = attachments[i]->tex_view;
    }

    VkFramebufferCreateInfo frame_buffer_create_params;
    frame_buffer_create_params.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frame_buffer_create_params.pNext           = nullptr;
    frame_buffer_create_params.flags           = 0;
    frame_buffer_create_params.renderPass      = self->pipeline_state.renderpass->handle;
    frame_buffer_create_params.attachmentCount = num_attachments;
    frame_buffer_create_params.pAttachments    = image_views;
    frame_buffer_create_params.width           = static_cast<uint32_t>(attachments[0]->image_width);
    frame_buffer_create_params.height          = static_cast<uint32_t>(attachments[0]->image_height);
    frame_buffer_create_params.layers          = static_cast<uint32_t>(attachments[0]->image_depth);

    const VkResult err = vkCreateFramebuffer(self->parent->handle, &frame_buffer_create_params, nullptr, &fb->handle);
    assert(err == VK_SUCCESS);

    self->parent->cache_framebuffer.insert(hash_code, fb, fb_state);
    AddCachedResource(self->parent, &fb->super, hash_code);
  }

  self->attachment_size[0] = static_cast<uint32_t>(attachments[0]->image_width);
  self->attachment_size[1] = static_cast<uint32_t>(attachments[0]->image_height);
  self->framebuffer        = fb;

  UpdateResourceFrame(&fb->super);
}

void bfGfxCmdList_setRenderAreaAbs(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
  self->render_area.offset.x      = x;
  self->render_area.offset.y      = y;
  self->render_area.extent.width  = width;
  self->render_area.extent.height = height;

  const float depths[2] = {0.0f, 1.0f};
  bfGfxCmdList_setViewport(self, float(x), float(y), float(width), float(height), depths);
  bfGfxCmdList_setScissorRect(self, x, y, width, height);
}

extern "C" void bfGfxCmdList_setRenderAreaRelImpl(float fb_width, float fb_height, bfGfxCommandListHandle self, float x, float y, float width, float height);

void bfGfxCmdList_setRenderAreaRel(bfGfxCommandListHandle self, float x, float y, float width, float height)
{
  bfGfxCmdList_setRenderAreaRelImpl(float(self->attachment_size[0]), float(self->attachment_size[1]), self, x, y, width, height);
}

void bfGfxCmdList_beginRenderpass(bfGfxCommandListHandle self)
{
  VkRenderPassBeginInfo begin_render_pass_info;
  begin_render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  begin_render_pass_info.pNext           = nullptr;
  begin_render_pass_info.renderPass      = self->pipeline_state.renderpass->handle;
  begin_render_pass_info.framebuffer     = self->framebuffer->handle;
  begin_render_pass_info.renderArea      = self->render_area;
  begin_render_pass_info.clearValueCount = self->pipeline_state.renderpass->info.num_attachments;
  begin_render_pass_info.pClearValues    = self->clear_colors;

  vkCmdBeginRenderPass(self->handle, &begin_render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

  self->pipeline_state.state.subpass_index = 0;
}

void bfGfxCmdList_nextSubpass(bfGfxCommandListHandle self)
{
  vkCmdNextSubpass(self->handle, VK_SUBPASS_CONTENTS_INLINE);
  ++self->pipeline_state.state.subpass_index;
}

#define state(self) self->pipeline_state.state

void bfGfxCmdList_setDrawMode(bfGfxCommandListHandle self, bfDrawMode draw_mode)
{
  state(self).draw_mode = draw_mode;
}

void bfGfxCmdList_setFrontFace(bfGfxCommandListHandle self, bfFrontFace front_face)
{
  state(self).front_face = front_face;
}

void bfGfxCmdList_setCullFace(bfGfxCommandListHandle self, bfCullFaceFlags cull_face)
{
  state(self).cull_face = cull_face;
}

void bfGfxCmdList_setDepthTesting(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_depth_test = value;
}

void bfGfxCmdList_setDepthWrite(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_depth_write = value;
}

void bfGfxCmdList_setDepthTestOp(bfGfxCommandListHandle self, bfCompareOp op)
{
  state(self).depth_test_op = op;
}

void bfGfxCmdList_setStencilTesting(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_stencil_test = value;
}

void bfGfxCmdList_setPrimitiveRestart(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_primitive_restart = value;
}

void bfGfxCmdList_setRasterizerDiscard(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_rasterizer_discard = value;
}

void bfGfxCmdList_setDepthBias(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_depth_bias = value;
}

void bfGfxCmdList_setSampleShading(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_sample_shading = value;
}

void bfGfxCmdList_setAlphaToCoverage(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_alpha_to_coverage = value;
}

void bfGfxCmdList_setAlphaToOne(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_alpha_to_one = value;
}

void bfGfxCmdList_setLogicOpEnabled(bfGfxCommandListHandle self, bfBool32 value)
{
  state(self).do_logic_op = value;
}

void bfGfxCmdList_setLogicOp(bfGfxCommandListHandle self, bfLogicOp op)
{
  state(self).logic_op = op;
}

void bfGfxCmdList_setPolygonFillMode(bfGfxCommandListHandle self, bfPolygonFillMode fill_mode)
{
  state(self).fill_mode = fill_mode;
}

#undef state

void bfGfxCmdList_setColorWriteMask(bfGfxCommandListHandle self, uint32_t output_attachment_idx, uint8_t color_mask)
{
  self->pipeline_state.blending[output_attachment_idx].color_write_mask = color_mask;
}

void bfGfxCmdList_setColorBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendOp op)
{
  self->pipeline_state.blending[output_attachment_idx].color_blend_op = op;
}

void bfGfxCmdList_setBlendSrc(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].color_blend_src = factor;
}

void bfGfxCmdList_setBlendDst(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].color_blend_dst = factor;
}

void bfGfxCmdList_setAlphaBlendOp(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendOp op)
{
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_op = op;
}

void bfGfxCmdList_setBlendSrcAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_src = factor;
}

void bfGfxCmdList_setBlendDstAlpha(bfGfxCommandListHandle self, uint32_t output_attachment_idx, bfBlendFactor factor)
{
  self->pipeline_state.blending[output_attachment_idx].alpha_blend_dst = factor;
}

void bfGfxCmdList_setStencilFailOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op)
{
  if (face == BF_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_fail_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_fail_op = op;
  }
}

void bfGfxCmdList_setStencilPassOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op)
{
  if (face == BF_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_pass_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_pass_op = op;
  }
}

void bfGfxCmdList_setStencilDepthFailOp(bfGfxCommandListHandle self, bfStencilFace face, bfStencilOp op)
{
  if (face == BF_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_depth_fail_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_depth_fail_op = op;
  }
}
void bfGfxCmdList_setStencilCompareOp(bfGfxCommandListHandle self, bfStencilFace face, bfCompareOp op)
{
  if (face == BF_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_compare_op = op;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_compare_op = op;
  }
}

void bfGfxCmdList_setStencilCompareMask(bfGfxCommandListHandle self, bfStencilFace face, uint8_t cmp_mask)
{
  if (face == BF_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_compare_mask = cmp_mask;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_compare_mask = cmp_mask;
  }

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK;
}

void bfGfxCmdList_setStencilWriteMask(bfGfxCommandListHandle self, bfStencilFace face, uint8_t write_mask)
{
  if (face == BF_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_write_mask = write_mask;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_write_mask = write_mask;
  }

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK;
}

void bfGfxCmdList_setStencilReference(bfGfxCommandListHandle self, bfStencilFace face, uint8_t ref_mask)
{
  if (face == BF_STENCIL_FACE_FRONT)
  {
    self->pipeline_state.state.stencil_face_front_reference = ref_mask;
  }
  else
  {
    self->pipeline_state.state.stencil_face_back_reference = ref_mask;
  }

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_STENCIL_REFERENCE;
}

void bfGfxCmdList_setDynamicStates(bfGfxCommandListHandle self, uint16_t dynamic_states)
{
  auto& s = self->pipeline_state.state;

  const bfBool32 set_dynamic_viewport           = (dynamic_states & BF_PIPELINE_DYNAMIC_VIEWPORT) != 0;
  const bfBool32 set_dynamic_scissor            = (dynamic_states & BF_PIPELINE_DYNAMIC_SCISSOR) != 0;
  const bfBool32 set_dynamic_line_width         = (dynamic_states & BF_PIPELINE_DYNAMIC_LINE_WIDTH) != 0;
  const bfBool32 set_dynamic_depth_bias         = (dynamic_states & BF_PIPELINE_DYNAMIC_DEPTH_BIAS) != 0;
  const bfBool32 set_dynamic_blend_constants    = (dynamic_states & BF_PIPELINE_DYNAMIC_BLEND_CONSTANTS) != 0;
  const bfBool32 set_dynamic_depth_bounds       = (dynamic_states & BF_PIPELINE_DYNAMIC_DEPTH_BOUNDS) != 0;
  const bfBool32 set_dynamic_stencil_cmp_mask   = (dynamic_states & BF_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK) != 0;
  const bfBool32 set_dynamic_stencil_write_mask = (dynamic_states & BF_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK) != 0;
  const bfBool32 set_dynamic_stencil_reference  = (dynamic_states & BF_PIPELINE_DYNAMIC_STENCIL_REFERENCE) != 0;

  s.dynamic_viewport           = set_dynamic_viewport;
  s.dynamic_scissor            = set_dynamic_scissor;
  s.dynamic_line_width         = set_dynamic_line_width;
  s.dynamic_depth_bias         = set_dynamic_depth_bias;
  s.dynamic_blend_constants    = set_dynamic_blend_constants;
  s.dynamic_depth_bounds       = set_dynamic_depth_bounds;
  s.dynamic_stencil_cmp_mask   = set_dynamic_stencil_cmp_mask;
  s.dynamic_stencil_write_mask = set_dynamic_stencil_write_mask;
  s.dynamic_stencil_reference  = set_dynamic_stencil_reference;

  self->dynamic_state_dirty = dynamic_states;
}

void bfGfxCmdList_setViewport(bfGfxCommandListHandle self, float x, float y, float width, float height, const float depth[2])
{
  static constexpr float k_DefaultDepth[2] = {0.0f, 1.0f};

  if (depth == nullptr)
  {
    depth = k_DefaultDepth;
  }

  auto& vp     = self->pipeline_state.viewport;
  vp.x         = x;
  vp.y         = y;
  vp.width     = width;
  vp.height    = height;
  vp.min_depth = depth[0];
  vp.max_depth = depth[1];

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_VIEWPORT;
}

void bfGfxCmdList_setScissorRect(bfGfxCommandListHandle self, int32_t x, int32_t y, uint32_t width, uint32_t height)
{
  auto& s = self->pipeline_state.scissor_rect;

  s.x      = x;
  s.y      = y;
  s.width  = width;
  s.height = height;

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_SCISSOR;
}

void bfGfxCmdList_setBlendConstants(bfGfxCommandListHandle self, const float constants[4])
{
  memcpy(self->pipeline_state.blend_constants, constants, sizeof(self->pipeline_state.blend_constants));

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_BLEND_CONSTANTS;
}

void bfGfxCmdList_setLineWidth(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.line_width = value;

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_LINE_WIDTH;
}

void bfGfxCmdList_setDepthClampEnabled(bfGfxCommandListHandle self, bfBool32 value)
{
  self->pipeline_state.state.do_depth_clamp = value;
}

void bfGfxCmdList_setDepthBoundsTestEnabled(bfGfxCommandListHandle self, bfBool32 value)
{
  self->pipeline_state.state.do_depth_bounds_test = value;
}

void bfGfxCmdList_setDepthBounds(bfGfxCommandListHandle self, float min, float max)
{
  self->pipeline_state.depth.min_bound = min;
  self->pipeline_state.depth.max_bound = max;

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_DEPTH_BOUNDS;
}

void bfGfxCmdList_setDepthBiasConstantFactor(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_constant_factor = value;
  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_DEPTH_BIAS;
}

void bfGfxCmdList_setDepthBiasClamp(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_clamp = value;
  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_DEPTH_BIAS;
}

void bfGfxCmdList_setDepthBiasSlopeFactor(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.depth.bias_slope_factor = value;
  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_DEPTH_BIAS;
}

void bfGfxCmdList_setMinSampleShading(bfGfxCommandListHandle self, float value)
{
  self->pipeline_state.min_sample_shading = value;
}

void bfGfxCmdList_setSampleMask(bfGfxCommandListHandle self, uint32_t sample_mask)
{
  self->pipeline_state.sample_mask = sample_mask;
}

void bfGfxCmdList_bindDrawCallPipeline(bfGfxCommandListHandle self, const bfDrawCallPipeline* pipeline_state)
{
  const std::uint64_t old_subpass_idx = self->pipeline_state.state.subpass_index;

  self->pipeline_state.state               = pipeline_state->state;
  self->pipeline_state.state.subpass_index = old_subpass_idx;
  self->pipeline_state.line_width          = pipeline_state->line_width;
  self->pipeline_state.program             = pipeline_state->program;
  self->pipeline_state.vertex_layout       = pipeline_state->vertex_layout;
  std::memcpy(self->pipeline_state.blend_constants, pipeline_state->blend_constants, sizeof(pipeline_state->blend_constants));
  std::memcpy(self->pipeline_state.blending, pipeline_state->blending, sizeof(pipeline_state->blending));  // TODO(SR): This can be optimized to copy less.

  self->dynamic_state_dirty |= BF_PIPELINE_DYNAMIC_LINE_WIDTH |
                               BF_PIPELINE_DYNAMIC_BLEND_CONSTANTS |
                               BF_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK |
                               BF_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK |
                               BF_PIPELINE_DYNAMIC_STENCIL_REFERENCE;
}

void bfGfxCmdList_bindVertexDesc(bfGfxCommandListHandle self, bfVertexLayoutSetHandle vertex_set_layout)
{
  self->pipeline_state.vertex_layout = vertex_set_layout;
}

void bfGfxCmdList_bindVertexBuffers(bfGfxCommandListHandle self, uint32_t first_binding, bfBufferHandle* buffers, uint32_t num_buffers, const uint64_t* offsets)
{
  assert(num_buffers < k_bfGfxMaxBufferBindings);

  VkBuffer binded_buffers[k_bfGfxMaxBufferBindings];
  uint64_t binded_offsets[k_bfGfxMaxBufferBindings];

  for (uint32_t i = 0; i < num_buffers; ++i)
  {
    binded_buffers[i] = buffers[i]->handle;
    binded_offsets[i] = offsets[i] + buffers[i]->alloc_info.offset;
  }

  vkCmdBindVertexBuffers(
   self->handle,
   first_binding,
   num_buffers,
   binded_buffers,
   binded_offsets);
}

void bfGfxCmdList_bindIndexBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, uint64_t offset, bfGfxIndexType idx_type)
{
  vkCmdBindIndexBuffer(self->handle, buffer->handle, offset, bfVkConvertIndexType(idx_type));
}

void bfGfxCmdList_bindProgram(bfGfxCommandListHandle self, bfShaderProgramHandle shader)
{
  self->pipeline_state.program = shader;
}

void bfGfxCmdList_bindDescriptorSets(bfGfxCommandListHandle self, uint32_t binding, bfDescriptorSetHandle* desc_sets, uint32_t num_desc_sets)
{
  const VkPipelineBindPoint   bind_point = self->pipeline_state.renderpass ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
  const bfShaderProgramHandle program    = self->pipeline_state.program;

  assert(bind_point == VK_PIPELINE_BIND_POINT_GRAPHICS && "Compute not supported yet.");
  assert(binding + num_desc_sets <= program->num_desc_set_layouts);
  assert(num_desc_sets <= k_bfGfxDescriptorSets);

  VkDescriptorSet sets[k_bfGfxDescriptorSets];

  for (uint32_t i = 0; i < num_desc_sets; ++i)
  {
    sets[i] = desc_sets[i]->handle;
  }

  vkCmdBindDescriptorSets(
   self->handle,
   bind_point,
   program->layout,
   binding,
   num_desc_sets,
   sets,
   0,
   nullptr);
}

void bfGfxCmdList_bindDescriptorSet(bfGfxCommandListHandle self, uint32_t set_index, const bfDescriptorSetInfo* desc_set_info)
{
  bfShaderProgramHandle program = self->pipeline_state.program;

  assert(set_index < program->num_desc_set_layouts);

  const std::uint64_t   hash_code = bf::vk::hash(program->desc_set_layout_infos[set_index], desc_set_info);
  bfDescriptorSetHandle desc_set  = self->parent->cache_descriptor_set.find(hash_code, *desc_set_info);

  if (!desc_set)
  {
    desc_set = bfShaderProgram_createDescriptorSet(program, set_index);

    for (uint32_t i = 0; i < desc_set_info->num_bindings; ++i)
    {
      const bfDescriptorElementInfo* binding_info = &desc_set_info->bindings[i];

      switch (binding_info->type)
      {
        case BF_DESCRIPTOR_ELEMENT_TEXTURE:
          bfDescriptorSet_setCombinedSamplerTextures(
           desc_set,
           binding_info->binding,
           binding_info->array_element_start,
           (bfTextureHandle*)binding_info->handles,  // NOLINT(clang-diagnostic-cast-qual)
           binding_info->num_handles);
          break;
        case BF_DESCRIPTOR_ELEMENT_BUFFER:
          bfDescriptorSet_setUniformBuffers(
           desc_set,
           binding_info->binding,
           binding_info->offsets,
           binding_info->sizes,
           (bfBufferHandle*)binding_info->handles,  // NOLINT(clang-diagnostic-cast-qual)
           binding_info->num_handles);
          break;
        case BF_DESCRIPTOR_ELEMENT_BUFFER_VIEW:
        case BF_DESCRIPTOR_ELEMENT_DYNAMIC_BUFFER:
        case BF_DESCRIPTOR_ELEMENT_INPUT_ATTACHMENT:
        default:
          assert(!"Not supported yet.");
          break;
      }
    }

    bfDescriptorSet_flushWrites(desc_set);

    self->parent->cache_descriptor_set.insert(hash_code, desc_set, *desc_set_info);
    AddCachedResource(self->parent, &desc_set->super, hash_code);
  }

  bfGfxCmdList_bindDescriptorSets(self, set_index, &desc_set, 1);
  UpdateResourceFrame(&desc_set->super);
}

static void flushPipeline(bfGfxCommandListHandle self)
{
  const uint64_t hash_code = bf::vk::hash(0x0, &self->pipeline_state);

  bfPipelineHandle pl = self->parent->cache_pipeline.find(hash_code, self->pipeline_state);

  if (!pl)
  {
    pl = xxx_AllocGfxObject<bfPipeline>(BF_GFX_OBJECT_PIPELINE, &g_Ctx->obj_man);

    auto& state   = self->pipeline_state;
    auto& ss      = state.state;
    auto& program = state.program;

    VkPipelineShaderStageCreateInfo shader_stages[BF_SHADER_TYPE_MAX];

    for (size_t i = 0; i < program->modules.size; ++i)
    {
      bfShaderModuleHandle shader_module = program->modules.elements[i];
      auto&                shader_stage  = shader_stages[i];
      shader_stage.sType                 = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shader_stage.pNext                 = nullptr;
      shader_stage.flags                 = 0x0;
      shader_stage.stage                 = bfVkConvertShaderType(shader_module->type);
      shader_stage.module                = shader_module->handle;
      shader_stage.pName                 = shader_module->entry_point;
      shader_stage.pSpecializationInfo   = nullptr;  // https://www.jeremyong.com/c++/vulkan/graphics/rendering/2018/04/20/putting-cpp-to-work-making-abstractions-for-vulkan-specialization-info/
    }

    VkPipelineVertexInputStateCreateInfo vertex_input;
    vertex_input.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input.pNext                           = nullptr;
    vertex_input.flags                           = 0x0;
    vertex_input.vertexBindingDescriptionCount   = state.vertex_layout->num_buffer_bindings;
    vertex_input.pVertexBindingDescriptions      = state.vertex_layout->buffer_bindings;
    vertex_input.vertexAttributeDescriptionCount = state.vertex_layout->num_attrib_bindings;
    vertex_input.pVertexAttributeDescriptions    = state.vertex_layout->attrib_bindings;

    VkPipelineInputAssemblyStateCreateInfo input_asm;
    input_asm.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_asm.pNext                  = nullptr;
    input_asm.flags                  = 0x0;
    input_asm.topology               = bfVkConvertTopology((bfDrawMode)state.state.draw_mode);
    input_asm.primitiveRestartEnable = state.state.do_primitive_restart;

    VkPipelineTessellationStateCreateInfo tess;
    tess.sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tess.pNext              = nullptr;
    tess.flags              = 0x0;
    tess.patchControlPoints = 0;  // TODO: Figure this out.

    // https://erkaman.github.io/posts/tess_opt.html
    // https://computergraphics.stackexchange.com/questions/7955/why-are-tessellation-shaders-disliked

    VkPipelineViewportStateCreateInfo viewport;
    VkViewport                        viewports[1];
    VkRect2D                          scissor_rects[1];

    viewports[0]           = bfVkConvertViewport(&state.viewport);
    scissor_rects[0]       = bfVkConvertScissorRect(&state.scissor_rect);
    viewport.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport.pNext         = nullptr;
    viewport.flags         = 0x0;
    viewport.viewportCount = uint32_t(bfCArraySize(viewports));
    viewport.pViewports    = viewports;
    viewport.scissorCount  = uint32_t(bfCArraySize(scissor_rects));
    viewport.pScissors     = scissor_rects;

    VkPipelineRasterizationStateCreateInfo rasterization;
    rasterization.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization.pNext                   = nullptr;
    rasterization.flags                   = 0x0;
    rasterization.depthClampEnable        = state.state.do_depth_clamp;
    rasterization.rasterizerDiscardEnable = state.state.do_rasterizer_discard;
    rasterization.polygonMode             = bfVkConvertPolygonMode((bfPolygonFillMode)state.state.fill_mode);
    rasterization.cullMode                = bfVkConvertCullModeFlags(state.state.cull_face);
    rasterization.frontFace               = bfVkConvertFrontFace((bfFrontFace)state.state.front_face);
    rasterization.depthBiasEnable         = state.state.do_depth_bias;
    rasterization.depthBiasConstantFactor = state.depth.bias_constant_factor;
    rasterization.depthBiasClamp          = state.depth.bias_clamp;
    rasterization.depthBiasSlopeFactor    = state.depth.bias_slope_factor;
    rasterization.lineWidth               = state.line_width;

    VkPipelineMultisampleStateCreateInfo multisample;
    multisample.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.pNext                 = nullptr;
    multisample.flags                 = 0x0;
    multisample.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisample.sampleShadingEnable   = state.state.do_sample_shading;
    multisample.minSampleShading      = state.min_sample_shading;
    multisample.pSampleMask           = &state.sample_mask;
    multisample.alphaToCoverageEnable = state.state.do_alpha_to_coverage;
    multisample.alphaToOneEnable      = state.state.do_alpha_to_one;

    const auto ConvertStencilOpState = [](VkStencilOpState& out, uint64_t fail, uint64_t pass, uint64_t depth_fail, uint64_t cmp_op, uint32_t cmp_mask, uint32_t write_mask, uint32_t ref) {
      out.failOp      = bfVkConvertStencilOp((bfStencilOp)fail);
      out.passOp      = bfVkConvertStencilOp((bfStencilOp)pass);
      out.depthFailOp = bfVkConvertStencilOp((bfStencilOp)depth_fail);
      out.compareOp   = bfVkConvertCompareOp((bfCompareOp)cmp_op);
      out.compareMask = cmp_mask;
      out.writeMask   = write_mask;
      out.reference   = ref;
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil;
    depth_stencil.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.pNext                 = nullptr;
    depth_stencil.flags                 = 0x0;
    depth_stencil.depthTestEnable       = state.state.do_depth_test;
    depth_stencil.depthWriteEnable      = state.state.do_depth_write;
    depth_stencil.depthCompareOp        = bfVkConvertCompareOp((bfCompareOp)state.state.depth_test_op);
    depth_stencil.depthBoundsTestEnable = state.state.do_depth_bounds_test;
    depth_stencil.stencilTestEnable     = state.state.do_stencil_test;

    ConvertStencilOpState(
     depth_stencil.front,
     ss.stencil_face_front_fail_op,
     ss.stencil_face_front_pass_op,
     ss.stencil_face_front_depth_fail_op,
     ss.stencil_face_front_compare_op,
     ss.stencil_face_front_compare_mask,
     ss.stencil_face_front_write_mask,
     ss.stencil_face_front_reference);

    ConvertStencilOpState(
     depth_stencil.back,
     ss.stencil_face_back_fail_op,
     ss.stencil_face_back_pass_op,
     ss.stencil_face_back_depth_fail_op,
     ss.stencil_face_back_compare_op,
     ss.stencil_face_back_compare_mask,
     ss.stencil_face_back_write_mask,
     ss.stencil_face_back_reference);

    depth_stencil.minDepthBounds = state.depth.min_bound;
    depth_stencil.maxDepthBounds = state.depth.max_bound;

    VkPipelineColorBlendStateCreateInfo color_blend;
    VkPipelineColorBlendAttachmentState color_blend_states[k_bfGfxMaxAttachments];

    const uint32_t num_color_attachments = self->pipeline_state.renderpass->info.subpasses[state.state.subpass_index].num_out_attachment_refs;

    for (uint32_t i = 0; i < num_color_attachments; ++i)
    {
      bfFramebufferBlending&               blend     = state.blending[i];
      VkPipelineColorBlendAttachmentState& clr_state = color_blend_states[i];

      clr_state.blendEnable = blend.color_blend_src != BF_BLEND_FACTOR_NONE && blend.color_blend_dst != BF_BLEND_FACTOR_NONE;

      if (clr_state.blendEnable)
      {
        clr_state.srcColorBlendFactor = bfVkConvertBlendFactor((bfBlendFactor)blend.color_blend_src);
        clr_state.dstColorBlendFactor = bfVkConvertBlendFactor((bfBlendFactor)blend.color_blend_dst);
        clr_state.colorBlendOp        = bfVkConvertBlendOp((bfBlendOp)blend.color_blend_op);
        clr_state.srcAlphaBlendFactor = bfVkConvertBlendFactor((bfBlendFactor)blend.alpha_blend_src);
        clr_state.dstAlphaBlendFactor = bfVkConvertBlendFactor((bfBlendFactor)blend.alpha_blend_dst);
        clr_state.alphaBlendOp        = bfVkConvertBlendOp((bfBlendOp)blend.alpha_blend_op);
      }
      else
      {
        clr_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        clr_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
        clr_state.colorBlendOp        = VK_BLEND_OP_ADD;
        clr_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        clr_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        clr_state.alphaBlendOp        = VK_BLEND_OP_ADD;
      }

      clr_state.colorWriteMask = bfVkConvertColorMask(blend.color_write_mask);
    }

    color_blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend.pNext           = nullptr;
    color_blend.flags           = 0x0;
    color_blend.logicOpEnable   = ss.do_logic_op;
    color_blend.logicOp         = bfVkConvertLogicOp((bfLogicOp)ss.logic_op);
    color_blend.attachmentCount = num_color_attachments;
    color_blend.pAttachments    = color_blend_states;
    memcpy(color_blend.blendConstants, state.blend_constants, sizeof(state.blend_constants));

    VkPipelineDynamicStateCreateInfo dynamic_state;
    VkDynamicState                   dynamic_state_storage[9];
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext             = nullptr;
    dynamic_state.flags             = 0x0;
    dynamic_state.dynamicStateCount = 0;
    dynamic_state.pDynamicStates    = dynamic_state_storage;

    auto& s = state.state;

    const auto addDynamicState = [&dynamic_state](VkDynamicState* states, uint64_t flag, VkDynamicState vk_state) {
      if (flag)
      {
        states[dynamic_state.dynamicStateCount++] = vk_state;
      }
    };

    addDynamicState(dynamic_state_storage, s.dynamic_viewport, VK_DYNAMIC_STATE_VIEWPORT);
    addDynamicState(dynamic_state_storage, s.dynamic_scissor, VK_DYNAMIC_STATE_SCISSOR);
    addDynamicState(dynamic_state_storage, s.dynamic_line_width, VK_DYNAMIC_STATE_LINE_WIDTH);
    addDynamicState(dynamic_state_storage, s.dynamic_depth_bias, VK_DYNAMIC_STATE_DEPTH_BIAS);
    addDynamicState(dynamic_state_storage, s.dynamic_blend_constants, VK_DYNAMIC_STATE_BLEND_CONSTANTS);
    addDynamicState(dynamic_state_storage, s.dynamic_depth_bounds, VK_DYNAMIC_STATE_DEPTH_BOUNDS);
    addDynamicState(dynamic_state_storage, s.dynamic_stencil_cmp_mask, VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
    addDynamicState(dynamic_state_storage, s.dynamic_stencil_write_mask, VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
    addDynamicState(dynamic_state_storage, s.dynamic_stencil_reference, VK_DYNAMIC_STATE_STENCIL_REFERENCE);

    // TODO(SR): Look into pipeline derivatives?
    //   @PipelineDerivative

    VkGraphicsPipelineCreateInfo pl_create_info;
    pl_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pl_create_info.pNext               = nullptr;
    pl_create_info.flags               = 0x0;  // @PipelineDerivative
    pl_create_info.stageCount          = program->modules.size;
    pl_create_info.pStages             = shader_stages;
    pl_create_info.pVertexInputState   = &vertex_input;
    pl_create_info.pInputAssemblyState = &input_asm;
    pl_create_info.pTessellationState  = &tess;
    pl_create_info.pViewportState      = &viewport;
    pl_create_info.pRasterizationState = &rasterization;
    pl_create_info.pMultisampleState   = &multisample;
    pl_create_info.pDepthStencilState  = &depth_stencil;
    pl_create_info.pColorBlendState    = &color_blend;
    pl_create_info.pDynamicState       = &dynamic_state;
    pl_create_info.layout              = program->layout;
    pl_create_info.renderPass          = self->pipeline_state.renderpass->handle;
    pl_create_info.subpass             = state.state.subpass_index;
    pl_create_info.basePipelineHandle  = VK_NULL_HANDLE;  // @PipelineDerivative
    pl_create_info.basePipelineIndex   = -1;              // @PipelineDerivative

    // TODO(Shareef): Look into Pipeline caches?
    const VkResult err = vkCreateGraphicsPipelines(
     self->parent->handle,
     VK_NULL_HANDLE,
     1,
     &pl_create_info,
     CUSTOM_ALLOC,
     &pl->handle);
    assert(err == VK_SUCCESS);

    self->parent->cache_pipeline.insert(hash_code, pl, self->pipeline_state);
    AddCachedResource(self->parent, &pl->super, hash_code);
  }

  if (pl != self->pipeline)
  {
    vkCmdBindPipeline(self->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->handle);
    self->dynamic_state_dirty = 0xFFFF;
    self->pipeline            = pl;
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_VIEWPORT && self->pipeline_state.state.dynamic_viewport)
  {
    VkViewport viewports = bfVkConvertViewport(&self->pipeline_state.viewport);
    vkCmdSetViewport(self->handle, 0, 1, &viewports);
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_SCISSOR && self->pipeline_state.state.dynamic_scissor)
  {
    VkRect2D scissors = bfVkConvertScissorRect(&self->pipeline_state.scissor_rect);
    vkCmdSetScissor(self->handle, 0, 1, &scissors);
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_LINE_WIDTH && self->pipeline_state.state.dynamic_line_width)
  {
    vkCmdSetLineWidth(self->handle, self->pipeline_state.line_width);
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_DEPTH_BIAS && self->pipeline_state.state.dynamic_depth_bias)
  {
    auto& depth = self->pipeline_state.depth;

    vkCmdSetDepthBias(self->handle, depth.bias_constant_factor, depth.bias_clamp, depth.bias_slope_factor);
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_BLEND_CONSTANTS && self->pipeline_state.state.dynamic_blend_constants)
  {
    vkCmdSetBlendConstants(self->handle, self->pipeline_state.blend_constants);
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_DEPTH_BOUNDS && self->pipeline_state.state.dynamic_depth_bounds)
  {
    auto& depth = self->pipeline_state.depth;

    vkCmdSetDepthBounds(self->handle, depth.min_bound, depth.max_bound);
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK && self->pipeline_state.state.dynamic_stencil_cmp_mask)
  {
    auto& ss = self->pipeline_state.state;

    if (ss.stencil_face_front_compare_mask == ss.stencil_face_back_compare_mask)
    {
      vkCmdSetStencilCompareMask(self->handle, VK_STENCIL_FRONT_AND_BACK, ss.stencil_face_front_compare_mask);
    }
    else
    {
      vkCmdSetStencilCompareMask(self->handle, VK_STENCIL_FACE_FRONT_BIT, ss.stencil_face_front_compare_mask);
      vkCmdSetStencilCompareMask(self->handle, VK_STENCIL_FACE_BACK_BIT, ss.stencil_face_back_compare_mask);
    }
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK && self->pipeline_state.state.dynamic_stencil_write_mask)
  {
    auto& ss = self->pipeline_state.state;

    if (ss.stencil_face_front_write_mask == ss.stencil_face_back_write_mask)
    {
      vkCmdSetStencilWriteMask(self->handle, VK_STENCIL_FRONT_AND_BACK, ss.stencil_face_front_write_mask);
    }
    else
    {
      vkCmdSetStencilWriteMask(self->handle, VK_STENCIL_FACE_FRONT_BIT, ss.stencil_face_front_write_mask);
      vkCmdSetStencilWriteMask(self->handle, VK_STENCIL_FACE_BACK_BIT, ss.stencil_face_back_write_mask);
    }
  }

  if (self->dynamic_state_dirty & BF_PIPELINE_DYNAMIC_STENCIL_REFERENCE && self->pipeline_state.state.dynamic_stencil_reference)
  {
    auto& ss = self->pipeline_state.state;

    if (ss.stencil_face_front_reference == ss.stencil_face_back_reference)
    {
      vkCmdSetStencilReference(self->handle, VK_STENCIL_FRONT_AND_BACK, ss.stencil_face_front_reference);
    }
    else
    {
      vkCmdSetStencilReference(self->handle, VK_STENCIL_FACE_FRONT_BIT, ss.stencil_face_front_reference);
      vkCmdSetStencilReference(self->handle, VK_STENCIL_FACE_BACK_BIT, ss.stencil_face_back_reference);
    }
  }

  self->dynamic_state_dirty = 0x0;

  UpdateResourceFrame(&pl->super);
}

void bfGfxCmdList_draw(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices)
{
  bfGfxCmdList_drawInstanced(self, first_vertex, num_vertices, 0, 1);
}

void bfGfxCmdList_drawInstanced(bfGfxCommandListHandle self, uint32_t first_vertex, uint32_t num_vertices, uint32_t first_instance, uint32_t num_instances)
{
  flushPipeline(self);
  vkCmdDraw(self->handle, num_vertices, num_instances, first_vertex, first_instance);
}

void bfGfxCmdList_drawIndexed(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset)
{
  bfGfxCmdList_drawIndexedInstanced(self, num_indices, index_offset, vertex_offset, 0, 1);
}

void bfGfxCmdList_drawIndexedInstanced(bfGfxCommandListHandle self, uint32_t num_indices, uint32_t index_offset, int32_t vertex_offset, uint32_t first_instance, uint32_t num_instances)
{
  flushPipeline(self);
  vkCmdDrawIndexed(self->handle, num_indices, num_instances, index_offset, vertex_offset, first_instance);
}

void bfGfxCmdList_executeSubCommands(bfGfxCommandListHandle self, bfGfxCommandListHandle* commands, uint32_t num_commands)
{
  assert(!"Not implemented");
}

void bfGfxCmdList_endRenderpass(bfGfxCommandListHandle self)
{
  auto& render_pass_info = self->pipeline_state.renderpass->info;

  for (std::uint32_t i = 0; i < render_pass_info.num_attachments; ++i)
  {
    render_pass_info.attachments[i].texture->tex_layout = render_pass_info.attachments[i].final_layout;
  }

  vkCmdEndRenderPass(self->handle);
#if 0
  VkMemoryBarrier memoryBarrier = {
   VK_STRUCTURE_TYPE_MEMORY_BARRIER,
   nullptr,
   VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
    VK_ACCESS_INDEX_READ_BIT |
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
    VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
    VK_ACCESS_SHADER_READ_BIT |
    VK_ACCESS_SHADER_WRITE_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_TRANSFER_READ_BIT |
    VK_ACCESS_TRANSFER_WRITE_BIT |
    VK_ACCESS_HOST_READ_BIT |
    VK_ACCESS_HOST_WRITE_BIT,
   VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
    VK_ACCESS_INDEX_READ_BIT |
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
    VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
    VK_ACCESS_SHADER_READ_BIT |
    VK_ACCESS_SHADER_WRITE_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_TRANSFER_READ_BIT |
    VK_ACCESS_TRANSFER_WRITE_BIT |
    VK_ACCESS_HOST_READ_BIT |
    VK_ACCESS_HOST_WRITE_BIT};

  vkCmdPipelineBarrier(
   self->handle,
   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // srcStageMask
   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,  // dstStageMask
   0x0,
   1,               // memoryBarrierCount
   &memoryBarrier,  // pMemoryBarriers
   0,
   nullptr,
   0,
   nullptr);
#endif
}

void bfGfxCmdList_end(bfGfxCommandListHandle self)
{
  const VkResult err = vkEndCommandBuffer(self->handle);

  if (err)
  {
    assert(false);
  }
}

void bfGfxCmdList_updateBuffer(bfGfxCommandListHandle self, bfBufferHandle buffer, bfBufferSize offset, bfBufferSize size, const void* data)
{
  vkCmdUpdateBuffer(self->handle, buffer->handle, offset, size, data);
}

extern void gfxDestroySwapchain(bfWindowSurface& window);

#include <stdio.h>

void bfGfxCmdList_submit(bfGfxCommandListHandle self)
{
  const VkFence       command_fence = self->fence;
  bfWindowSurface&    window        = *self->window;
  const std::uint32_t frame_index   = bfGfxGetFrameInfo().frame_index;

  VkSemaphore          wait_semaphores[]   = {window.is_image_available[frame_index]};
  VkPipelineStageFlags wait_stages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};  // What to wait for, like: DO NOT WRITE TO COLOR UNTIL IMAGE IS AVALIBALLE.
  VkSemaphore          signal_semaphores[] = {window.is_render_done[frame_index]};

  VkSubmitInfo submit_info;
  submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext                = NULL;
  submit_info.waitSemaphoreCount   = uint32_t(bfCArraySize(wait_semaphores));
  submit_info.pWaitSemaphores      = wait_semaphores;
  submit_info.pWaitDstStageMask    = wait_stages;
  submit_info.commandBufferCount   = 1;
  submit_info.pCommandBuffers      = &self->handle;
  submit_info.signalSemaphoreCount = uint32_t(bfCArraySize(signal_semaphores));
  submit_info.pSignalSemaphores    = signal_semaphores;

  vkResetFences(self->parent->handle, 1, &command_fence);

  VkResult err = vkQueueSubmit(self->parent->queues[BF_GFX_QUEUE_GRAPHICS], 1, &submit_info, command_fence);

  assert(err == VK_SUCCESS && "bfGfxCmdList_submit: failed to submit the graphics queue");

  VkPresentInfoKHR present_info;
  present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.pNext              = nullptr;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;
  present_info.swapchainCount     = 1;
  present_info.pSwapchains        = &window.swapchain.handle;
  present_info.pImageIndices      = &window.image_index;
  present_info.pResults           = nullptr;

  err = vkQueuePresentKHR(self->parent->queues[BF_GFX_QUEUE_PRESENT], &present_info);

  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
  {
    gfxDestroySwapchain(window);
  }
  else
  {
    assert(err == VK_SUCCESS && "GfxContext_submitFrame: failed to present graphics queue");
  }

  window.current_cmd_list = nullptr;
}

// TODO(SR): Make a new file
#include "bf/bf_hash.hpp"

namespace bf::vk
{
  using namespace gfx_hash;

  std::uint64_t hash(std::uint64_t self, const bfPipelineCache* pipeline)
  {
    const auto    num_attachments = pipeline->renderpass->info.subpasses[pipeline->state.subpass_index].num_out_attachment_refs;
    std::uint64_t state_bits[2];

    static_assert(sizeof(state_bits) == sizeof(pipeline->state), "Needs to be same size.");

    std::memcpy(state_bits, &pipeline->state, sizeof(state_bits));

    state_bits[0] &= bfPipelineCache_state0Mask(&pipeline->state);
    state_bits[1] &= bfPipelineCache_state1Mask(&pipeline->state);

    for (std::uint64_t state_bit : state_bits)
    {
      self = hash::addU64(self, state_bit);
    }

    if (!pipeline->state.dynamic_viewport)
    {
      gfx_hash::hash(self, pipeline->viewport);
    }

    if (!pipeline->state.dynamic_scissor)
    {
      gfx_hash::hash(self, pipeline->scissor_rect);
    }

    if (!pipeline->state.dynamic_blend_constants)
    {
      for (float blend_constant : pipeline->blend_constants)
      {
        self = hash::addF32(self, blend_constant);
      }
    }

    if (!pipeline->state.dynamic_line_width)
    {
      self = hash::addF32(self, pipeline->line_width);
    }

    gfx_hash::hash(self, pipeline->depth, pipeline->state);
    self = hash::addF32(self, pipeline->min_sample_shading);
    self = hash::addU64(self, pipeline->sample_mask);
    self = hash::addU32(self, pipeline->state.subpass_index);
    self = hash::addU32(self, num_attachments);

    for (std::uint32_t i = 0; i < num_attachments; ++i)
    {
      gfx_hash::hash(self, pipeline->blending[i]);
    }

    self = hash::addPointer(self, pipeline->program);
    self = hash::addPointer(self, pipeline->renderpass);
    self = hash::addPointer(self, pipeline->vertex_layout);

    return self;
  }

  std::uint64_t hash(std::uint64_t self, bfTextureHandle* attachments, std::size_t num_attachments)
  {
    if (num_attachments)
    {
      self = hash::addS32(self, attachments[0]->image_width);
      self = hash::addS32(self, attachments[0]->image_height);
    }

    for (std::size_t i = 0; i < num_attachments; ++i)
    {
      self = hash::addU32(self, attachments[i]->super.id);
    }

    return self;
  }

  std::uint64_t hash(const bfDescriptorSetLayoutInfo& parent, const bfDescriptorSetInfo* desc_set_info)
  {
    std::uint64_t self = desc_set_info->num_bindings;

    for (uint32_t i = 0; i < desc_set_info->num_bindings; ++i)
    {
      const bfDescriptorElementInfo* binding = &desc_set_info->bindings[i];

      self = hash::addU32(self, binding->binding);
      self = hash::addU32(self, binding->array_element_start);
      self = hash::addU32(self, binding->num_handles);
      self = hash::addU32(self, parent.layout_bindings[i].stageFlags);

      for (uint32_t j = 0; j < binding->num_handles; ++j)
      {
        self = hash::addU32(self, binding->handles[j]->id);

        if (binding->type == BF_DESCRIPTOR_ELEMENT_BUFFER)
        {
          self = hash::addU64(self, binding->offsets[j]);
          self = hash::addU64(self, binding->sizes[j]);
        }
      }
    }

    return self;
  }
}  // namespace bf::vk

static constexpr std::uint64_t k_FrontStencilCmpStateMask       = 0b0000000000000000011111111000000000000000000000000000000000000000;
static constexpr std::uint64_t k_FrontStencilWriteStateMask     = 0b0000000001111111100000000000000000000000000000000000000000000000;
static constexpr std::uint64_t k_FrontStencilReferenceStateMask = 0b0111111110000000000000000000000000000000000000000000000000000000;
static constexpr std::uint64_t k_BackStencilCmpStateMask        = 0b0000000000000000000000000000000000000000000000111111110000000000;
static constexpr std::uint64_t k_BackStencilWriteStateMask      = 0b0000000000000000000000000000000000000011111111000000000000000000;
static constexpr std::uint64_t k_BackStencilReferenceStateMask  = 0b0000000000000000000000000000001111111100000000000000000000000000;

uint64_t bfPipelineCache_state0Mask(const bfPipelineState* self)
{
  uint64_t result = 0xFFFFFFFFFFFFFFFF;

  if (self->dynamic_stencil_cmp_mask)
  {
    result &= ~k_FrontStencilCmpStateMask;
  }

  if (self->dynamic_stencil_write_mask)
  {
    result &= ~k_FrontStencilWriteStateMask;
  }

  if (self->dynamic_stencil_reference)
  {
    result &= ~k_FrontStencilReferenceStateMask;
  }

  return result;
}

uint64_t bfPipelineCache_state1Mask(const bfPipelineState* self)
{
  uint64_t result = 0xFFFFFFFFFFFFFFFF;

  if (self->dynamic_stencil_cmp_mask)
  {
    result &= ~k_BackStencilCmpStateMask;
  }

  if (self->dynamic_stencil_write_mask)
  {
    result &= ~k_BackStencilWriteStateMask;
  }

  if (self->dynamic_stencil_reference)
  {
    result &= ~k_BackStencilReferenceStateMask;
  }

  return result;
}

#include "vulkan/bf_vulkan_conversions.h"

#include <cassert> /* assert */

#define CUSTOM_ALLOC nullptr

bfRenderpassHandle bfGfxDevice_newRenderpass(bfGfxDeviceHandle self, const bfRenderpassCreateParams* params)
{
  bfRenderpassHandle renderpass = xxx_AllocGfxObject<bfRenderpass>(BF_GFX_OBJECT_RENDERPASS, &g_Ctx->obj_man);

  // VK_ATTACHMENT_UNUSED

  renderpass->info                        = *params;
  const auto              num_attachments = params->num_attachments;
  VkAttachmentDescription attachments[k_bfGfxMaxAttachments];
  const uint32_t          num_subpasses = params->num_subpasses;
  VkSubpassDescription    subpasses[k_bfGfxMaxSubpasses];
  const auto              num_dependencies = params->num_dependencies;
  VkSubpassDependency     dependencies[k_bfGfxMaxRenderpassDependencies];
  VkAttachmentReference   inputs[k_bfGfxMaxSubpasses][k_bfGfxMaxAttachments];
  VkAttachmentReference   outputs[k_bfGfxMaxSubpasses][k_bfGfxMaxAttachments];
  VkAttachmentReference   depth_atts[k_bfGfxMaxSubpasses];

  const auto bitsToLoadOp = [](uint32_t i, bfLoadStoreFlags load_ops, bfLoadStoreFlags clear_ops) -> VkAttachmentLoadOp {
    if (bfBit(i) & clear_ops)
    {
      // TODO(Shareef): Assert for the load bit NOT to be set.
      return VK_ATTACHMENT_LOAD_OP_CLEAR;
    }

    if (bfBit(i) & load_ops)
    {
      // TODO(Shareef): Assert for the clear bit NOT to be set.
      return VK_ATTACHMENT_LOAD_OP_LOAD;
    }

    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  };

  const auto bitsToStoreOp = [](uint32_t i, bfLoadStoreFlags store_ops) -> VkAttachmentStoreOp {
    if (bfBit(i) & store_ops)
    {
      return VK_ATTACHMENT_STORE_OP_STORE;
    }

    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  };

  // For Compat:
  //  DO NOT NEED TO MATCH:
  //
  //  - initialLayout, finalLayout
  //  - loadOp, storeOp, stencilLoadOp, stencilStoreOp,
  //  - VkAttachmentReference::layout.
  //
  //  Everything else must match.
  //  AttachmentRef: (format, sample count)
  //  AttachmentRef[]: All AttachmentRefs, pretty must needs to be same length.
  //
  //  Special Case:
  //
  //  - If the Renderpass only has one subpass then:
  //      "the resolve attachment reference and depth/stencil resolve mode compatibility requirements"
  //
  // [https://renderdoc.org/vkspec_chunked/chap9.html#renderpass-compatibility]

  const auto bfAttToVkAtt = [](const bfAttachmentRefCache* in) -> VkAttachmentReference {
    VkAttachmentReference out;
    out.attachment = in->attachment_index;
    out.layout     = bfVkConvertImgLayout(in->layout);
    return out;
  };

  for (uint32_t i = 0; i < num_attachments; ++i)
  {
    VkAttachmentDescription* const att      = attachments + i;
    const bfAttachmentInfo* const  att_info = params->attachments + i;

    att->flags          = att_info->may_alias ? VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT : 0x0;
    att->format         = att_info->texture->tex_format;
    att->samples        = bfVkConvertSampleCount(att_info->texture->tex_samples);
    att->loadOp         = bitsToLoadOp(i, params->load_ops, params->clear_ops);
    att->storeOp        = bitsToStoreOp(i, params->store_ops);
    att->stencilLoadOp  = bitsToLoadOp(i, params->stencil_load_ops, params->stencil_clear_ops);
    att->stencilStoreOp = bitsToStoreOp(i, params->stencil_store_ops);
    att->initialLayout  = bfVkConvertImgLayout(att_info->texture->tex_layout);
    att->finalLayout    = bfVkConvertImgLayout(att_info->final_layout);
  }

  for (uint32_t i = 0; i < num_subpasses; ++i)
  {
    VkSubpassDescription* const sub      = subpasses + i;
    const bfSubpassCache* const sub_info = params->subpasses + i;

    for (uint32_t j = 0; j < sub_info->num_in_attachment_refs; ++j)
    {
      inputs[i][j] = bfAttToVkAtt(sub_info->in_attachment_refs + j);
    }

    for (uint32_t j = 0; j < sub_info->num_out_attachment_refs; ++j)
    {
      outputs[i][j] = bfAttToVkAtt(sub_info->out_attachment_refs + j);
    }

    const bool has_depth = sub_info->depth_attachment.attachment_index != UINT32_MAX;

    if (has_depth)
    {
      depth_atts[i] = bfAttToVkAtt(&sub_info->depth_attachment);
    }

    sub->flags                   = 0x0;
    sub->pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub->inputAttachmentCount    = sub_info->num_in_attachment_refs;
    sub->pInputAttachments       = inputs[i];
    sub->colorAttachmentCount    = sub_info->num_out_attachment_refs;
    sub->pColorAttachments       = outputs[i];
    sub->pResolveAttachments     = NULL;  // TODO(Shareef): This is for multisampling.
    sub->pDepthStencilAttachment = has_depth ? depth_atts + i : nullptr;
    sub->preserveAttachmentCount = 0;
    sub->pPreserveAttachments    = NULL;  // NOTE(Shareef): For attachments that must be preserved during this subpass but must not be touched by it.
  }

  for (uint32_t i = 0; i < num_dependencies; ++i)
  {
    VkSubpassDependency* const       dep      = dependencies + i;
    const bfSubpassDependency* const dep_info = params->dependencies + i;

    dep->srcSubpass      = dep_info->subpasses[0];
    dep->dstSubpass      = dep_info->subpasses[1];
    dep->srcStageMask    = bfVkConvertPipelineStageFlags(dep_info->pipeline_stage_flags[0]);
    dep->dstStageMask    = bfVkConvertPipelineStageFlags(dep_info->pipeline_stage_flags[1]);
    dep->srcAccessMask   = bfVkConvertAccessFlags(dep_info->access_flags[0]);
    dep->dstAccessMask   = bfVkConvertAccessFlags(dep_info->access_flags[1]);
    dep->dependencyFlags = dep_info->reads_same_pixel ? 0x0 : VK_DEPENDENCY_BY_REGION_BIT;
  }

  VkRenderPassCreateInfo renderpass_create_info;
  renderpass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderpass_create_info.pNext           = nullptr;
  renderpass_create_info.flags           = 0x0;
  renderpass_create_info.attachmentCount = num_attachments;
  renderpass_create_info.pAttachments    = attachments;
  renderpass_create_info.subpassCount    = num_subpasses;
  renderpass_create_info.pSubpasses      = subpasses;
  renderpass_create_info.dependencyCount = num_dependencies;
  renderpass_create_info.pDependencies   = dependencies;

  const auto err = vkCreateRenderPass(self->handle, &renderpass_create_info, CUSTOM_ALLOC, &renderpass->handle);

  assert(err == VK_SUCCESS && "Failed to create renderpass.");

  return renderpass;
}

template<typename T>
static void DeleteResource(T* obj)
{
  memset(obj, 0xCD, sizeof(*obj));
  xxx_Free(obj);
}

void bfGfxDevice_release_(bfGfxDeviceHandle self, bfGfxBaseHandle resource)
{
  if (resource)
  {
    bfBaseGfxObject* const obj = static_cast<bfBaseGfxObject*>(resource);

    switch (obj->type)
    {
      case BF_GFX_OBJECT_BUFFER:
      {
        bfBufferHandle buffer = reinterpret_cast<bfBufferHandle>(obj);

        vkDestroyBuffer(self->handle, buffer->handle, CUSTOM_ALLOC);
        VkPoolAllocator_free(buffer->alloc_pool, &buffer->alloc_info);

        DeleteResource(buffer);
        break;
      }
      case BF_GFX_OBJECT_RENDERPASS:
      {
        bfRenderpassHandle renderpass = reinterpret_cast<bfRenderpassHandle>(obj);

        vkDestroyRenderPass(self->handle, renderpass->handle, CUSTOM_ALLOC);

        DeleteResource(renderpass);
        break;
      }
      case BF_GFX_OBJECT_SHADER_MODULE:
      {
        bfShaderModuleHandle shader_module = reinterpret_cast<bfShaderModuleHandle>(obj);

        if (shader_module->handle != VK_NULL_HANDLE)
        {
          vkDestroyShaderModule(shader_module->parent->handle, shader_module->handle, CUSTOM_ALLOC);
        }

        DeleteResource(shader_module);
        break;
      }
      case BF_GFX_OBJECT_SHADER_PROGRAM:
      {
        bfShaderProgramHandle shader_program = reinterpret_cast<bfShaderProgramHandle>(obj);

        for (uint32_t i = 0; i < shader_program->num_desc_set_layouts; ++i)
        {
          const VkDescriptorSetLayout layout = shader_program->desc_set_layouts[i];

          if (layout != VK_NULL_HANDLE)
          {
            vkDestroyDescriptorSetLayout(shader_program->parent->handle, layout, CUSTOM_ALLOC);
          }
        }

        if (shader_program->layout != VK_NULL_HANDLE)
        {
          vkDestroyPipelineLayout(shader_program->parent->handle, shader_program->layout, CUSTOM_ALLOC);
        }

        DeleteResource(shader_program);
        break;
      }
      case BF_GFX_OBJECT_DESCRIPTOR_SET:
      {
        bfDescriptorSetHandle desc_set = reinterpret_cast<bfDescriptorSetHandle>(obj);

        MaterialPool_free(self->descriptor_pool, desc_set);

        DeleteResource(desc_set);
        break;
      }
      case BF_GFX_OBJECT_TEXTURE:
      {
        bfTextureHandle texture = reinterpret_cast<bfTextureHandle>(obj);

        bfTexture_setSampler(texture, nullptr);

        if (texture->tex_view != VK_NULL_HANDLE)
        {
          vkDestroyImageView(self->handle, texture->tex_view, CUSTOM_ALLOC);
        }

        if (texture->tex_memory != VK_NULL_HANDLE)
        {
          vkFreeMemory(self->handle, texture->tex_memory, CUSTOM_ALLOC);
        }

        if (texture->tex_image != VK_NULL_HANDLE)
        {
          vkDestroyImage(self->handle, texture->tex_image, CUSTOM_ALLOC);
        }

        DeleteResource(texture);
        break;
      }
      case BF_GFX_OBJECT_FRAMEBUFFER:
      {
        bfFramebufferHandle framebuffer = reinterpret_cast<bfFramebufferHandle>(obj);

        vkDestroyFramebuffer(self->handle, framebuffer->handle, CUSTOM_ALLOC);

        DeleteResource(framebuffer);
        break;
      }
      case BF_GFX_OBJECT_PIPELINE:
      {
        bfPipelineHandle pipeline = reinterpret_cast<bfPipelineHandle>(obj);

        vkDestroyPipeline(self->handle, pipeline->handle, CUSTOM_ALLOC);

        DeleteResource(pipeline);
        break;
      }
      default:
      {
        assert(!"Invalid object type.");
        break;
      }
    }

    // NOTE(SR):
    //   asan complains about new / delete pairing with different sizes.

    // memset(obj, 0xCD, sizeof(*obj));
    // delete obj;
  }
}
