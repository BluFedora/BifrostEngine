// TODO(SR): Update glfw to version 3.3

#include "bifrost/bifrost_version.h"
#include <bifrost/bifrost.hpp>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream> /*  */
#include <utility>

// [https://www.glfw.org/docs/latest/group__native.html]
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#undef GLFW_EXPOSE_NATIVE_WIN32

class TestClass
{
 public:
  int         x;
  std::string y;

  TestClass(int f, std::string msg = "") :
    x(f),
    y{std::move(msg)}
  {
    try
    {
      std::cout << "TestClass(f = " << x << ")\n";
    }
    catch (...)
    {
    }
  }

  [[nodiscard]] const char* printf(float h) const
  {
    std::cout << "h = " << h << ", my var = " << x << "\n";
    return "__ Return from printf __";
  }

  void myRandomFn(int hello) const
  {
    std::cout << "(" << x << ") Hello is " << hello << "\n";
  }

  void myRandomFn2(int hello)
  {
    std::cout << "Changing x to: " << hello << "\n";
    x = hello;
  }

  [[nodiscard]] const std::string& getY() const
  {
    return y;
  }

  void setY(const std::string& val)
  {
    y = val;
  }

  ~TestClass() noexcept
  {
    try
    {
      std::cout << "~TestClass(" << x << ")\n";
    }
    catch (...)
    {
    }
  }
};

static void userErrorFn(struct BifrostVM_t* vm, BifrostVMError err, int line_no, const char* message)
{
  (void)vm;
  (void)line_no;
  if (err == BIFROST_VM_ERROR_STACK_TRACE_BEGIN || err == BIFROST_VM_ERROR_STACK_TRACE_END)
  {
    printf("### ------------ ERROR ------------ ###\n");
  }
  else
  {
    printf("%s", message);
  }
}

void testFN()
{
  std::cout << "does this work?" << std::endl;
}

static const char source[] = R"(
  import "main" for TestClass, cppFn;

  func hello()
  {
    print "Hello from another module.";
  }

  var t = new TestClass.ctor(28);
  print "ret from t = " +  t:printf(54);

  hello();

  class GameState
  {
    static var i = 2;
  };

  func update()
  {
    // print "I am updating!" + GameState.i;
    // GameState.i = GameState.i + 1;
    // t = nil;
  }

  func callMeFromCpp(arg0, arg1, arg2)
  {
    print "You passed in: " + arg0 + ", " + arg1 + ", "  + arg2;
    cppFn();
  }
)";

struct BifrostEngineCreateParams
{
  const char*   app_name;
  std::uint32_t app_version;
  std::uint32_t width;
  std::uint32_t height;
  void*         render_module;
  void*         render_window;
};

class BaseRenderer
{
  struct BasicVertex final
  {
    float         pos[4];
    unsigned char color[4];
    float         uv[2];
  };

 private:
  bfGfxContextHandle      m_GfxBackend;
  bfGfxDeviceHandle       m_GfxDevice;
  bfGfxCommandListHandle  m_MainCmdList;
  bfTextureHandle         m_MainSurface;
  bfVertexLayoutSetHandle m_BasicVertexLayout;
  bfBufferHandle          m_VertexBuffer;
  bfBufferHandle          m_IndexBuffer;
  bfTextureHandle         m_TestTexture;
  bfShaderModuleHandle    m_ShaderModuleV;
  bfShaderModuleHandle    m_ShaderModuleF;
  bfShaderProgramHandle   m_ShaderProgram;
  bfDescriptorSetHandle   m_TestMaterial;

 public:
  BaseRenderer() :
    m_GfxBackend{nullptr},
    m_GfxDevice{nullptr},
    m_MainCmdList{nullptr},
    m_MainSurface{nullptr},
    m_BasicVertexLayout{nullptr},
    m_VertexBuffer{nullptr},
    m_IndexBuffer{nullptr},
    m_TestTexture{nullptr},
    m_ShaderModuleV{nullptr},
    m_ShaderModuleF{nullptr},
    m_ShaderProgram{nullptr},
    m_TestMaterial{nullptr}
  {
  }

