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

struct BifrostEngineCreateParams : public bfGfxContextCreateParams
{
  std::uint32_t width;
  std::uint32_t height;
};

static GLFWwindow* g_Window = nullptr;

class BaseRenderer
{
  struct BasicVertex final
  {
    float         pos[4];
    unsigned char color[4];
    float         uv[2];
  };

 private:
  bfGfxContextHandle           m_GfxBackend;
  bfGfxDeviceHandle            m_GfxDevice;
  bfGfxCommandListHandle       m_MainCmdList;
  bfTextureHandle              m_MainSurface;
  bfVertexLayoutSetHandle      m_BasicVertexLayout;
  bfBufferHandle               m_VertexBuffer;
  bfBufferHandle               m_IndexBuffer;
  bfBufferHandle               m_UniformBuffer;
  bfTextureHandle              m_TestTexture;
  bfShaderModuleHandle         m_ShaderModuleV;
  bfShaderModuleHandle         m_ShaderModuleF;
  bfShaderProgramHandle        m_ShaderProgram;
  bfDescriptorSetHandle        m_TestMaterial;
  bfDescriptorSetHandle        m_TestMaterial2;
  std::vector<bfGfxBaseHandle> m_Resources;
  Mat4x4                       m_ModelView;
  Camera                       m_Camera;
  float                        m_Time;

 public:
  BaseRenderer() :
    m_GfxBackend{nullptr},
    m_GfxDevice{nullptr},
    m_MainCmdList{nullptr},
    m_MainSurface{nullptr},
    m_BasicVertexLayout{nullptr},
    m_VertexBuffer{nullptr},
    m_IndexBuffer{nullptr},
    m_UniformBuffer{nullptr},
    m_TestTexture{nullptr},
    m_ShaderModuleV{nullptr},
    m_ShaderModuleF{nullptr},
    m_ShaderProgram{nullptr},
    m_TestMaterial{nullptr},
    m_TestMaterial2{nullptr},
    m_Resources{},
    m_ModelView{},
    m_Camera{},
    m_Time{0.0f}
  {
    Vec3f cam_pos = {0.0f, 0.0f, 4.0f, 1.0f};
    Vec3f cam_up  = {0.0f, 1.0f, 0.0f, 0.0f};

    Mat4x4_identity(&m_ModelView);
    Camera_init(&m_Camera, &cam_pos, &cam_up, 0.0f, 0.0f);
  }

