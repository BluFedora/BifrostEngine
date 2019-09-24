#include "bifrost/graphics/bifrost_gfx_api.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include "bifrost/debug/bifrost_dbg_logger.h"
#include "vulkan/bifrost_vulkan_logical_device.h"
#include "vulkan/bifrost_vulkan_material_pool.h"
#include "vulkan/bifrost_vulkan_mem_allocator.h"

#include <cassert> /* assert   */
#include <cstddef> /* size_t   */
#include <cstdint> /* uint32_t */
#include <cstdlib>
#include <cstring> /* strcmp   */
#include <vulkan/vulkan.h>

#define BIFROST_USE_DEBUG_CALLBACK 1
#define BIFROST_USE_VALIDATION_LAYERS 1
#define BIFROST_ENGINE_NAME "Bifrost Engine"
#define BIFROST_ENGINE_VERSION 0
#define BIFROST_VULKAN_CUSTOM_ALLOCATOR nullptr

// [ ] bifrost_vulkan_backend.h
// [ ] bifrost_vulkan_framebuffer.c
// [ ] bifrost_vulkan_pool_allocator.c
// [ ] bifrost_vulkan_buffer.c
// [ ] bifrost_vulkan_framebuffer.h
// [ ] bifrost_vulkan_pool_allocator.h
// [ ] bifrost_vulkan_buffer.h
// [ ] bifrost_vulkan_logical_device.c
// [ ] bifrost_vulkan_render_pass.c
// [ ] bifrost_vulkan_command_buffer.c
// [ ] bifrost_vulkan_material.c
// [ ] bifrost_vulkan_render_pass.h
// [ ] bifrost_vulkan_context.c
// [ ] bifrost_vulkan_material.h
// [ ] bifrost_vulkan_shader.c
// [ ] bifrost_vulkan_context.h
// [ ] bifrost_vulkan_shader.h
// [ ] bifrost_vulkan_conversions.c
// [ ] bifrost_vulkan_pipeline.c
// [ ] bifrost_vulkan_texture.c
// [ ] bifrost_vulkan_conversions.h
// [ ] bifrost_vulkan_pipeline.h
// [ ] bifrost_vulkan_texture.h
// [X] bifrost_vulkan_physical_device.h

// Memory Functions (To Be Replaced)