  void startup(const bfGfxContextCreateParams& gfx_create_params, uint32_t width, uint32_t height)
  {
    m_GfxBackend = bfGfxContext_new(&gfx_create_params);
    m_GfxDevice  = bfGfxContext_device(m_GfxBackend);

    bfGfxContext_onResize(m_GfxBackend, width, height);

    m_BasicVertexLayout = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(m_BasicVertexLayout, 0, sizeof(BasicVertex));
    bfVertexLayout_addVertexLayout(m_BasicVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(BasicVertex, pos));
    bfVertexLayout_addVertexLayout(m_BasicVertexLayout, 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(BasicVertex, color));
    bfVertexLayout_addVertexLayout(m_BasicVertexLayout, 0, BIFROST_VFA_FLOAT32_2, offsetof(BasicVertex, uv));

    bfBufferCreateParams buffer_params;
    buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
    buffer_params.allocation.size       = sizeof(BasicVertex) * 4;
    buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

    m_VertexBuffer = bfGfxDevice_newBuffer(m_GfxDevice, &buffer_params);

    buffer_params.allocation.size = sizeof(std::uint16_t) * 6;
    buffer_params.usage           = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_INDEX_BUFFER;

    m_IndexBuffer = bfGfxDevice_newBuffer(m_GfxDevice, &buffer_params);

    static const BasicVertex VERTEX_DATA[] =
     {
      {{-0.5f, -0.5f, 0.0f, 1.0f}, {255, 255, 255, 255}, {0.0f, 0.0f}},
      {{+0.5f, -0.5f, 0.0f, 1.0f}, {255, 255, 255, 255}, {1.0f, 0.0f}},
      {{+0.5f, +0.5f, 0.0f, 1.0f}, {255, 0, 255, 255}, {1.0f, 1.0f}},
      {{-0.5f, +0.5f, 0.0f, 1.0f}, {255, 255, 255, 255}, {0.0f, 1.0f}},
     };

    static const uint16_t INDEX_DATA[] = {0u, 1u, 2u, 3u, 2u, 0u};

    void* vertex_buffer_ptr = bfBuffer_map(m_VertexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
    void* index_buffer_ptr  = bfBuffer_map(m_IndexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);

    std::memcpy(vertex_buffer_ptr, VERTEX_DATA, sizeof(VERTEX_DATA));
    std::memcpy(index_buffer_ptr, INDEX_DATA, sizeof(INDEX_DATA));

    bfBuffer_unMap(m_VertexBuffer);
    bfBuffer_unMap(m_IndexBuffer);

    bfTextureCreateParams      create_texture = bfTextureCreateParams_init2D(BIFROST_TEXTURE_UNKNOWN_SIZE, BIFROST_TEXTURE_UNKNOWN_SIZE, BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM);
    bfTextureSamplerProperties sampler        = bfTextureSamplerProperties_init(BIFROST_SFM_NEAREST, BIFROST_SAM_REPEAT);

    create_texture.generate_mipmaps = bfFalse;

    m_TestTexture = bfGfxDevice_newTexture(m_GfxDevice, &create_texture);
    bfTexture_loadFile(m_TestTexture, "../assets/texture.png");
    bfTexture_setSampler(m_TestTexture, &sampler);

    bfShaderProgramCreateParams create_shader;
    create_shader.debug_name    = "My Test Shader";
    create_shader.num_desc_sets = 1;

    m_ShaderModuleV = bfGfxDevice_newShaderModule(m_GfxDevice, BIFROST_SHADER_TYPE_VERTEX);
    m_ShaderModuleF = bfGfxDevice_newShaderModule(m_GfxDevice, BIFROST_SHADER_TYPE_FRAGMENT);
    m_ShaderProgram = bfGfxDevice_newShaderProgram(m_GfxDevice, &create_shader);

    bfShaderModule_loadFile(m_ShaderModuleV, "../assets/basic_material.vert.spv");
    bfShaderModule_loadFile(m_ShaderModuleF, "../assets/basic_material.frag.spv");

    bfShaderProgram_addModule(m_ShaderProgram, m_ShaderModuleV);
    bfShaderProgram_addModule(m_ShaderProgram, m_ShaderModuleF);

    bfShaderProgram_addImageSampler(m_ShaderProgram, "u_DiffuseTexture", 0, 0, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    // bfShaderProgram_addUniformBuffer(m_ShaderProgram, "u_ModelTransform", 0, 1, 1, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(m_ShaderProgram);

    m_TestMaterial = bfShaderProgram_createDescriptorSet(m_ShaderProgram, 0);

    bfDescriptorSet_setCombinedSamplerTextures(m_TestMaterial, 0, 0, &m_TestTexture, 1);
    bfDescriptorSet_flushWrites(m_TestMaterial);
  }

  [[nodiscard]] bool frameBegin()
  {
    if (bfGfxContext_beginFrame(m_GfxBackend, -1))
    {
      bfGfxCommandListCreateParams thread_command_list{0, -1};

      m_MainCmdList = bfGfxContext_requestCommandList(m_GfxBackend, &thread_command_list);

      if (m_MainCmdList)
      {
        m_MainSurface = bfGfxDevice_requestSurface(m_MainCmdList);

        return bfGfxCmdList_begin(m_MainCmdList);
      }
    }

    return false;
  }

  void frameUpdate()
  {
    bfAttachmentInfo main_surface;
    main_surface.texture      = m_MainSurface;
    main_surface.final_layout = BIFROST_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    main_surface.may_alias    = bfFalse;

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x1);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, 0x1);
    bfRenderpassInfo_setClearOps(&renderpass_info, 0x1);
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStoreOps(&renderpass_info, 0x1);
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, 0x1);
    bfRenderpassInfo_addAttachment(&renderpass_info, &main_surface);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x1);
    bfRenderpassInfo_addColorOut(&renderpass_info, 0, 0, BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // bfRenderpassInfo_addDepthOut(&renderpass_info, 0, 0, BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    // bfRenderpassInfo_addInput(&renderpass_info, 0x1);
    // bfRenderpassInfo_addDependencies(&renderpass_info, 0x1);

    BifrostClearValue clear_colors;
    clear_colors.color.float32[0] = 1.0f;
    clear_colors.color.float32[1] = 0.6f;
    clear_colors.color.float32[2] = 1.0f;
    clear_colors.color.float32[3] = 1.0f;

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info);
    bfGfxCmdList_setClearValues(m_MainCmdList, &clear_colors);
    bfGfxCmdList_setAttachments(m_MainCmdList, &m_MainSurface);
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);

    bfGfxCmdList_beginRenderpass(m_MainCmdList);
    {
      uint64_t buffer_offset = 0;

      bfGfxCmdList_bindVertexDesc(m_MainCmdList, m_BasicVertexLayout);
      bfGfxCmdList_bindVertexBuffers(m_MainCmdList, 0, &m_VertexBuffer, 1, &buffer_offset);
      bfGfxCmdList_bindIndexBuffer(m_MainCmdList, m_IndexBuffer, 0, BIFROST_INDEX_TYPE_UINT16);
      bfGfxCmdList_bindProgram(m_MainCmdList, m_ShaderProgram);
      bfGfxCmdList_bindDescriptorSets(m_MainCmdList, 0, &m_TestMaterial, 1);
      bfGfxCmdList_drawIndexed(m_MainCmdList, 6, 0, 0);
    }
    bfGfxCmdList_endRenderpass(m_MainCmdList);
  }

  void frameEnd() const
  {
    bfGfxCmdList_end(m_MainCmdList);
    bfGfxCmdList_submit(m_MainCmdList);
    bfGfxContext_endFrame(m_GfxBackend);
  }

  void cleanup()
  {
    bfGfxDevice_flush(m_GfxDevice);
    bfGfxDevice_release(m_GfxDevice, m_TestMaterial);
    bfGfxDevice_release(m_GfxDevice, m_ShaderModuleF);
    bfGfxDevice_release(m_GfxDevice, m_ShaderModuleV);
    bfGfxDevice_release(m_GfxDevice, m_ShaderProgram);
    bfGfxDevice_release(m_GfxDevice, m_TestTexture);
    bfGfxDevice_release(m_GfxDevice, m_VertexBuffer);
    bfGfxDevice_release(m_GfxDevice, m_IndexBuffer);
    bfVertexLayout_delete(m_BasicVertexLayout);
    bfGfxContext_delete(m_GfxBackend);
    m_GfxBackend = nullptr;
  }
};

