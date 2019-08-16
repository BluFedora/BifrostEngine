#include "bifrost/graphics/bifrost_gfx_api.h"

#include "bifrost/debug/bifrost_dbg_logger.h"

#include "vulkan/bifrost_vulkan_physical_device.h" /* VulkanPhysicalDevice */

#include <cassert> /* assert   */
#include <cstddef> /* size_t   */
#include <cstdint> /* uint32_t */
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

namespace
{
  // Memory Functions (To Be Replaced)

  // This has very non c++ type sematics.

  template<typename T>
  struct FixedArray
  {
   private:
    T*          m_Data;
    std::size_t m_Size;

   public:
    FixedArray(std::size_t size) :
      m_Data(new T[size]),
      m_Size(size)
    {
    }

    FixedArray() :
      m_Data(nullptr),
      m_Size(0u)
    {
    }

    // A resize will not copy over the only elements.
    // Also does not checking for redundancy
    void setSize(std::size_t new_size)
    {
      delete[] m_Data;
      m_Data = (new_size ? (new T[new_size]) : nullptr);
      m_Size = new_size;
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

    [[nodiscard]] std::size_t size() const {
      return m_Size;
    }

    T* begin() const
    {
      return m_Data;
    }

    T* end() const
    {
      return m_Data + m_Size;
    }

    FixedArray(const FixedArray&) = delete;
    FixedArray(FixedArray&&)      = delete;
    FixedArray& operator=(const FixedArray&) = delete;
    FixedArray& operator=(FixedArray&&) = delete;

    ~FixedArray()
    {
      delete[] m_Data;
    }
  };