template<typename T>
T* allocN(std::size_t num_elements)
{
  return (T*)std::calloc(num_elements, sizeof(T));
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

  // A resize will not copy over the only elements.
  void setSize(std::size_t new_size)
  {
    if (new_size != m_Size)
    {
      freeN(m_Data);
      m_Data = (new_size ? allocN<T>(new_size) : nullptr);
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

struct VulkanWindow final
{
  VkSurfaceKHR        surface;
  VulkanSwapchainInfo swapchain_info;
  VulkanSwapchain     swapchain;
  VkSemaphore*        is_image_available;
  VkSemaphore*        is_render_done;
};

BIFROST_DEFINE_HANDLE(GfxContext)
{
  const bfGfxContextCreateParams*  params;               // Only valid during initialization
  std::size_t                      max_frame_in_flight;  // TODO(Shareef): Make customizable
  VkInstance                       instance;
  FixedArray<VulkanPhysicalDevice> physical_devices;
  VulkanPhysicalDevice*            physical_device;
  VulkanWindow                     main_window;
  bfGfxDeviceHandle                logical_device;
  PoolAllocator                    device_memory_allocator;
  VkCommandPool                    command_pools[1];  // TODO(Shareef): One per thread.
  VulkanDescriptorPool*            descriptor_pool;
  uint32_t                         image_index;
  uint32_t                         frame_count;
  uint32_t                         frame_index;  // frame_count % max_frame_in_flight

#if BIFROST_USE_DEBUG_CALLBACK
  VkDebugReportCallbackEXT debug_callback;
#endif
};

namespace
{
  // Context Setup
  void        gfxContextSetupApp(bfGfxContextHandle self, const bfGfxContextCreateParams* params);
  bfBool32    gfxContextCheckLayers(const char* const needed_layers[], size_t num_layers);
  bfBool32    gfxContextSetDebugCallback(bfGfxContextHandle self, PFN_vkDebugReportCallbackEXT callback);
  const char* gfxContextSetupPhysicalDevices(bfGfxContextHandle self);
  void        gfxContextPrintExtensions();
  const char* gfxContextSelectPhysicalDevice(bfGfxContextHandle self);
  const char* gfxContextInitSurface(bfGfxContextHandle self);
  const char* gfxContextFindSurfacePresent(bfGfxContextHandle self, VulkanWindow& window);
  const char* gfxContextCreateLogicalDevice(bfGfxContextHandle self);
  const char* gfxContextInitAllocator(bfGfxContextHandle self);
  const char* gfxContextInitCommandPool(bfGfxContextHandle self, uint16_t thread_index);
  const char* gfxContextInitSemaphores(bfGfxContextHandle self, VulkanWindow& window);
  const char* gfxContextInitSwapchainInfo(bfGfxContextHandle self, VulkanWindow& window);
  const char* gfxContextInitMaterialPool(bfGfxContextHandle self);
  void        gfxContextInitSwapchain(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextInitSwapchainImageList(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextInitCmdFences(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextInitCmdBuffers(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextDestroyCmdBuffers(bfGfxContextHandle self, const VulkanSwapchain* swapchain);
  void        gfxContextDestroyCmdFences(bfGfxContextHandle self, const VulkanSwapchain* swapchain);
  void        gfxContextDestroySwapchainImageList(bfGfxContextHandle self, const VulkanSwapchain* swapchain);
  void        gfxContextDestroySwapchain(bfGfxContextHandle self, VulkanSwapchain* swapchain);
}  // namespace

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
  bfLogError("validation layer: %s", msg);
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

bfGfxContextHandle bfGfxContext_new(const bfGfxContextCreateParams* params)
{
  const auto self           = new bfGfxContext();
  self->params              = params;
  self->max_frame_in_flight = 2;
  self->frame_count         = 0;
  self->frame_index         = 0;
  gfxContextSetupApp(self, params);
  if (!gfxContextSetDebugCallback(self, &gfxContextDbgCallback))
  {
    bfLogError("Failed to set the debug callback.");
  }
  {
    const auto err = gfxContextSetupPhysicalDevices(self);

    if (err)
    {
      bfLogError("Failed to '%s'.", err);
    }
  }
  {
    gfxContextPrintExtensions();
  }
  {
    // TODO(Shareef): Make an array of fn pointers to auto check.
    gfxContextSelectPhysicalDevice(self);
    gfxContextInitSurface(self);
    gfxContextFindSurfacePresent(self, self->main_window);
    gfxContextCreateLogicalDevice(self);
    gfxContextInitAllocator(self);
    gfxContextInitCommandPool(self, 0);
    gfxContextInitSemaphores(self, self->main_window);
    gfxContextInitSwapchainInfo(self, self->main_window);
    gfxContextInitMaterialPool(self);
  }
  return self;
}

bfGfxDeviceHandle bfGfxContext_device(bfGfxContextHandle self)
{
  return self->logical_device;
}

bfTextureHandle bfGfxContext_swapchainImage(bfGfxContextHandle self)
{
  return &self->main_window.swapchain.img_list.images[self->image_index];
}

void bfGfxContext_onResize(bfGfxContextHandle self, uint32_t width, uint32_t height)
{
  bfGfxDevice_flush(self->logical_device);

  auto& window = self->main_window;

  VulkanSwapchain old_swapchain = window.swapchain;

  gfxContextDestroyCmdBuffers(self, &old_swapchain);
  gfxContextDestroyCmdFences(self, &old_swapchain);
  gfxContextDestroySwapchainImageList(self, &old_swapchain);
  gfxContextDestroySwapchain(self, &old_swapchain);

  gfxContextInitSwapchain(self, window);
  gfxContextInitSwapchainImageList(self, window);
  gfxContextInitCmdFences(self, window);
  gfxContextInitCmdBuffers(self, window);
}

bfBool32 bfGfxContext_beginFrame(bfGfxContextHandle self)
{
  return bfFalse;
}

void bfGfxContext_endFrame(bfGfxContextHandle self)
{
  ++self->frame_count;
  self->frame_index = self->frame_count % self->max_frame_in_flight;
}

void bfGfxContext_delete(bfGfxContextHandle self)
{
  delete self;
}

// Device

void bfGfxDevice_flush(bfGfxDeviceHandle self)
{
  // vkQueueWaitIdle(GfxContext_currentLogicalDevice()->queues[DQI_GRAPHICS]);
  vkDeviceWaitIdle(self->handle);
}

namespace
{
  void gfxContextSetupApp(bfGfxContextHandle self, const bfGfxContextCreateParams* params)
  {
    static const char* const VALIDATION_LAYER_NAMES[] = {"VK_LAYER_LUNARG_standard_validation"};
    static const char* const INSTANCE_EXT_NAMES[]     = {
      VK_KHR_SURFACE_EXTENSION_NAME,

#if defined(VK_USE_PLATFORM_WIN32_KHR)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
      VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#else
#error "Invalid platform selected for the Vulkan Rendering Backend."
#endif

#if BIFROST_USE_DEBUG_CALLBACK
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#endif
    };

#if BIFROST_USE_VALIDATION_LAYERS
    const bfBool32 layers_are_supported = gfxContextCheckLayers(VALIDATION_LAYER_NAMES, bfCArraySize(VALIDATION_LAYER_NAMES));

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
    app_info.apiVersion         = VK_API_VERSION_1_0;

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
    init_info.enabledExtensionCount   = bfCArraySize(INSTANCE_EXT_NAMES);
    init_info.ppEnabledExtensionNames = INSTANCE_EXT_NAMES;

    const VkResult err = vkCreateInstance(&init_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->instance);

    if (err)
    {
      static const char* const error_messages[] =
       {
        "Unknown Error.",
        "There was not a compatible Vulkan ICD.",
       };

      bfLogError("gfxContextSetupApp(%s %s)", "vkCreateInstance", error_messages[err == VK_ERROR_INCOMPATIBLE_DRIVER]);
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

  bfBool32 gfxContextSetDebugCallback(bfGfxContextHandle self, PFN_vkDebugReportCallbackEXT callback)
  {
#if BIFROST_USE_DEBUG_CALLBACK
    VkDebugReportCallbackCreateInfoEXT create_info;
    create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    create_info.pNext       = nullptr;
    create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    create_info.pfnCallback = callback;
    create_info.pUserData   = nullptr;

    PFN_vkCreateDebugReportCallbackEXT func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(self->instance, "vkCreateDebugReportCallbackEXT");

    if (func)
    {
      return func(self->instance, &create_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->debug_callback) == VK_SUCCESS;
    }

    return bfFalse;  //VK_ERROR_EXTENSION_NOT_PRESENT;
#else
    return bfTrue;
#endif
  }

  const char* gfxContextSetupPhysicalDevices(bfGfxContextHandle self)
  {
    std::uint32_t num_devices;
    VkResult      err = vkEnumeratePhysicalDevices(self->instance, &num_devices, nullptr);

    if (err)
    {
      return "enumerate devices";
    }

    if (num_devices == 0)
    {
      return "find a Vulkan enabled device.";
    }

    FixedArray<VkPhysicalDevice> device_list{num_devices};
    self->physical_devices.setSize(num_devices);

    err = vkEnumeratePhysicalDevices(self->instance, &num_devices, &device_list[0]);

    if (err)
    {
      return "enumerate devices";
    }

    bfLogPush("Physical Device Listing (%u)", (unsigned)num_devices);
    size_t index = 0;
    for (auto& device : self->physical_devices)
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

  const char* gfxContextSelectPhysicalDevice(bfGfxContextHandle self)
  {
    if (self->physical_devices.size() == 0)
    {
      return "Found no Physical devices";
    }

    // TODO: Select device based on "device_features", "device_properties", "deviceType" etc
    self->physical_device = &self->physical_devices[0];

    return nullptr;
  }

  const char* gfxContextInitSurface(bfGfxContextHandle self)
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
     .dpy = s_Display,
     .window = s_Window};

    VkResult err = vkCreateXlibSurfaceKHR(instance, &create_info, CUSTOM_ALLOCATOR, surface);
#else  // TODO: Other Platforms
#error "unsupported platform for Vulkan"
#endif

    return err ? "Failed to create Surface" : nullptr;
  }

  uint32_t findQueueBasic(const VulkanPhysicalDevice* device, uint32_t queue_size, VkQueueFlags flags);

  const char* gfxContextFindSurfacePresent(bfGfxContextHandle self, VulkanWindow& window)
  {
    VulkanPhysicalDevice* device           = self->physical_device;
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

    device->queue_list.graphics_family_index = UINT32_MAX;
    device->queue_list.compute_family_index  = UINT32_MAX;
    device->queue_list.transfer_family_index = UINT32_MAX;
    device->queue_list.present_family_index  = UINT32_MAX;

    for (uint32_t i = 0; i < queue_size; ++i)
    {
      const VkQueueFamilyProperties* const queue = &device->queue_list.queues[i];

      if (queue->queueCount && queue->queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        if (device->queue_list.graphics_family_index == UINT32_MAX)
        {
          device->queue_list.graphics_family_index = i;
        }

        if (supports_present[i])
        {
          device->queue_list.present_family_index = device->queue_list.graphics_family_index = i;
          break;
        }
      }
    }

    device->queue_list.compute_family_index  = findQueueBasic(device, queue_size, VK_QUEUE_COMPUTE_BIT);
    device->queue_list.transfer_family_index = findQueueBasic(device, queue_size, VK_QUEUE_TRANSFER_BIT);

    if (device->queue_list.present_family_index == UINT32_MAX)
    {
      // If didn't find a queue that supports both graphics and present, then find a separate present queue.
      for (uint32_t i = 0; i < queue_size; ++i)
      {
        if (supports_present[i])
        {
          device->queue_list.present_family_index = i;
          break;
        }
      }
    }

    freeN(supports_present);

    if (
     device->queue_list.graphics_family_index == UINT32_MAX ||
     device->queue_list.present_family_index == UINT32_MAX ||
     device->queue_list.compute_family_index == UINT32_MAX ||
     device->queue_list.transfer_family_index == UINT32_MAX)
    {
      return "Could not find Queues for Present / Graphics / Compute / Transfer.";
    }

    return nullptr;
  }

  VkDeviceQueueCreateInfo makeBasicQCreateInfo(uint32_t queue_index, uint32_t num_queues, const float* queue_priortites)
  {
    VkDeviceQueueCreateInfo create_info;

    create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    create_info.pNext            = NULL;
    create_info.flags            = 0x0;
    create_info.queueFamilyIndex = queue_index;
    create_info.queueCount       = num_queues;
    create_info.pQueuePriorities = queue_priortites;

    return create_info;
  }

  const char* gfxContextCreateLogicalDevice(bfGfxContextHandle self)
  {
    static const char* const DEVICE_EXT_NAMES[] =
     {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
     };  // I should be checking of the extensions are supported.

    static const float queue_priorities[] = {0.0f};

    VulkanPhysicalDevice*   device        = self->physical_device;
    const uint32_t          gfx_queue_idx = device->queue_list.graphics_family_index;
    uint32_t                num_queues    = 0;
    VkDeviceQueueCreateInfo queue_create_infos[BIFROST_GFX_QUEUE_MAX];

    const auto addQueue = [gfx_queue_idx, &num_queues, &queue_create_infos](uint32_t queue_index) {
      if (gfx_queue_idx != queue_index)
      {
        queue_create_infos[num_queues++] = makeBasicQCreateInfo(queue_index, 1, queue_priorities);
      }
    };

    queue_create_infos[num_queues++] = makeBasicQCreateInfo(gfx_queue_idx, 1, queue_priorities);
    addQueue(device->queue_list.compute_family_index);
    addQueue(device->queue_list.transfer_family_index);
    addQueue(device->queue_list.present_family_index);

    VkPhysicalDeviceFeatures deviceFeatures;  // I should be checking if the features exist for my device at all.
    memset(&deviceFeatures, 0x0, sizeof(deviceFeatures));
    //deviceFeatures.samplerAnisotropy = VK_TRUE;
    //deviceFeatures.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo device_info;
    device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext                   = NULL;
    device_info.flags                   = 0;
    device_info.queueCreateInfoCount    = num_queues;
    device_info.pQueueCreateInfos       = queue_create_infos;
    device_info.enabledLayerCount       = 0;
    device_info.ppEnabledLayerNames     = NULL;
    device_info.enabledExtensionCount   = bfCArraySize(DEVICE_EXT_NAMES);
    device_info.ppEnabledExtensionNames = DEVICE_EXT_NAMES;
    device_info.pEnabledFeatures        = &deviceFeatures;

    self->logical_device         = new bfGfxDevice();
    self->logical_device->parent = device;

    const VkResult err = vkCreateDevice(device->handle, &device_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->logical_device->handle);

    if (err)
    {
      delete self->logical_device;
      self->logical_device = nullptr;
      return "Failed to create device";
    }

    // Matches order of 'BifrostGfxQueueType'.
    const uint32_t queues_to_grab[] =
     {
      device->queue_list.graphics_family_index,  // BIFROST_GFX_QUEUE_GRAPHICS
      device->queue_list.compute_family_index,   // BIFROST_GFX_QUEUE_COMPUTE
      device->queue_list.transfer_family_index,  // BIFROST_GFX_QUEUE_TRANSFER
      device->queue_list.present_family_index,   // BIFROST_GFX_QUEUE_PRESENT
     };

    for (uint32_t i = 0; i < bfCArraySize(queues_to_grab); ++i)
    {
      // The 0 means grab the first queue of the specified family.
      // the number must be less than "VkDeviceQueueCreateInfo::queueCount"
      vkGetDeviceQueue(self->logical_device->handle, queues_to_grab[i], 0, &self->logical_device->queues[i]);
    }

    return nullptr;
  }

  const char* gfxContextInitAllocator(bfGfxContextHandle self)
  {
    VkPoolAllocatorCtor(&self->device_memory_allocator, self->logical_device);
    return nullptr;
  }

  const char* gfxContextInitCommandPool(bfGfxContextHandle self, uint16_t thread_index)
  {
    assert(thread_index == 0 && "Current implemetation only supports one thread currently.");

    const VulkanPhysicalDevice* const phys_device    = self->physical_device;
    bfGfxDeviceHandle                 logical_device = self->logical_device;

    // This should be per thread.
    VkCommandPoolCreateInfo cmd_pool_info;
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |            // Since these are short lived buffers
                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // Since I Reuse the Buffer each frame
    cmd_pool_info.queueFamilyIndex = phys_device->queue_list.graphics_family_index;

    VkResult error = vkCreateCommandPool(
     logical_device->handle,
     &cmd_pool_info,
     BIFROST_VULKAN_CUSTOM_ALLOCATOR,
     &self->command_pools[thread_index]);

    return error ? "Failed to create command pool" : nullptr;
  }

  const char* gfxContextInitSemaphores(bfGfxContextHandle self, VulkanWindow& window)
  {
    const size_t       num_semaphores = self->max_frame_in_flight * 2;
    VkSemaphore* const semaphores     = allocN<VkSemaphore>(num_semaphores);

    window.is_image_available = semaphores;
    window.is_render_done     = semaphores + self->max_frame_in_flight;

    VkSemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = NULL;
    semaphore_create_info.flags = 0;

    for (size_t i = 0; i < num_semaphores; ++i)
    {
      const VkResult err = vkCreateSemaphore(
       self->logical_device->handle,
       &semaphore_create_info,
       BIFROST_VULKAN_CUSTOM_ALLOCATOR,
       semaphores + i);

      if (err)
      {
        return "Failed to create a Semaphore";
      }
    }

    return nullptr;
  }

  VkSurfaceFormatKHR gfxContextFindSurfaceFormat(const VkSurfaceFormatKHR* const formats, const uint32_t num_formats);

  const char* gfxContextInitSwapchainInfo(bfGfxContextHandle self, VulkanWindow& window)
  {
    VulkanPhysicalDevice* const device = self->physical_device;
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

  const char* gfxContextInitMaterialPool(bfGfxContextHandle self)
  {
    // TODO(SR): Pick better magic numbers / make it customizable.
    MaterialPoolCreateParams material_pool_create;
    material_pool_create.logical_device        = self->logical_device;
    material_pool_create.num_textures_per_link = 32;
    material_pool_create.num_uniforms_per_link = 16;
    material_pool_create.num_descsets_per_link = 8;

    self->descriptor_pool = MaterialPool_new(&material_pool_create);

    return nullptr;
  }

  void gfxContextInitSwapchain(bfGfxContextHandle self, VulkanWindow& window)
  {
    VulkanPhysicalDevice* const logical_device = self->physical_device;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(logical_device->handle, window.surface, &window.swapchain_info.capabilities);
    assert(!"Finish implementing this.");
  }

  void gfxContextInitSwapchainImageList(bfGfxContextHandle self, VulkanWindow& window)
  {
    bfGfxDeviceHandle      logical_device = self->logical_device;
    VulkanSwapchain* const swapchain      = &window.swapchain;

    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &swapchain->img_list.size, NULL);

    FixedArray<VkImage> temp_images{swapchain->img_list.size};
    vkGetSwapchainImagesKHR(logical_device->handle, swapchain->handle, &swapchain->img_list.size, &temp_images[0]);

    swapchain->img_list.images = allocN<bfTexture>(swapchain->img_list.size);

    for (uint32_t i = 0; i < swapchain->img_list.size; ++i)
    {
      bfTexture* const image = swapchain->img_list.images + i;

      image->image_type      = BIFROST_TEX_TYPE_2D;
      image->image_width     = int32_t(swapchain->extents.width);
      image->image_height    = int32_t(swapchain->extents.height);
      image->image_depth     = 1;
      image->image_miplevels = 1;
      image->tex_memory      = VK_NULL_HANDLE;
      // image->tex_view = ; TODO(SR): Create View
      image->tex_sampler = VK_NULL_HANDLE;
      image->tex_layout  = BIFROST_IMAGE_LAYOUT_UNDEFINED;
      image->tex_format  = (BifrostImageFormat)swapchain->format.format;  // TODO: This needs to be made safer.

      //
      // Texture_createImageView(logical_device->handle, temp_images[i], swapchain->format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
      //
    }
  }

  void gfxContextInitCmdFences(bfGfxContextHandle self, VulkanWindow& window)
  {
    bfGfxDeviceHandle logical_device = self->logical_device;

    const std::uint32_t num_fences = window.swapchain.img_list.size;
    window.swapchain.fences        = allocN<VkFence>(window.swapchain.img_list.size);

    for (uint32_t i = 0; i < num_fences; ++i)
    {
      VkFenceCreateInfo fence_create_info;
      fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_create_info.pNext = NULL;
      fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      vkCreateFence(logical_device->handle, &fence_create_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, window.swapchain.fences + i);
    }
  }

  void gfxContextInitCmdBuffers(bfGfxContextHandle self, VulkanWindow& window)
  {
    bfGfxDeviceHandle device = self->logical_device;

    VkCommandBufferAllocateInfo cmd_alloc_info;
    cmd_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext              = NULL;
    cmd_alloc_info.commandPool        = self->command_pools[0];
    cmd_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = window.swapchain.img_list.size;

    window.swapchain.command_buffers = allocN<VkCommandBuffer>(cmd_alloc_info.commandBufferCount);

    VkResult error = vkAllocateCommandBuffers(device->handle, &cmd_alloc_info, window.swapchain.command_buffers);
    assert(error == VK_SUCCESS);
  }

  void gfxContextDestroyCmdBuffers(bfGfxContextHandle self, const VulkanSwapchain* swapchain)
  {
    bfGfxDeviceHandle logical_device = self->logical_device;
    vkFreeCommandBuffers(logical_device->handle, self->command_pools[0], swapchain->img_list.size, swapchain->command_buffers);
    freeN(swapchain->command_buffers);
  }

  void gfxContextDestroyCmdFences(bfGfxContextHandle self, const VulkanSwapchain* swapchain)
  {
    bfGfxDeviceHandle logical_device = self->logical_device;
    VkDevice          device         = logical_device->handle;

    for (uint32_t i = 0; i < swapchain->img_list.size; ++i)
    {
      vkDestroyFence(device, swapchain->fences[i], BIFROST_VULKAN_CUSTOM_ALLOCATOR);
    }

    freeN(swapchain->fences);
  }

  void gfxContextDestroySwapchainImageList(bfGfxContextHandle self, const VulkanSwapchain* swapchain)
  {
    bfGfxDeviceHandle                     logical_device = self->logical_device;
    VkDevice                              device         = logical_device->handle;
    const VulkanSwapchainImageList* const image_list     = &swapchain->img_list;

    for (uint32_t i = 0; i < image_list->size; ++i)
    {
      vkDestroyImageView(device, image_list->images[i].tex_view, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
    }

    free(image_list->images);
  }

  void gfxContextDestroySwapchain(bfGfxContextHandle self, VulkanSwapchain* swapchain)
  {
    if (swapchain->handle)
    {
      bfGfxDeviceHandle logical_device = self->logical_device;

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
}  // namespace

BifrostImageLayout bfTexture_layout(bfTextureHandle self)
{
  return self->tex_layout;
}