class BifrostEngine : private bifrost::bfNonCopyMoveable<BifrostEngine>
{
 private:
  std::pair<int, const char**> m_CmdlineArgs;
  BaseRenderer                 m_Renderer;

 public:
  BifrostEngine(int argc, const char* argv[]) :
    m_CmdlineArgs{argc, argv},
    m_Renderer{}
  {
  }

  void init(const BifrostEngineCreateParams& params)
  {
    IBifrostDbgLogger logger_config{
     nullptr,
     [](void* data, const BifrostDbgLogInfo* info) {
       (void)data;

       if (info->level != BIFROST_LOGGER_LVL_POP)
       {
         static constexpr unsigned int TAB_SIZE = 4;

#if 0
         std::printf("%*c%s(%u): \n", TAB_SIZE * info->indent_level, ' ', info->func, info->line);
         std::printf("%*c", TAB_SIZE * info->indent_level * 2, ' ');
#else
         std::printf("%*c", TAB_SIZE * info->indent_level, ' ');

#endif
         std::vprintf(info->format, info->args);
         std::printf("\n");
       }
     }};

    bfLogger_init(&logger_config);

    bfLogPush("Engine Init of App: %s", params.app_name);

    const bfGfxContextCreateParams gfx_create_params = {
     params.app_name,
     params.app_version,
     params.render_module,
     params.render_window,
    };

    m_Renderer.startup(gfx_create_params, params.width, params.height);

    bfLogPop();
  }