  void startup(const bfGfxContextCreateParams& gfx_create_params)
  {
    m_GfxBackend = bfGfxContext_new(&gfx_create_params);
    m_GfxDevice  = bfGfxContext_device(m_GfxBackend);

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

    buffer_params.allocation.size = 0x100 * 2;
    buffer_params.usage           = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_UNIFORM_BUFFER;

    m_UniformBuffer = bfGfxDevice_newBuffer(m_GfxDevice, &buffer_params);

    static const BasicVertex VERTEX_DATA[] =
     {
      {{-0.5f, -0.5f, 0.0f, 1.0f}, {0, 255, 255, 255}, {0.0f, 0.0f}},
      {{+0.5f, -0.5f, 0.0f, 1.0f}, {255, 255, 0, 255}, {1.0f, 0.0f}},
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
    bfShaderProgram_addUniformBuffer(m_ShaderProgram, "u_ModelTransform", 0, 1, 1, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(m_ShaderProgram);

    m_TestMaterial = bfShaderProgram_createDescriptorSet(m_ShaderProgram, 0);
    m_TestMaterial2 = bfShaderProgram_createDescriptorSet(m_ShaderProgram, 0); 

    bfDescriptorSet_setCombinedSamplerTextures(m_TestMaterial, 0, 0, &m_TestTexture, 1);

    uint64_t offset = 0;
    uint64_t sizes  = sizeof(m_ModelView);
    bfDescriptorSet_setUniformBuffers(m_TestMaterial, 1, 0, &offset, &sizes, &m_UniformBuffer, 1);
    bfDescriptorSet_flushWrites(m_TestMaterial);

    offset += 0x100;

    bfDescriptorSet_setCombinedSamplerTextures(m_TestMaterial2, 0, 0, &m_TestTexture, 1);
    bfDescriptorSet_setUniformBuffers(m_TestMaterial2, 1, 0, &offset, &sizes, &m_UniformBuffer, 1);
    bfDescriptorSet_flushWrites(m_TestMaterial2);

    m_Resources.push_back(m_TestMaterial);
    m_Resources.push_back(m_TestMaterial2);
    m_Resources.push_back(m_ShaderModuleF);
    m_Resources.push_back(m_ShaderModuleV);
    m_Resources.push_back(m_ShaderProgram);
    m_Resources.push_back(m_TestTexture);
    m_Resources.push_back(m_VertexBuffer);
    m_Resources.push_back(m_IndexBuffer);
    m_Resources.push_back(m_UniformBuffer);
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
    // Camera BGN
    Mat4x4 rot, scl, trans;

    Mat4x4_initScalef(&scl, 1.4f, 1.4f, 1.0f);
    Mat4x4_initRotationf(&rot, 0.0f, 0.0f, m_Time * 20.0f);
    Mat4x4_initTranslatef(&trans, 0.0f, 3.0f, 1.0f);
    Mat4x4_mult(&rot, &scl, &m_ModelView);

    auto model2 = m_ModelView;

    Mat4x4_mult(&trans, &m_ModelView, &m_ModelView);

    const auto camera_move_speed = 0.05f;
    
    if (glfwGetKey(g_Window, GLFW_KEY_W) == GLFW_PRESS)
    {
      Camera_moveForward(&m_Camera, camera_move_speed);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_A) == GLFW_PRESS)
    {
      Camera_moveLeft(&m_Camera, camera_move_speed);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_S) == GLFW_PRESS)
    {
      Camera_moveBackward(&m_Camera, camera_move_speed);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_D) == GLFW_PRESS)
    {
      Camera_moveRight(&m_Camera, camera_move_speed);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_Q) == GLFW_PRESS)
    {
      Camera_moveUp(&m_Camera, camera_move_speed);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_E) == GLFW_PRESS)
    {
      Camera_moveDown(&m_Camera, camera_move_speed);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_R) == GLFW_PRESS)
    {
      Camera_addPitch(&m_Camera, -0.01f);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_F) == GLFW_PRESS)
    {
      Camera_addPitch(&m_Camera, 0.01f);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_H) == GLFW_PRESS)
    {
      Camera_addYaw(&m_Camera, 0.01f);
    }

    if (glfwGetKey(g_Window, GLFW_KEY_G) == GLFW_PRESS)
    {
      Camera_addYaw(&m_Camera, -0.01f);
    }


    int width, height;
    glfwGetWindowSize(g_Window, &width, &height);

    Camera_onResize(&m_Camera, width, height);
    Camera_update(&m_Camera);

    // Model -> View -> Proj
    Mat4x4_mult(&m_Camera.view_cache, &m_ModelView, &m_ModelView);
    Mat4x4_mult(&m_Camera.proj_cache, &m_ModelView, &m_ModelView);

    Mat4x4_mult(&m_Camera.view_cache, &model2, &model2);
    Mat4x4_mult(&m_Camera.proj_cache, &model2, &model2);

    void* uniform_buffer_ptr = bfBuffer_map(m_UniformBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
    std::memcpy(uniform_buffer_ptr, &m_ModelView, sizeof(m_ModelView));
    std::memcpy((unsigned char*)uniform_buffer_ptr + 0x100, &model2, sizeof(model2));
    bfBuffer_unMap(m_UniformBuffer);

    // Camera END

    bfAttachmentInfo main_surface;
    main_surface.texture      = m_MainSurface;
    main_surface.final_layout = BIFROST_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    main_surface.may_alias    = bfFalse;

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setClearOps(&renderpass_info, 0x1);
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStoreOps(&renderpass_info, 0x1);
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, 0x0);
    bfRenderpassInfo_addAttachment(&renderpass_info, &main_surface);
    bfRenderpassInfo_addColorOut(&renderpass_info, 0, 0, BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    // bfRenderpassInfo_addDepthOut(&renderpass_info, 0, 0, BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    // bfRenderpassInfo_addInput(&renderpass_info, 0x1);
    // bfRenderpassInfo_addDependencies(&renderpass_info, 0x1);

    BifrostClearValue clear_colors;
    clear_colors.color.float32[0] = 0.8f;
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
      bfGfxCmdList_bindDescriptorSets(m_MainCmdList, 0, &m_TestMaterial2, 1);
      bfGfxCmdList_drawIndexed(m_MainCmdList, 6, 0, 0);
    }
    bfGfxCmdList_endRenderpass(m_MainCmdList);
  }

  void frameEnd()
  {
    bfGfxCmdList_end(m_MainCmdList);
    bfGfxCmdList_submit(m_MainCmdList);
    bfGfxContext_endFrame(m_GfxBackend);

    m_Time += 1.0f / 60.0f;
  }

  void cleanup()
  {
    bfGfxDevice_flush(m_GfxDevice);

    for (auto resource : m_Resources)
    {
      bfGfxDevice_release(m_GfxDevice, resource);
    }

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

    m_Renderer.startup(params);

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

  void endFrame()
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
#include <thread>
#include <chrono>

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


  g_Window = main_window;


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
      {
       argv[0],
       0,
      GetModuleHandle(nullptr),
      glfwGetWin32Window(main_window),
      },
       INITIAL_WINDOW_SIZE[0],
       INITIAL_WINDOW_SIZE[1],
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

      std::this_thread::sleep_for(std::chrono::milliseconds(16));
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

#if 0
#include <imgui/imgui.h>

static const char* GLFW_ClipboardGet(void* user_data);
static void        GLFW_ClipboardSet(void* user_data, const char* text);

static void GLFW_onKeyChanged(GLFWwindow* window, int key, int scancode, int action, int mods)
{
}

static void GLFW_onMousePosChanged(GLFWwindow* window, double xpos, double ypos)
{
}

static void GLFW_onMouseButtonChanged(GLFWwindow* window, int button, int action, int mods)
{
}

void GLFW_onWindowFileDropped(GLFWwindow* window, int count, const char** paths)
{
}

void GLFW_onJoystickStateChanged(int joy, int event)
{
}

static void GLFW_onWindowSizeChanged(GLFWwindow* window, int width, int height)
{
}

static void GLFW_onWindowCharacterInput(GLFWwindow* window, unsigned int codepoint, int mods)
{
}

static void GLFW_onScrollWheel(GLFWwindow* window, double xoffset, double yoffset)
{
}

static void GLFW_onWindowIconify(GLFWwindow* window, int iconified)
{
}

void GLFW_onWindowFocusChanged(GLFWwindow* window, int focused)
{
}

static void GLFW_onWindowClose(GLFWwindow* window)
{
  // glfwSetWindowShouldClose(window, GLFW_FALSE);
}

static void ImGuiSetup(GLFWwindow* window, BifrostEngine* engine)
{
  ImGuiIO& io            = ImGui::GetIO();
  io.BackendPlatformName = "Bifrost GLFW Backend";
  io.BackendRendererName = "Bifrost GLFW Backend";

  // Keyboard Setup
  io.KeyMap[ImGuiKey_Tab]         = GLFW_KEY_TAB;
  io.KeyMap[ImGuiKey_LeftArrow]   = GLFW_KEY_LEFT;
  io.KeyMap[ImGuiKey_RightArrow]  = GLFW_KEY_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow]     = GLFW_KEY_UP;
  io.KeyMap[ImGuiKey_DownArrow]   = GLFW_KEY_DOWN;
  io.KeyMap[ImGuiKey_PageUp]      = GLFW_KEY_PAGE_UP;
  io.KeyMap[ImGuiKey_PageDown]    = GLFW_KEY_PAGE_DOWN;
  io.KeyMap[ImGuiKey_Home]        = GLFW_KEY_HOME;
  io.KeyMap[ImGuiKey_End]         = GLFW_KEY_END;
  io.KeyMap[ImGuiKey_Insert]      = GLFW_KEY_INSERT;
  io.KeyMap[ImGuiKey_Delete]      = GLFW_KEY_DELETE;
  io.KeyMap[ImGuiKey_Backspace]   = GLFW_KEY_BACKSPACE;
  io.KeyMap[ImGuiKey_Space]       = GLFW_KEY_SPACE;
  io.KeyMap[ImGuiKey_Enter]       = GLFW_KEY_ENTER;
  io.KeyMap[ImGuiKey_Escape]      = GLFW_KEY_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
  io.KeyMap[ImGuiKey_A]           = GLFW_KEY_A;
  io.KeyMap[ImGuiKey_C]           = GLFW_KEY_C;
  io.KeyMap[ImGuiKey_V]           = GLFW_KEY_V;
  io.KeyMap[ImGuiKey_X]           = GLFW_KEY_X;
  io.KeyMap[ImGuiKey_Y]           = GLFW_KEY_Y;
  io.KeyMap[ImGuiKey_Z]           = GLFW_KEY_Z;

  // Clipboard
  io.GetClipboardTextFn = GLFW_ClipboardGet;
  io.SetClipboardTextFn = GLFW_ClipboardSet;
  io.ClipboardUserData = window;

  glfwSetWindowUserPointer(window, engine);
  glfwSetKeyCallback(window, GLFW_onKeyChanged);
  glfwSetCursorPosCallback(window, GLFW_onMousePosChanged);
  glfwSetMouseButtonCallback(window, GLFW_onMouseButtonChanged);
  glfwSetDropCallback(window, GLFW_onWindowFileDropped);
  glfwSetJoystickCallback(GLFW_onJoystickStateChanged);
  glfwSetWindowSizeCallback(window, GLFW_onWindowSizeChanged);
  glfwSetCharModsCallback(window, GLFW_onWindowCharacterInput);
  glfwSetScrollCallback(window, GLFW_onScrollWheel);
  glfwSetWindowIconifyCallback(window, GLFW_onWindowIconify);
  glfwSetWindowFocusCallback(window, GLFW_onWindowFocusChanged);
  glfwSetWindowCloseCallback(window, GLFW_onWindowClose);
}

static const char* GLFW_ClipboardGet(void* user_data)
{
  return glfwGetClipboardString(static_cast<GLFWwindow*>(user_data));
}

static void GLFW_ClipboardSet(void* user_data, const char* text)
{
  glfwSetClipboardString(static_cast<GLFWwindow*>(user_data), text);
}
#endif