#include "bifrost/graphics/bifrost_gfx_api.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include "bifrost/debug/bifrost_dbg_logger.h"
#include "vulkan/bifrost_vulkan_logical_device.h"
#include "vulkan/bifrost_vulkan_material_pool.h"
#include "vulkan/bifrost_vulkan_mem_allocator.h"

#include "vulkan/bifrost_vulkan_conversions.h"
#include <algorithm> /* clamp */
#include <cassert>   /* assert   */
#include <cmath>
#include <cstddef> /* size_t   */
#include <cstdint> /* uint32_t */
#include <cstdlib>
#include <cstring> /* strcmp   */
#include <vulkan/vulkan.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION

#define BIFROST_USE_DEBUG_CALLBACK 1
#define BIFROST_USE_VALIDATION_LAYERS 1
#define BIFROST_ENGINE_NAME "Bifrost Engine"
#define BIFROST_ENGINE_VERSION 0
#define BIFROST_VULKAN_CUSTOM_ALLOCATOR nullptr

// Memory Functions (To Be Replaced)

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

BIFROST_DEFINE_HANDLE(GfxContext)
{
  const bfGfxContextCreateParams*  params;                // Only valid during initialization
  std::uint32_t                    max_frames_in_flight;  // TODO(Shareef): Make customizable
  VkInstance                       instance;
  FixedArray<VulkanPhysicalDevice> physical_devices;
  VulkanPhysicalDevice*            physical_device;
  VulkanWindow                     main_window;
  bfGfxDeviceHandle                logical_device;
  VkCommandPool                    command_pools[1];  // TODO(Shareef): One per thread.
  uint32_t                         image_index;
  bfFrameCount_t                   frame_count;
  bfFrameCount_t                   frame_index;  // frame_count % max_frames_in_flight

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
  bool        gfxContextInitSwapchain(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextInitSwapchainImageList(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextInitCmdFences(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextInitCmdBuffers(bfGfxContextHandle self, VulkanWindow& window);
  void        gfxContextDestroyCmdBuffers(bfGfxContextHandle self, VulkanSwapchain* swapchain);
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
  const auto self            = new bfGfxContext();
  self->params               = params;
  self->max_frames_in_flight = 2;
  self->frame_count          = 0;
  self->frame_index          = 0;
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
    gfxContextInitSwapchainInfo(self, self->main_window);
    gfxContextInitSemaphores(self, self->main_window);
  }

  self->params = nullptr;
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

static void gfxRecreateSwapchain(bfGfxContextHandle self, VulkanWindow& window)
{
  if (gfxContextInitSwapchain(self, window))
  {
    gfxContextInitSwapchainImageList(self, window);
    gfxContextInitCmdFences(self, window);
    gfxContextInitCmdBuffers(self, window);
  }
}

void bfGfxContext_onResize(bfGfxContextHandle self)
{
  bfGfxDevice_flush(self->logical_device);

  auto& window = self->main_window;

  VulkanSwapchain old_swapchain = window.swapchain;

  gfxContextDestroyCmdBuffers(self, &old_swapchain);
  gfxContextDestroyCmdFences(self, &old_swapchain);
  gfxContextDestroySwapchainImageList(self, &old_swapchain);
  gfxContextDestroySwapchain(self, &old_swapchain);

  gfxRecreateSwapchain(self, window);
}

bfBool32 bfGfxContext_beginFrame(bfGfxContextHandle self, int window_idx)
{
  if (window_idx < 0)
  {
    window_idx = 0;
  }

  VulkanWindow* window = window_idx == 0 ? &self->main_window : nullptr;

  if (window)
  {
    if (window->swapchain_needs_creation)
    {
      bfGfxDevice_flush(self->logical_device);
      gfxRecreateSwapchain(self, *window);

      return bfFalse;
    }

    if (window->swapchain.extents.width <= 0.0f && window->swapchain.extents.height <= 0.0f)
    {
      return bfFalse;
    }

    const VkResult err = vkAcquireNextImageKHR(self->logical_device->handle,
                                               window->swapchain.handle,
                                               UINT64_MAX,
                                               window->is_image_available,
                                               VK_NULL_HANDLE,
                                               &window->image_index);

    if (!(err == VK_SUCCESS || err == VK_TIMEOUT || err == VK_NOT_READY || err == VK_SUBOPTIMAL_KHR))
    {
      if (err == VK_ERROR_OUT_OF_DATE_KHR)
      {
        bfGfxContext_onResize(self);
        std::printf("Surface out of date.... recreating swap chain\n");
      }

      return bfFalse;
    }

    const VkFence command_fence = window->swapchain.fences[window->image_index];

    if (vkWaitForFences(self->logical_device->handle, 1, &command_fence, VK_FALSE, UINT64_MAX) != VK_SUCCESS)
    {
      std::printf("Waiting for fence takes too long!");
      return bfFalse;
    }

    vkResetFences(self->logical_device->handle, 1, &command_fence);

    return bfTrue;
  }

  return bfFalse;
}

bfGfxFrameInfo bfGfxContext_getFrameInfo(bfGfxContextHandle self, int window_idx)
{
  if (window_idx < 0)
  {
    window_idx = 0;
  }

  VulkanWindow* window = window_idx == 0 ? &self->main_window : nullptr;

  if (window)
  {
    return {window->image_index, self->frame_count, window->swapchain.img_list.size};
  }

  return {0, 0, 0};
}

void bfGfxContext_endFrame(bfGfxContextHandle self)
{
  // TODO: This whole set of garbage collection should maybe get called not every frame??

  BifrostGfxObjectBase* prev         = nullptr;
  BifrostGfxObjectBase* curr         = self->logical_device->cached_resources;
  BifrostGfxObjectBase* release_list = nullptr;

  while (curr)
  {
    BifrostGfxObjectBase* next = curr->next;

    if ((self->frame_count - curr->last_frame_used & bfFrameCountMax) >= 60)
    {
      if (prev)
      {
        prev->next = next;
      }
      else
      {
        self->logical_device->cached_resources = next;
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

  if (release_list)
  {
    // bfGfxDevice_flush(self->logical_device);
    while (release_list)
    {
      BifrostGfxObjectBase* next = release_list->next;

      if (release_list->type == BIFROST_GFX_OBJECT_RENDERPASS)
      {
        self->logical_device->cache_renderpass.remove(release_list->hash_code);
      }
      else if (release_list->type == BIFROST_GFX_OBJECT_PIPELINE)
      {
        self->logical_device->cache_pipeline.remove(release_list->hash_code);
      }
      else if (release_list->type == BIFROST_GFX_OBJECT_FRAMEBUFFER)
      {
        self->logical_device->cache_framebuffer.remove(release_list->hash_code);
      }
      else if (release_list->type == BIFROST_GFX_OBJECT_DESCRIPTOR_SET)
      {
        self->logical_device->cache_descriptor_set.remove(release_list->hash_code);
      }
      else
      {
        assert(!"Need to updated this check.");
      }

      bfGfxDevice_release(self->logical_device, release_list);
      release_list = next;
    }
  }

  ++self->frame_count;
  self->frame_index = self->frame_count % self->max_frames_in_flight;
}

void bfGfxContext_delete(bfGfxContextHandle self)
{
  auto&           window        = self->main_window;
  VulkanSwapchain old_swapchain = window.swapchain;
  auto            device        = self->logical_device;

  BifrostGfxObjectBase* curr = device->cached_resources;

  while (curr)
  {
    BifrostGfxObjectBase* next = curr->next;
    bfGfxDevice_release(self->logical_device, curr);
    curr = next;
  }

  VkPoolAllocatorDtor(&device->device_memory_allocator);

  gfxContextDestroyCmdBuffers(self, &old_swapchain);
  gfxContextDestroyCmdFences(self, &old_swapchain);
  gfxContextDestroySwapchainImageList(self, &old_swapchain);
  gfxContextDestroySwapchain(self, &old_swapchain);

  MaterialPool_delete(device->descriptor_pool);

  vkDestroySemaphore(device->handle, window.is_image_available, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  vkDestroySemaphore(device->handle, window.is_render_done, BIFROST_VULKAN_CUSTOM_ALLOCATOR);

#if BIFROST_USE_DEBUG_CALLBACK
  PFN_vkDestroyDebugReportCallbackEXT func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(self->instance, "vkDestroyDebugReportCallbackEXT");

  if (func)
  {
    func(self->instance, self->debug_callback, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  }
#endif

  vkDestroyCommandPool(device->handle, self->command_pools[0], BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  vkDestroyDevice(device->handle, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  vkDestroyInstance(self->instance, BIFROST_VULKAN_CUSTOM_ALLOCATOR);
  delete self;
}

// Device

void bfGfxDevice_flush(bfGfxDeviceHandle self)
{
  vkDeviceWaitIdle(self->handle);
}

bfGfxCommandListHandle bfGfxContext_requestCommandList(bfGfxContextHandle self, const bfGfxCommandListCreateParams* params)
{
  int window_idx = params->window_idx;

  if (window_idx < 0)
  {
    window_idx = 0;
  }

  VulkanWindow* window = window_idx == 0 ? &self->main_window : nullptr;

  if (window)
  {
    bfGfxCommandListHandle list = new bfGfxCommandList();

    list->context        = self;
    list->parent         = self->logical_device;
    list->handle         = window->swapchain.command_buffers[window->image_index];
    list->fence          = window->swapchain.fences[window->image_index];
    list->window         = window;
    list->render_area    = {};
    list->framebuffer    = nullptr;
    list->pipeline       = nullptr;
    list->pipeline_state = {};
    list->has_command    = bfFalse;
    std::memset(list->clear_colors, 0x0, sizeof(list->clear_colors));

    vkResetCommandBuffer(list->handle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

    bfGfxCmdList_setDrawMode(list, BIFROST_DRAW_MODE_TRIANGLE_LIST);
    bfGfxCmdList_setFrontFace(list, BIFROST_FRONT_FACE_CW);
    bfGfxCmdList_setCullFace(list, BIFROST_CULL_FACE_NONE);
    bfGfxCmdList_setDepthTesting(list, bfFalse);
    bfGfxCmdList_setDepthWrite(list, bfFalse);
    bfGfxCmdList_setDepthTestOp(list, BIFROST_COMPARE_OP_ALWAYS);
    bfGfxCmdList_setStencilTesting(list, bfFalse);
    bfGfxCmdList_setPrimitiveRestart(list, bfFalse);
    bfGfxCmdList_setRasterizerDiscard(list, bfFalse);
    bfGfxCmdList_setDepthBias(list, bfFalse);
    bfGfxCmdList_setSampleShading(list, bfFalse);
    bfGfxCmdList_setAlphaToCoverage(list, bfFalse);
    bfGfxCmdList_setAlphaToOne(list, bfFalse);
    bfGfxCmdList_setLogicOp(list, BIFROST_LOGIC_OP_CLEAR);
    bfGfxCmdList_setPolygonFillMode(list, BIFROST_POLYGON_MODE_FILL);

    for (int i = 0; i < BIFROST_GFX_RENDERPASS_MAX_ATTACHMENTS; ++i)
    {
      bfGfxCmdList_setColorWriteMask(list, i, BIFROST_COLOR_MASK_RGBA);
      bfGfxCmdList_setColorBlendOp(list, i, BIFROST_BLEND_OP_ADD);
      bfGfxCmdList_setBlendSrc(list, i, BIFROST_BLEND_FACTOR_SRC_ALPHA);
      bfGfxCmdList_setBlendDst(list, i, BIFROST_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
      bfGfxCmdList_setAlphaBlendOp(list, i, BIFROST_BLEND_OP_ADD);
      bfGfxCmdList_setBlendSrcAlpha(list, i, BIFROST_BLEND_FACTOR_ONE);
      bfGfxCmdList_setBlendDstAlpha(list, i, BIFROST_BLEND_FACTOR_ZERO);
    }

    const auto SetupStencilState = [&list](BifrostStencilFace face) {
      bfGfxCmdList_setStencilFailOp(list, face, BIFROST_STENCIL_OP_KEEP);
      bfGfxCmdList_setStencilPassOp(list, face, BIFROST_STENCIL_OP_KEEP);
      bfGfxCmdList_setStencilDepthFailOp(list, face, BIFROST_STENCIL_OP_KEEP);
      bfGfxCmdList_setStencilCompareOp(list, face, BIFROST_STENCIL_OP_KEEP);
      bfGfxCmdList_setStencilCompareMask(list, face, 0xFF);
      bfGfxCmdList_setStencilWriteMask(list, face, 0xFF);
      bfGfxCmdList_setStencilReference(list, face, 0xFF);
    };

    SetupStencilState(BIFROST_STENCIL_FACE_FRONT);
    SetupStencilState(BIFROST_STENCIL_FACE_BACK);

    // bfGfxCmdList_setDynamicStates(list, BIFROST_PIPELINE_DYNAMIC_VIEWPORT | BIFROST_PIPELINE_DYNAMIC_SCISSOR);
    bfGfxCmdList_setDynamicStates(list, 0x0);
    const float depths[] = {0.0f, 1.0f};
    bfGfxCmdList_setViewport(list, 0.0f, 0.0f, 0.0f, 0.0f, depths);
    bfGfxCmdList_setScissorRect(list, 0, 0, 1, 1);
    const float blend_constants[] = {1.0f, 1.0f, 1.0f, 1.0f};
    bfGfxCmdList_setBlendConstants(list, blend_constants);
    bfGfxCmdList_setLineWidth(list, 1.0f);
    bfGfxCmdList_setDepthClampEnabled(list, bfFalse);
    bfGfxCmdList_setDepthBoundsTestEnabled(list, bfFalse);
    bfGfxCmdList_setDepthBounds(list, 0.0f, 1.0f);
    bfGfxCmdList_setDepthBiasConstantFactor(list, 0.0f);
    bfGfxCmdList_setDepthBiasClamp(list, 0.0f);
    bfGfxCmdList_setDepthBiasSlopeFactor(list, 0.0f);
    bfGfxCmdList_setMinSampleShading(list, 0.0f);
    bfGfxCmdList_setSampleMask(list, 0xFFFFFFFF);

    return list;
  }

  return nullptr;
}

bfTextureHandle bfGfxDevice_requestSurface(bfGfxCommandListHandle command_list)
{
  return &command_list->window->swapchain.img_list.images[command_list->window->image_index];
}

bfDeviceLimits bfGfxDevice_limits(bfGfxDeviceHandle self)
{
  VkPhysicalDeviceLimits& vk_limits = self->parent->device_properties.limits;
  bfDeviceLimits          limits;

  limits.uniform_buffer_offset_alignment = vk_limits.minUniformBufferOffsetAlignment;

  return limits;
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
    init_info.enabledExtensionCount   = static_cast<uint32_t>(bfCArraySize(INSTANCE_EXT_NAMES));
    init_info.ppEnabledExtensionNames = INSTANCE_EXT_NAMES;

    const VkResult err = vkCreateInstance(&init_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->instance);

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

  bfBool32 gfxContextSetDebugCallback(bfGfxContextHandle self, PFN_vkDebugReportCallbackEXT callback)
  {
#if BIFROST_USE_DEBUG_CALLBACK
    VkDebugReportCallbackCreateInfoEXT create_info;
    create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    create_info.pNext       = nullptr;
    create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    create_info.pfnCallback = callback;
    create_info.pUserData   = nullptr;

    PFN_vkCreateDebugReportCallbackEXT func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
     self->instance, "vkCreateDebugReportCallbackEXT");

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
      device.parent = self;
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
     .dpy = self->params->platform_module,
     .window = self->params->platform_window};

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

  const char* gfxContextCreateLogicalDevice(bfGfxContextHandle self)
  {
    static const char* const DEVICE_EXT_NAMES[] =
     {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
     };  // I should be checking if the extensions are supported.

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

    VkPhysicalDeviceFeatures deviceFeatures;  // I should be checking if the features exist for the device in the first place.
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
    device_info.enabledExtensionCount   = uint32_t(bfCArraySize(DEVICE_EXT_NAMES));
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

    MaterialPoolCreateParams create_material_pool;
    create_material_pool.logical_device        = self->logical_device;
    create_material_pool.num_textures_per_link = 32;
    create_material_pool.num_uniforms_per_link = 16;
    create_material_pool.num_descsets_per_link = 8;

    self->logical_device->descriptor_pool  = MaterialPool_new(&create_material_pool);
    self->logical_device->cached_resources = nullptr;

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
    VkPoolAllocatorCtor(&self->logical_device->device_memory_allocator, self->logical_device);
    return nullptr;
  }

  const char* gfxContextInitCommandPool(bfGfxContextHandle self, uint16_t thread_index)
  {
    assert(thread_index == 0 && "Current implemetation only supports one thread currently.");

    const VulkanPhysicalDevice* const phys_device    = self->physical_device;
    const bfGfxDeviceHandle           logical_device = self->logical_device;

    // This should be per thread.
    VkCommandPoolCreateInfo cmd_pool_info;
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.pNext = nullptr;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |            // Since these are short lived buffers
                          VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;  // Since I Reuse the Buffer each frame
    cmd_pool_info.queueFamilyIndex = phys_device->queue_list.graphics_family_index;

    const VkResult error = vkCreateCommandPool(
     logical_device->handle,
     &cmd_pool_info,
     BIFROST_VULKAN_CUSTOM_ALLOCATOR,
     &self->command_pools[thread_index]);

    return error ? "Failed to create command pool" : nullptr;
  }

  const char* gfxContextInitSemaphores(bfGfxContextHandle self, VulkanWindow& window)
  {
    VkSemaphore* const semaphores[] = {&window.is_image_available, &window.is_render_done};

    VkSemaphoreCreateInfo semaphore_create_info;
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = NULL;
    semaphore_create_info.flags = 0;

    for (auto semaphore : semaphores)
    {
      const VkResult err = vkCreateSemaphore(
       self->logical_device->handle,
       &semaphore_create_info,
       BIFROST_VULKAN_CUSTOM_ALLOCATOR,
       semaphore);

      if (err)
      {
        return "Failed to create a Semaphore";
      }
    }

    return nullptr;
  }

  VkSurfaceFormatKHR gfxContextFindSurfaceFormat(const VkSurfaceFormatKHR* const formats, const uint32_t num_formats);
  VkPresentModeKHR   gfxFindSurfacePresentMode(const VkPresentModeKHR* const present_modes,
                                               const uint32_t                num_present_modes);

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

  VkExtent2D gfxFindSurfaceExtents(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

  bool gfxContextInitSwapchain(bfGfxContextHandle self, VulkanWindow& window)
  {
    auto& swapchain_info = window.swapchain_info;

    VulkanPhysicalDevice* const physical_device = self->physical_device;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device->handle, window.surface, &swapchain_info.capabilities);

    const VkPresentModeKHR surface_present_mode = gfxFindSurfacePresentMode(
     swapchain_info.present_modes, swapchain_info.num_present_modes);
    const VkExtent2D surface_extents = gfxFindSurfaceExtents(swapchain_info.capabilities, 0, 0);

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
      physical_device->queue_list.graphics_family_index,
      physical_device->queue_list.present_family_index,
     };

    if (physical_device->queue_list.graphics_family_index != physical_device->queue_list.present_family_index)
    {
      // If the graphics and present queues are from different queue families,
      // we either have to explicitly transfer ownership of images between
      // the queues, or we have to create the swapchain with imageSharingMode
      // as VK_SHARING_MODE_CONCURRENT
      swapchain_ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
      swapchain_ci.queueFamilyIndexCount = 2;
      swapchain_ci.pQueueFamilyIndices   = queue_family_indices;
    }

    VkResult err = vkCreateSwapchainKHR(self->logical_device->handle, &swapchain_ci, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &window.swapchain.handle);

    window.swapchain.extents        = surface_extents;
    window.swapchain.present_mode   = surface_present_mode;
    window.swapchain_needs_creation = false;

    if (err)
    {
      std::printf("GfxContext_initSwapchain %s %s", "vkCreateSwapchainKHR", "Failed to Create Swapchain");
    }

    return true;
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
      image->tex_view        = bfCreateImageView2D(logical_device->handle, temp_images[i], swapchain->format.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
      image->tex_sampler     = VK_NULL_HANDLE;
      image->tex_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
      image->tex_format      = swapchain->format.format;
      image->tex_samples     = BIFROST_SAMPLE_1;
    }
  }

  void gfxContextInitCmdFences(bfGfxContextHandle self, VulkanWindow& window)
  {
    bfGfxDeviceHandle logical_device = self->logical_device;

    const std::uint32_t num_fences = window.swapchain.img_list.size;
    window.swapchain.fences        = allocN<VkFence>(num_fences);

    for (uint32_t i = 0; i < num_fences; ++i)
    {
      VkFenceCreateInfo fence_create_info;
      fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_create_info.pNext = NULL;
      fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

      vkCreateFence(logical_device->handle, &fence_create_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, window.swapchain.fences + i);
    }
  }

  void gfxContextCreateCommandBuffers(bfGfxContextHandle self, std::uint32_t num_buffers, VkCommandBuffer* result)
  {
    const bfGfxDeviceHandle device = self->logical_device;

    VkCommandBufferAllocateInfo cmd_alloc_info;
    cmd_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_alloc_info.pNext              = nullptr;
    cmd_alloc_info.commandPool        = self->command_pools[0];
    cmd_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_alloc_info.commandBufferCount = num_buffers;

    const VkResult error = vkAllocateCommandBuffers(device->handle, &cmd_alloc_info, result);
    assert(error == VK_SUCCESS);
  }

  struct TempCommandBuffer final
  {
    bfGfxContextHandle context;
    VkCommandBuffer    handle;
  };

  VkCommandBuffer* gfxContextCreateCommandBuffers(bfGfxContextHandle self, std::uint32_t num_buffers)
  {
    VkCommandBuffer* result = allocN<VkCommandBuffer>(num_buffers);
    gfxContextCreateCommandBuffers(self, num_buffers, result);
    return result;
  }

  void gfxContextDestroyCommandBuffers(bfGfxContextHandle self, std::uint32_t num_buffers, VkCommandBuffer* buffers, bool free_array = true)
  {
    if (buffers)
    {
      bfGfxDeviceHandle logical_device = self->logical_device;
      vkFreeCommandBuffers(logical_device->handle, self->command_pools[0], num_buffers, buffers);

      if (free_array)
      {
        freeN(buffers);
      }
    }
  }

  TempCommandBuffer gfxContextBeginTransientCommandBuffer(bfGfxContextHandle self)
  {
    VkCommandBuffer result;
    gfxContextCreateCommandBuffers(self, 1, &result);

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext            = nullptr;
    begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(result, &begin_info);

    return {self, result};
  }

  void gfxContextEndTransientCommandBuffer(TempCommandBuffer buffer, BifrostGfxQueueType queue_type = BIFROST_GFX_QUEUE_GRAPHICS, bool wait_for_finish = true)
  {
    const VkQueue queue = buffer.context->logical_device->queues[queue_type];

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
      gfxContextDestroyCommandBuffers(buffer.context, 1, &buffer.handle, false);
    }  // else the caller is responsible for freeing
  }

  void gfxContextInitCmdBuffers(bfGfxContextHandle self, VulkanWindow& window)
  {
    window.swapchain.command_buffers = gfxContextCreateCommandBuffers(self, window.swapchain.img_list.size);
  }

  void gfxContextDestroyCmdBuffers(bfGfxContextHandle self, VulkanSwapchain* swapchain)
  {
    gfxContextDestroyCommandBuffers(self, swapchain->img_list.size, swapchain->command_buffers);
    swapchain->command_buffers = nullptr;
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

  VkPresentModeKHR gfxFindSurfacePresentMode(const VkPresentModeKHR* const present_modes,
                                             const uint32_t                num_present_modes)
  {
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;  // Guaranteed to exist according to the standard.

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

  VkBool32 memory_type_from_properties(VkPhysicalDeviceMemoryProperties mem_props, uint32_t typeBits, VkFlags requirements_mask, uint32_t* typeIndex)
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
}  // namespace

/* Buffers */
bfBufferHandle bfGfxDevice_newBuffer(bfGfxDeviceHandle self_, const bfBufferCreateParams* params)
{
  bfBufferHandle self = new bfBuffer();

  BifrostGfxObjectBase_ctor(&self->super, BIFROST_GFX_OBJECT_BUFFER);

  self->alloc_pool            = &self_->device_memory_allocator;
  self->alloc_info.mapped_ptr = NULL;
  self->real_size             = params->allocation.size;

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

  VkResult error = vkCreateBuffer(self_->handle, &buffer_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->handle);
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
                        params->usage & BIFROST_BUF_PERSISTENTLY_MAPPED_BUFFER,
                        memory_type_index,
                        &self->alloc_info);
  vkBindBufferMemory(self_->handle, self->handle, self->alloc_info.handle, self->alloc_info.offset);

  return self;
}

bfBufferSize bfBuffer_size(bfBufferHandle self)
{
  return self->real_size;
}

void* bfBuffer_map(bfBufferHandle self, bfBufferSize offset, bfBufferSize size)
{
  assert(self->alloc_info.mapped_ptr == NULL && "Buffer_map attempt to map an already mapped buffer.");
  vkMapMemory(self->alloc_pool->m_LogicalDevice->handle, self->alloc_info.handle, offset, size, 0, &self->alloc_info.mapped_ptr);
  return self->alloc_info.mapped_ptr;
}

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
  const auto transient_cmd = gfxContextBeginTransientCommandBuffer(src->alloc_pool->m_LogicalDevice->parent->parent);
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
  vkUnmapMemory(self->alloc_pool->m_LogicalDevice->handle, self->alloc_info.handle);
  self->alloc_info.mapped_ptr = nullptr;
}

/* Shader Program + Module */
bfShaderModuleHandle bfGfxDevice_newShaderModule(bfGfxDeviceHandle self_, BifrostShaderType type)
{
  bfShaderModuleHandle self = new bfShaderModule();

  BifrostGfxObjectBase_ctor(&self->super, BIFROST_GFX_OBJECT_SHADER_MODULE);

  self->parent         = self_;
  self->type           = type;
  self->handle         = VK_NULL_HANDLE;
  self->entry_point[0] = '\0';

  return self;
}

bfShaderProgramHandle bfGfxDevice_newShaderProgram(bfGfxDeviceHandle self_, const bfShaderProgramCreateParams* params)
{
  bfShaderProgramHandle self = new bfShaderProgram();

  BifrostGfxObjectBase_ctor(&self->super, BIFROST_GFX_OBJECT_SHADER_PROGRAM);

  assert(params->num_desc_sets <= BIFROST_GFX_RENDERPASS_MAX_DESCRIPTOR_SETS);

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

BifrostShaderType bfShaderModule_type(bfShaderModuleHandle self)
{
  return self->type;
}

char* LoadFileIntoMemory(const char* const filename, long* out_size)
{
  FILE* f      = std::fopen(filename, "rb");  // NOLINT(android-cloexec-fopen)
  char* buffer = nullptr;

  if (f)
  {
    std::fseek(f, 0, SEEK_END);
    long file_size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);  //same as rewind(f);
    buffer = static_cast<char*>(std::malloc(file_size + 1));
    if (buffer)
    {
      std::fread(buffer, sizeof(char), file_size, f);
      buffer[file_size] = '\0';
    }
    else
    {
      file_size = 0;
    }

    *out_size = file_size;
    std::fclose(f);
  }

  return buffer;
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

void bfShaderProgram_addAttribute(bfShaderProgramHandle self, const char* name, uint32_t binding)
{
  (void)self;
  (void)name;
  (void)binding;
  /* NO-OP */
}

void bfShaderProgram_addUniformBuffer(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, BifrostShaderStageFlags stages)
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

void bfShaderProgram_addImageSampler(bfShaderProgramHandle self, const char* name, uint32_t set, uint32_t binding, uint32_t how_many, BifrostShaderStageFlags stages)
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

  bfDescriptorSetHandle self = new bfDescriptorSet();

  BifrostGfxObjectBase_ctor(&self->super, BIFROST_GFX_OBJECT_DESCRIPTOR_SET);

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
      self->num_writes > BIFROST_GFX_DESCRIPTOR_SET_MAX_WRITES)
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
    image_infos[i].imageLayout = textures[i]->tex_layout;
  }
}

void bfDescriptorSet_setUniformBuffers(bfDescriptorSetHandle self, uint32_t binding, uint32_t array_element_start, const bfBufferSize* offsets, const bfBufferSize* sizes, bfBufferHandle* buffers, uint32_t num_buffers)
{
  VkWriteDescriptorSet*   write        = bfDescriptorSet__checkForFlush(self, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, binding, array_element_start, num_buffers, 0, 0);
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
  bfTextureHandle self = new bfTexture();

  BifrostGfxObjectBase_ctor(&self->super, BIFROST_GFX_OBJECT_TEXTURE);

  self->parent          = self_;
  self->flags           = params->flags;
  self->image_type      = params->type;
  self->image_width     = params->width;
  self->image_height    = params->height;
  self->image_depth     = params->depth;
  self->image_miplevels = params->generate_mipmaps;
  self->tex_image       = VK_NULL_HANDLE;
  self->tex_memory      = VK_NULL_HANDLE;
  self->tex_view        = VK_NULL_HANDLE;
  self->tex_sampler     = VK_NULL_HANDLE;
  self->tex_layout      = VK_IMAGE_LAYOUT_UNDEFINED;
  self->tex_format      = bfVkConvertFormat(params->format);
  self->tex_samples     = BIFROST_SAMPLE_1;

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

  VkPipelineStageFlagBits src_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlagBits dst_flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

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

  if (self->flags & BIFROST_TEX_IS_DEPTH_ATTACHMENT)
  {
    aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;

    if (self->flags & BIFROST_TEX_IS_STENCIL_ATTACHMENT)
    {
      aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  }
  else if (self->flags & BIFROST_TEX_IS_COLOR_ATTACHMENT)
  {
    aspects |= VK_IMAGE_ASPECT_COLOR_BIT;
  }
  else  // Sampled Texture
  {
    aspects |= VK_IMAGE_ASPECT_COLOR_BIT;
  }

  return aspects;
}

static void bfTexture__setLayout(bfTextureHandle self, VkImageLayout layout)
{
  const auto transient_cmd = gfxContextBeginTransientCommandBuffer(self->parent->parent->parent);
  setImageLayout(transient_cmd.handle, self->tex_image, bfTexture__aspect(self), self->tex_layout, layout, self->image_miplevels);
  gfxContextEndTransientCommandBuffer(transient_cmd);
  self->tex_layout = layout;
}

static void bfTexture__createImage(bfTextureHandle self)
{
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
  create_image.tiling                = VK_IMAGE_TILING_OPTIMAL;  // TODO
  create_image.usage                 = 0x0;
  create_image.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
  create_image.queueFamilyIndexCount = 0;
  create_image.pQueueFamilyIndices   = nullptr;
  create_image.initialLayout         = self->tex_layout;

  if (self->flags & BIFROST_TEX_IS_MULTI_QUEUE)
  {
    create_image.sharingMode = VK_SHARING_MODE_CONCURRENT;
  }

  // TODO: Make this a function? BGN
  if (self->flags & BIFROST_TEX_IS_TRANSFER_SRC || self->image_miplevels > 1)
  {
    create_image.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if (self->flags & BIFROST_TEX_IS_TRANSFER_DST)
  {
    create_image.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  if (self->flags & BIFROST_TEX_IS_SAMPLED)
  {
    create_image.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  if (self->flags & BIFROST_TEX_IS_STORAGE)
  {
    create_image.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
  }

  if (self->flags & BIFROST_TEX_IS_COLOR_ATTACHMENT)
  {
    create_image.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  }

  if (self->flags & (BIFROST_TEX_IS_DEPTH_ATTACHMENT | BIFROST_TEX_IS_STENCIL_ATTACHMENT))
  {
    create_image.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  }

  if (self->flags & BIFROST_TEX_IS_TRANSIENT)
  {
    create_image.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
  }

  if (self->flags & BIFROST_TEX_IS_INPUT_ATTACHMENT)
  {
    create_image.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
  }
  // TODO: Make this a function? END

  const VkResult error = vkCreateImage(self->parent->handle, &create_image, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->tex_image);
  assert(error == VK_SUCCESS);
}

static void bfTexture__allocMemory(bfTextureHandle self)
{
  // TODO: Switch to Pool Allocator?

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(self->parent->handle, self->tex_image, &memRequirements);

  VkMemoryAllocateInfo alloc_info;
  alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.pNext          = nullptr;
  alloc_info.allocationSize = memRequirements.size;
  // alloc_info.memoryTypeIndex = 0;

  memory_type_from_properties(self->parent->parent->memory_properties, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &alloc_info.memoryTypeIndex);

  vkAllocateMemory(self->parent->handle, &alloc_info, BIFROST_VULKAN_CUSTOM_ALLOCATOR, &self->tex_memory);
  vkBindImageMemory(self->parent->handle, self->tex_image, self->tex_memory, 0);
}

bfBool32 bfTexture_loadFile(bfTextureHandle self, const char* file)
{
  int      num_components = 0;
  stbi_uc* texture_data   = stbi_load(file, &self->image_width, &self->image_height, &num_components, STBI_rgb_alpha);

  if (texture_data)
  {
    assert(num_components == 4);
    bfTexture_loadData(self, reinterpret_cast<const char*>(texture_data), self->image_width * self->image_height * num_components * sizeof(char));
    stbi_image_free(texture_data);

    return bfTrue;
  }

  return bfFalse;
}

void bfTexture_loadBuffer(bfTextureHandle self, bfBufferHandle buffer)
{
  bfTexture__setLayout(self, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  const auto transient_cmd = gfxContextBeginTransientCommandBuffer(self->parent->parent->parent);
  {
    VkBufferImageCopy region;

    region.bufferOffset                    = 0;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageSubresource.aspectMask     = bfTexture__aspect(self);
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
     uint32_t(self->image_width),
     uint32_t(self->image_height),
     uint32_t(self->image_depth)};

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
    int32_t mip_width  = int32_t(self->image_width);
    int32_t mip_height = int32_t(self->image_height);

    const auto copy_cmds = gfxContextBeginTransientCommandBuffer(self->parent->parent->parent);
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

    self->tex_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  else
  {
    bfTexture__setLayout(self, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

bfBool32 bfTexture_loadData(bfTextureHandle self, const char* pixels, size_t pixels_length)
{
  const bool is_indefinite =
   self->image_width == BIFROST_TEXTURE_UNKNOWN_SIZE ||
   self->image_height == BIFROST_TEXTURE_UNKNOWN_SIZE ||
   self->image_depth == BIFROST_TEXTURE_UNKNOWN_SIZE;

  assert(!is_indefinite && "Texture_setData: The texture dimensions should be defined by this point.");

  self->image_miplevels = self->image_miplevels ? 1 + uint32_t(std::floor(std::log2(float(std::max(std::max(self->image_width, self->image_height), self->image_depth))))) : 1;

  bfTexture__createImage(self);
  bfTexture__allocMemory(self);
  self->tex_view = bfCreateImageView2D(self->parent->handle, self->tex_image, self->tex_format, bfTexture__aspect(self), self->image_miplevels);

  if (pixels)
  {
    bfBufferCreateParams buffer_params;
    buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
    buffer_params.allocation.size       = pixels_length;
    buffer_params.usage                 = BIFROST_BUF_TRANSFER_SRC;

    const bfBufferHandle staging_buffer = bfGfxDevice_newBuffer(self->parent, &buffer_params);

    bfBuffer_map(staging_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
    bfBuffer_copyCPU(staging_buffer, 0, pixels, pixels_length);
    bfBuffer_unMap(staging_buffer);

    bfTexture_loadBuffer(self, staging_buffer);
    bfGfxDevice_release(self->parent, staging_buffer);
  }

  return bfTrue;
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
  bfVertexLayoutSetHandle self = new bfVertexLayoutSet();
  std::memset(self, 0x0, sizeof(bfVertexLayoutSet));
  return self;
}

static void bfVertexLayout_addXBinding(bfVertexLayoutSetHandle self, uint32_t binding, uint32_t sizeof_vertex, VkVertexInputRate input_rate)
{
  assert(self->num_attrib_bindings < BIFROST_GFX_VERTEX_LAYOUT_MAX_BINDINGS);

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

void bfVertexLayout_addVertexLayout(bfVertexLayoutSetHandle self, uint32_t binding, BifrostVertexFormatAttribute format, uint32_t offset)
{
  assert(self->num_attrib_bindings < BIFROST_GFX_VERTEX_LAYOUT_MAX_BINDINGS);

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

void UpdateResourceFrame(bfGfxContextHandle ctx, BifrostGfxObjectBase* obj)
{
  obj->last_frame_used = ctx->frame_count;
}