  [[nodiscard]] bool beginFrame()
  {
    return m_Renderer.frameBegin();
  }

  void update()
  {
    m_Renderer.frameUpdate();
  }

  void endFrame() const
  {
    m_Renderer.frameEnd();
  }

  void deinit()
  {
    bfLogger_deinit();
    m_Renderer.cleanup();
  }
};

namespace ErrorCodes
{
  static constexpr int GLFW_FAILED_TO_INIT = -1;
  // static constexpr int GLAD_FAILED_TO_INIT = -2;
}  // namespace ErrorCodes

namespace bifrost::meta
{
  template<>
  const auto& Meta::registerMembers<TestClass>()
  {
    static auto member_ptrs = members(
     field("x", &TestClass::x),
     property("y", &TestClass::getY, &TestClass::setY),
     function("myRandomFn", &TestClass::myRandomFn),
     function("myRandomFn2", &TestClass::myRandomFn2));

    return member_ptrs;
  }
}  // namespace bifrost::meta

#include <filesystem>

int main(int argc, const char* argv[])  // NOLINT(bugprone-exception-escape)
{
  namespace bfmeta = bifrost::meta;
  using namespace bifrost;
  namespace bf = bifrost;

  static constexpr int INITIAL_WINDOW_SIZE[] = {1280, 720};

  const auto p = std::filesystem::current_path().string();

  std::printf("Bifrost Engine v%s\n", BIFROST_VERSION_STR);
  std::printf("Working Dir: %s\n", p.c_str());
  std::cout << __cplusplus << "\n";

  TestClass my_obj = {74, "This message will be in Y"};

  try
  {
    std::cout << "Meta Testing Bed: \n";

    for_each(meta::membersOf<TestClass>(), [&my_obj](const auto& member) {
      std::cout << member.name() << " : ";

      if constexpr (bfmeta::is_function_v<decltype(member)>)
      {
        member.call(my_obj, 6);
      }
      else
      {
        std::cout << member.get(my_obj) << std::endl;
      }
    });

    std::cout << "x = " << my_obj.x << "\n";

    std::cout << "Meta Testing End: \n\n";
  }
  catch (...)
  {
  }

  int error_code = 0;

  if (!glfwInit())
  {
    error_code = ErrorCodes::GLFW_FAILED_TO_INIT;
    return error_code;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  const auto main_window = glfwCreateWindow(INITIAL_WINDOW_SIZE[0], INITIAL_WINDOW_SIZE[1], "Bifrost Engine", nullptr, nullptr);
  glfwSetWindowSizeLimits(main_window, 300, 70, GLFW_DONT_CARE, GLFW_DONT_CARE);
  /*
  glfwMakeContextCurrent(main_window);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    error_code = ErrorCodes::GLAD_FAILED_TO_INIT;
    goto shutdown_glfw;  // NOLINT(hicpp-avoid-goto)
  }
  */

  std::cout << "\n\nScripting Language Test Begin\n";

  VMParams vm_params;
  vm_params.error_fn           = &userErrorFn;
  vm_params.print_fn           = [](BifrostVM*, const char* message) { std::cout << message << "\n"; };
  vm_params.min_heap_size      = 20;
  vm_params.heap_size          = 200;
  vm_params.heap_growth_factor = 0.1f;
  VM vm{vm_params};

  const BifrostVMClassBind clz_bind = bf::vmMakeClassBinding<TestClass>(
   "TestClass",
   bf::vmMakeCtorBinding<TestClass, int>(),
   bf::vmMakeMemberBinding<&TestClass::printf>("printf"));

  vm.stackResize(1);
  vm.moduleMake(0, "main");
  vm.moduleBind(0, clz_bind);
  vm.moduleBind<testFN>(0, "cppFn");

  const BifrostVMError err = vm.execInModule("main2", source, std::size(source));

  bfValueHandle update_fn = nullptr;

  if (!err)
  {
    vm.stackResize(1);
    vm.moduleLoad(0, "main2");
    vm.stackLoadVariable(0, 0, "callMeFromCpp");

    vm.call(0, 45, std::string("Hello from cpp") + "!!!", false);

    vm.moduleLoad(0, "main2");
    vm.stackLoadVariable(0, 0, "update");
    update_fn = vm.stackMakeHandle(0);
  }

  std::cout << "\n\nScripting Language Test End\n";

  {
    BifrostEngine engine{argc, argv};

    const BifrostEngineCreateParams params =
     {
      argv[0],
      0,
      INITIAL_WINDOW_SIZE[0],
      INITIAL_WINDOW_SIZE[1],
      GetModuleHandle(nullptr),
      glfwGetWin32Window(main_window),
     };

    engine.init(params);

    while (!glfwWindowShouldClose(main_window))
    {
      int window_width, window_height;
      glfwGetWindowSize(main_window, &window_width, &window_height);

      glfwPollEvents();

      if (engine.beginFrame())
      {
        engine.update();

        if (update_fn)
        {
          vm.stackResize(1);
          vm.stackLoadHandle(0, update_fn);

          if (vm.stackGetType(0) == BIFROST_VM_FUNCTION)
          {
            vm.call(0);
          }
        }

        engine.endFrame();
      }

      // glfwSwapBuffers(main_window);
    }

    engine.deinit();
  }

  vm.stackDestroyHandle(update_fn);

  // shutdown_glfw:
  glfwDestroyWindow(main_window);
  glfwTerminate();

  // shutdown_exit:
  return error_code;
}