  // Context Setup
  void        gfxContextSetupApp(bfGfxContextHandle self, const bfGfxContextCreateParams* params);
  bfBool32    gfxContextCheckLayers(const char* const needed_layers[], size_t num_layers);
  bfBool32    gfxContextSetDebugCallback(bfGfxContextHandle self, PFN_vkDebugReportCallbackEXT callback);
  const char* gfxContextSetupPhysicalDevices(bfGfxContextHandle self);
  const char* gfxContextSelectPhysicalDevice(bfGfxContextHandle self);
  const char* gfxContextInitSurface(bfGfxContextHandle self);
  const char* gfxContextSelectLogicalDevice(bfGfxContextHandle self);
  const char* gfxContextInitAllocator(bfGfxContextHandle self);
  const char* gfxContextInitCommandPool(bfGfxContextHandle self);
  const char* gfxContextInitSemaphores(bfGfxContextHandle self);
  const char* gfxContextInitSwapchainInfo(bfGfxContextHandle self);
  const char* gfxContextInitMaterialPool(bfGfxContextHandle self);
}  // namespace

static VKAPI_ATTR VkBool32 VKAPI_CALL gfcContextDbgCallback(
 VkDebugReportFlagsEXT      flags,
 VkDebugReportObjectTypeEXT objType,
 uint64_t                   obj,
 size_t                     location,
 int32_t                    code,
 const char*                layerPrefix,
 const char*                msg,
 void*                      userData)
{
  bfLogError("validation layer: %s\n", msg);
  return VK_FALSE;
}

BIFROST_DEFINE_HANDLE(GfxContext)
{
  VkInstance                       instance;
  FixedArray<VulkanPhysicalDevice> physical_devices;

#if BIFROST_USE_DEBUG_CALLBACK
  VkDebugReportCallbackEXT debug_callback;
#endif
};

BIFROST_DEFINE_HANDLE(GfxDevice)
{
  VulkanPhysicalDevice* parent;
  VkDevice              handle;
  VkQueue               queues[BIFROST_GFX_QUEUE_MAX];
};

bfGfxContextHandle bfGfxContext_new(const bfGfxContextCreateParams* params)
{
  const auto self = new bfGfxContext();
  gfxContextSetupApp(self, params);
  if (!gfxContextSetDebugCallback(self, &gfcContextDbgCallback))
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
  return self;
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
    for (const auto& layer : layers)
    {
      bfLogPrint("|%-40s (%s) v%u", layer.layerName, layer.description, (unsigned)layer.implementationVersion);
    }
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

    bfDbgPush("Physical Device Listing (%u)", (unsigned)num_devices);
    size_t index = 0;
    for (auto& device : self->physical_devices)
    {
      device.handle = device_list[index];

      vkGetPhysicalDeviceMemoryProperties(vk_phys_dev, &device.memory_properties);
      vkGetPhysicalDeviceProperties(vk_phys_dev, &device.device_properties);
      vkGetPhysicalDeviceFeatures(vk_phys_dev, &device.device_features);

      vkGetPhysicalDeviceQueueFamilyProperties(device.handle, &device.queue_list.size, NULL);
      device.queue_list.queues = calloc(device.queue_list.size, sizeof(VkQueueFamilyProperties));
      vkGetPhysicalDeviceQueueFamilyProperties(device.handle, &device.queue_list.size, device.queue_list.queues);

      vkEnumerateDeviceExtensionProperties(device.handle, NULL, &device.extension_list.size, NULL);
      device.extension_list.extensions = malloc(sizeof(VkExtensionProperties) * device.extension_list.size);
      vkEnumerateDeviceExtensionProperties(device.handle, NULL, &device.extension_list.size, device.extension_list.extensions);

      // printf("VULKAN_PHYSICAL_DEVICE[%i / %i]:\n", (i + 1), devices->size);
      printf("---- Device Memory Properties ----\n");
      printf("\t Heap Count:        %i\n", device.memory_properties.memoryHeapCount);

      for (uint32_t j = 0; j < device.memory_properties.memoryHeapCount; ++j)
      {
        const VkMemoryHeap* const memory_heap = &device.memory_properties.memoryHeaps[j];

        printf("\t\t HEAP[%i].flags = %i\n", j, (int)memory_heap->flags);
        printf("\t\t HEAP[%i].size  = %u\n", j, (uint32_t)memory_heap->size);

        if (memory_heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
          printf("\t\t\t VK_MEMORY_HEAP_DEVICE_LOCAL_BIT = true;\n");
        }

        if (memory_heap->flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
        {
          printf("\t\t\t VK_MEMORY_HEAP_MULTI_INSTANCE_BIT = true;\n");
        }
      }

      printf("\t Memory Type Count: %i\n", device.memory_properties.memoryTypeCount);

      for (uint32_t j = 0; j < device.memory_properties.memoryTypeCount; ++j)
      {
        const VkMemoryType* const memory_type = &device.memory_properties.memoryTypes[j];

        printf("\t\t MEM_TYPE[%2i].heapIndex     = %u\n", j, memory_type->heapIndex);
        printf("\t\t MEM_TYPE[%2i].propertyFlags = %u\n", j, memory_type->propertyFlags);

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        {
          printf("\t\t\t VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = true;\n");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
          printf("\t\t\t VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = true;\n");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        {
          printf("\t\t\t VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = true;\n");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        {
          printf("\t\t\t VK_MEMORY_PROPERTY_HOST_CACHED_BIT = true;\n");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        {
          printf("\t\t\t VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT = true;\n");
        }

        if (memory_type->propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
        {
          printf("\t\t\t VK_MEMORY_PROPERTY_PROTECTED_BIT = true;\n");
        }
      }

      printf("------- Device  Properties -------\n");

      printf("\t API VERSION: %u.%u.%u\n",
             VK_VERSION_MAJOR(device.device_properties.apiVersion),
             VK_VERSION_MINOR(device.device_properties.apiVersion),
             VK_VERSION_PATCH(device.device_properties.apiVersion));
      printf("\t API VERSION: %u.%u.%u\n",
             VK_VERSION_MAJOR(device.device_properties.driverVersion),
             VK_VERSION_MINOR(device.device_properties.driverVersion),
             VK_VERSION_PATCH(device.device_properties.driverVersion));
      printf("\t DRIVER VERSION: %u\n", device.device_properties.driverVersion);
      printf("\t Device ID: %u\n", device.device_properties.deviceID);
      printf("\t Vendor ID: %u\n", device.device_properties.vendorID);

      switch (device.device_properties.deviceType)
      {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        {
          printf("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_OTHER\n");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        {
          printf("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU\n");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        {
          printf("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU\n");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        {
          printf("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU\n");
          break;
        }
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
        {
          printf("\t DEVICE_TYPE = VK_PHYSICAL_DEVICE_TYPE_CPU\n");
          break;
        }
        default:
        {
          printf("\t DEVICE_TYPE = DEVICE_TYPE_UNKNOWN\n");
          break;
        }
      }

      printf("\t DEVICE_NAME: \"%s\"\n", device.device_properties.deviceName);

      printf("\t PIPELINE_CACHE_UUID:\n");
      for (uint32_t j = 0; j < VK_UUID_SIZE; ++j)
      {
        printf("\t\t [%u] = %i\n", j, (int)device.device_properties.pipelineCacheUUID[j]);
      }

      // TODO: Device Limits    (VkPhysicalDeviceLimits)(limits)
      // TODO: sparseProperties (VkPhysicalDeviceSparseProperties)(sparseProperties)

      ++index;
    }
    bfDbgPop();

    return nullptr;
  }
}  // namespace
