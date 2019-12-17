#include "bifrost/bifrost_version.h"

#include "bifrost/core/bifrost_game_state_machine.hpp"
#include "bifrost/memory/bifrost_freelist_allocator.hpp"
#include "bifrost/platform/bifrost_window_glfw.hpp"
#include "demo/game_state_layers/main_demo.hpp"
#include "imgui/imgui.h"
#include <bifrost/bifrost.hpp>
#include <bifrost_editor/bifrost_imgui_glfw.hpp>
#include <chrono>
#include <functional>
#include <glfw/glfw3.h>
#include <iostream> /*  */
#include <thread>
#include <utility>
#define GLFW_EXPOSE_NATIVE_WIN32
#include "bifrost/asset_io/test.h"
#include <glfw/glfw3native.h>
#undef GLFW_EXPOSE_NATIVE_WIN32

namespace bifrost
{
  class Entity : public BaseObject<Entity>
  {
   public:
    Entity()
    {
      std::printf("Created an entity\n");
    }
  };

  template<typename TDerived>
  class Component : public BaseObject<Component<TDerived>, TDerived>
  {
   public:
    Component() = default;
  };

  class SpriteComponent : public Component<SpriteComponent>
  {
   public:
    SpriteComponent()
    {
      std::printf("Created a SpriteComponent\n");
    }
  };

  class MeshComponent : public Component<MeshComponent>
  {
   public:
    MeshComponent()
    {
      std::printf("Created a MeshComponent\n");
    }
  };

  namespace meta
  {
    template<>
    const auto& Meta::registerMembers<Entity>()
    {
      static auto member_ptrs = members(
       class_info<Entity>("Entity"));

      return member_ptrs;
    }

    template<>
    const auto& Meta::registerMembers<SpriteComponent>()
    {
      static auto member_ptrs = members(
       class_info<SpriteComponent, Component<SpriteComponent>>("SpriteComponent"));

      return member_ptrs;
    }

    template<>
    const auto& Meta::registerMembers<MeshComponent>()
    {
      static auto member_ptrs = members(
       class_info<MeshComponent, Component<MeshComponent>>("MeshComponent"));

      return member_ptrs;
    }
  }  // namespace meta
}  // namespace bifrost

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
    return "__ Return from printf *^";
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
    std::printf("%s", message);
  }
}

void testFN()
{
  std::cout << "does this work?" << std::endl;
}

static const char source[] = R"(
  import "main"    for TestClass, cppFn, BigFunc, AnotherOne;
  import "bifrost" for Camera;
  import "std:io"  for print;

  static var cam = new Camera();

  var t = new TestClass.ctor(28);
  print("ret from t = " +  t:printf(54));

  hello();

  func hello()
  {
    print("Hello from another module.");
  }

  class GameState
  {
    static var i = 2;
  };

  func update()
  {
    cam:update(0.4);

    // BigFunc();
    // print AnotherOne();

    // print "I am updating!" + GameState.i;
    // GameState.i = GameState.i + 1;
    // t = nil;
  }

  func callMeFromCpp(arg0, arg1, arg2)
  {
    print("You passed in: " + arg0 + ", " + arg1 + ", "  + arg2);
    cppFn();
  }
)";

struct BifrostEngineCreateParams : public bfGfxContextCreateParams
{
  std::uint32_t width;
  std::uint32_t height;
};

static GLFWwindow* g_Window = nullptr;

class Model
{
  bfBufferHandle vertex_buffer = nullptr;
  size_t         num_vertices  = 0;

 public:
  void load(bfGfxDeviceHandle device, const char* file)
  {
    long  file_size;
    char* file_data;

    if ((file_data = LoadFileIntoMemory(file, &file_size)))
    {
      TEST_ModelData data = TEST_AssetIO_loadObj(file_data, (uint)file_size);
      num_vertices        = Array_size(&data.vertices);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE;
      buffer_params.allocation.size       = sizeof(BasicVertex) * num_vertices;
      buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

      vertex_buffer = bfGfxDevice_newBuffer(device, &buffer_params);

      void* vertex_buffer_ptr = bfBuffer_map(vertex_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE);

      std::memcpy(vertex_buffer_ptr, data.vertices, buffer_params.allocation.size);

      bfBuffer_flushRange(vertex_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
      bfBuffer_unMap(vertex_buffer);

      data.destroy();
    }
  }

  void draw(bfGfxCommandListHandle cmdlist)
  {
    uint64_t buffer_offset = 0;
    bfGfxCmdList_bindVertexBuffers(cmdlist, 0, &vertex_buffer, 1, &buffer_offset);
    bfGfxCmdList_draw(cmdlist, 0, (uint32_t)num_vertices);
  }

  void unload(bfGfxDeviceHandle device)
  {
    bfGfxDevice_release(device, vertex_buffer);
    vertex_buffer = nullptr;
  }
};

static Model s_Model;

class BaseRenderer
{
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
  bfTextureHandle              m_DepthBuffer;
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
    m_DepthBuffer{nullptr},
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

  bfGfxContextHandle context() const
  {
    return m_GfxBackend;
  }

  bfTextureHandle surface() const
  {
    return m_MainSurface;
  }

  bfGfxCommandListHandle mainCommandList() const
  {
    return m_MainCmdList;
  }

  void startup(const bfGfxContextCreateParams& gfx_create_params)
  {
    m_GfxBackend = bfGfxContext_new(&gfx_create_params);
    m_GfxDevice  = bfGfxContext_device(m_GfxBackend);

    m_BasicVertexLayout = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(m_BasicVertexLayout, 0, sizeof(BasicVertex));
    bfVertexLayout_addVertexLayout(m_BasicVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(BasicVertex, pos));
    bfVertexLayout_addVertexLayout(m_BasicVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(BasicVertex, normal));
    bfVertexLayout_addVertexLayout(m_BasicVertexLayout, 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(BasicVertex, color));
    bfVertexLayout_addVertexLayout(m_BasicVertexLayout, 0, BIFROST_VFA_FLOAT32_2, offsetof(BasicVertex, uv));

    bfBufferCreateParams buffer_params;
    buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE;
    buffer_params.allocation.size       = sizeof(BasicVertex) * 4;
    buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

    m_VertexBuffer = bfGfxDevice_newBuffer(m_GfxDevice, &buffer_params);

    buffer_params.allocation.size = sizeof(std::uint16_t) * 6;
    buffer_params.usage           = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_INDEX_BUFFER;

    m_IndexBuffer = bfGfxDevice_newBuffer(m_GfxDevice, &buffer_params);

    buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
    buffer_params.allocation.size       = 0x100 * 2;
    buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_UNIFORM_BUFFER;

    m_UniformBuffer = bfGfxDevice_newBuffer(m_GfxDevice, &buffer_params);

    const BasicVertex VERTEX_DATA[] =
     {
      {Vec3f{-0.5f, -0.5f, 0.0f, 1.0f}, Vec3f{}, {0, 255, 255, 255}, {0.0f, 0.0f}},
      {Vec3f{+0.5f, -0.5f, 0.0f, 1.0f}, Vec3f{}, {255, 255, 0, 255}, {1.0f, 0.0f}},
      {Vec3f{+0.5f, +0.5f, 0.0f, 1.0f}, Vec3f{}, {255, 0, 255, 255}, {1.0f, 1.0f}},
      {Vec3f{-0.5f, +0.5f, 0.0f, 1.0f}, Vec3f{}, {255, 255, 255, 255}, {0.0f, 1.0f}},
     };

    const uint16_t INDEX_DATA[] = {0u, 1u, 2u, 3u, 2u, 0u};

    void* vertex_buffer_ptr = bfBuffer_map(m_VertexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
    void* index_buffer_ptr  = bfBuffer_map(m_IndexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);

    std::memcpy(vertex_buffer_ptr, VERTEX_DATA, sizeof(VERTEX_DATA));
    std::memcpy(index_buffer_ptr, INDEX_DATA, sizeof(INDEX_DATA));

    bfBuffer_flushRange(m_VertexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
    bfBuffer_flushRange(m_IndexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
    bfBuffer_unMap(m_VertexBuffer);
    bfBuffer_unMap(m_IndexBuffer);

    bfTextureCreateParams      create_texture = bfTextureCreateParams_init2D(BIFROST_TEXTURE_UNKNOWN_SIZE, BIFROST_TEXTURE_UNKNOWN_SIZE, BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM);
    bfTextureSamplerProperties sampler        = bfTextureSamplerProperties_init(BIFROST_SFM_NEAREST, BIFROST_SAM_REPEAT);

    // create_texture.generate_mipmaps = bfFalse;

    m_TestTexture = bfGfxDevice_newTexture(m_GfxDevice, &create_texture);
    bfTexture_loadFile(m_TestTexture, "assets/texture.png");
    bfTexture_setSampler(m_TestTexture, &sampler);

    bfShaderProgramCreateParams create_shader;
    create_shader.debug_name    = "My Test Shader";
    create_shader.num_desc_sets = 1;

    m_ShaderModuleV = bfGfxDevice_newShaderModule(m_GfxDevice, BIFROST_SHADER_TYPE_VERTEX);
    m_ShaderModuleF = bfGfxDevice_newShaderModule(m_GfxDevice, BIFROST_SHADER_TYPE_FRAGMENT);
    m_ShaderProgram = bfGfxDevice_newShaderProgram(m_GfxDevice, &create_shader);

    bfShaderModule_loadFile(m_ShaderModuleV, "assets/basic_material.vert.spv");
    bfShaderModule_loadFile(m_ShaderModuleF, "assets/basic_material.frag.spv");

    bfShaderProgram_addModule(m_ShaderProgram, m_ShaderModuleV);
    bfShaderProgram_addModule(m_ShaderProgram, m_ShaderModuleF);

    bfShaderProgram_addImageSampler(m_ShaderProgram, "u_DiffuseTexture", 0, 0, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addUniformBuffer(m_ShaderProgram, "u_ModelTransform", 0, 1, 1, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(m_ShaderProgram);

    m_TestMaterial  = bfShaderProgram_createDescriptorSet(m_ShaderProgram, 0);
    m_TestMaterial2 = bfShaderProgram_createDescriptorSet(m_ShaderProgram, 0);

    uint64_t offset = 0;
    uint64_t sizes  = sizeof(m_ModelView);

    bfDescriptorSet_setCombinedSamplerTextures(m_TestMaterial, 0, 0, &m_TestTexture, 1);
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

    s_Model.load(m_GfxDevice, "assets/models/cultchar.obj");
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

        const std::uint32_t surface_w = bfTexture_width(m_MainSurface);
        const std::uint32_t surface_h = bfTexture_height(m_MainSurface);

        if (!m_DepthBuffer || bfTexture_width(m_DepthBuffer) != surface_w || bfTexture_height(m_DepthBuffer) != surface_h)
        {
          bfGfxDevice_flush(m_GfxDevice);
          bfGfxDevice_release(m_GfxDevice, m_DepthBuffer);

          bfTextureCreateParams create_depth_tex = bfTextureCreateParams_initDepthAttachment(surface_w, surface_h, BIFROST_IMAGE_FORMAT_D24_UNORM_S8_UINT, bfFalse, bfFalse);

          m_DepthBuffer = bfGfxDevice_newTexture(m_GfxDevice, &create_depth_tex);
          bfTexture_loadData(m_DepthBuffer, nullptr, 0);
        }

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

    bfAttachmentInfo depth_buffer;
    depth_buffer.texture      = m_DepthBuffer;
    depth_buffer.final_layout = BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_buffer.may_alias    = bfFalse;

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setClearOps(&renderpass_info, 1 | 2);
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStoreOps(&renderpass_info, 1);
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, 0x0);
    bfRenderpassInfo_addAttachment(&renderpass_info, &main_surface);
    bfRenderpassInfo_addAttachment(&renderpass_info, &depth_buffer);
    bfRenderpassInfo_addColorOut(&renderpass_info, 0, 0, BIFROST_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    bfRenderpassInfo_addDepthOut(&renderpass_info, 0, 1, BIFROST_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    // bfRenderpassInfo_addInput(&renderpass_info, 0x1);
    // bfRenderpassInfo_addDependencies(&renderpass_info, 0x1);

    BifrostClearValue clear_colors[2];
    clear_colors[0].color.float32[0] = 0.8f;
    clear_colors[0].color.float32[1] = 0.6f;
    clear_colors[0].color.float32[2] = 1.0f;
    clear_colors[0].color.float32[3] = 1.0f;

    clear_colors[1].depthStencil.depth   = 1.0f;
    clear_colors[1].depthStencil.stencil = 0;

    bfTextureHandle attachments[] = {m_MainSurface, m_DepthBuffer};

    bfGfxCmdList_setRenderpassInfo(m_MainCmdList, &renderpass_info);
    bfGfxCmdList_setClearValues(m_MainCmdList, clear_colors);
    bfGfxCmdList_setAttachments(m_MainCmdList, attachments);
    bfGfxCmdList_setRenderAreaRel(m_MainCmdList, 0.0f, 0.0f, 1.0f, 1.0f);

    bfGfxCmdList_beginRenderpass(m_MainCmdList);
    {
      uint64_t buffer_offset = 0;

      bfGfxCmdList_setDepthTesting(m_MainCmdList, bfTrue);
      bfGfxCmdList_setDepthWrite(m_MainCmdList, bfTrue);
      bfGfxCmdList_setDepthTestOp(m_MainCmdList, BIFROST_COMPARE_OP_LESS_OR_EQUAL);

      bfGfxCmdList_bindProgram(m_MainCmdList, m_ShaderProgram);
      bfGfxCmdList_bindVertexDesc(m_MainCmdList, m_BasicVertexLayout);

      bfGfxCmdList_bindDescriptorSets(m_MainCmdList, 0, &m_TestMaterial, 1);
      s_Model.draw(m_MainCmdList);

      bfGfxCmdList_bindVertexBuffers(m_MainCmdList, 0, &m_VertexBuffer, 1, &buffer_offset);
      bfGfxCmdList_bindIndexBuffer(m_MainCmdList, m_IndexBuffer, 0, BIFROST_INDEX_TYPE_UINT16);
      bfGfxCmdList_bindDescriptorSets(m_MainCmdList, 0, &m_TestMaterial2, 1);
      bfGfxCmdList_drawIndexed(m_MainCmdList, 6, 0, 0);
    }
  }

  void frameEnd()
  {
    bfGfxCmdList_endRenderpass(m_MainCmdList);
    bfGfxCmdList_end(m_MainCmdList);
    bfGfxCmdList_submit(m_MainCmdList);
    bfGfxContext_endFrame(m_GfxBackend);

    m_Time += 1.0f / 60.0f;
  }

  void cleanup()
  {
    bfGfxDevice_flush(m_GfxDevice);

    s_Model.unload(m_GfxDevice);

    bfGfxDevice_release(m_GfxDevice, m_DepthBuffer);

    for (auto resource : m_Resources)
    {
      bfGfxDevice_release(m_GfxDevice, resource);
    }

    bfVertexLayout_delete(m_BasicVertexLayout);
    bfGfxContext_delete(m_GfxBackend);
    m_GfxBackend = nullptr;
  }
};

namespace bifrost
{
}

class BifrostEngine : private bifrost::bfNonCopyMoveable<BifrostEngine>
{
 private:
  std::pair<int, const char**> m_CmdlineArgs;
  bifrost::FreeListAllocator   m_MainMemory;
  bifrost::GameStateMachine    m_StateMachine;
  BaseRenderer                 m_Renderer;

 public:
  BifrostEngine(int argc, const char* argv[]) :
    m_CmdlineArgs{argc, argv},
    m_MainMemory{new char[100000000ull], 100000000ull},
    m_StateMachine{*this, m_MainMemory},
    m_Renderer{}
  {
  }

  bifrost::GameStateMachine& stateMachine() { return m_StateMachine; }
  BaseRenderer&              renderer() { return m_Renderer; }

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

    bifrost::BaseObjectT::instanciate("Entity", m_MainMemory);
    bifrost::BaseObjectT::instanciate("Component", m_MainMemory);
    bifrost::BaseObjectT::instanciate("SpriteComponent", m_MainMemory);
    bifrost::BaseObjectT::instanciate("MeshComponent", m_MainMemory);

    bfLogPop();
  }

  [[nodiscard]] bool beginFrame()
  {
    m_StateMachine.purgeStates();

    return m_Renderer.frameBegin();
  }

  void onEvent(bifrost::Event& evt)
  {
    for (auto it = m_StateMachine.rbegin(); (evt.flags & bifrost::Event::FLAGS_IS_ACCEPTED) == 0 && it != m_StateMachine.rend(); ++it)
    {
      it->onEvent(*this, evt);
    }
  }

  void update()
  {
    m_Renderer.frameUpdate();

    for (auto& state : m_StateMachine)
    {
      state.onUpdate(*this, 0.0f);
    }
  }

  void endFrame()
  {
    m_Renderer.frameEnd();
  }

  void deinit()
  {
    m_StateMachine.removeAll();
    bfLogger_deinit();
    m_Renderer.cleanup();
  }

  ~BifrostEngine()
  {
    delete[] m_MainMemory.begin();
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

namespace bifrost
{
  class Camera
  {
   private:
    float m_ElapsedTime = 0.0f;

   public:
    void update(float dt)
    {
      std::printf("Camera::update(@ %f) with %f as dt.\n", m_ElapsedTime, dt);

      m_ElapsedTime += dt;
    }
  };
}  // namespace bifrost

#ifdef _WIN32
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement                = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif  //  _WIN32

static GLFWmonitor* get_current_monitor(GLFWwindow* window)
{
  int                nmonitors, i;
  int                wx, wy, ww, wh;
  int                mx, my, mw, mh;
  int                overlap, bestoverlap;
  GLFWmonitor*       bestmonitor;
  const GLFWvidmode* mode;

  bestoverlap = 0;
  bestmonitor = nullptr;

  glfwGetWindowPos(window, &wx, &wy);
  glfwGetWindowSize(window, &ww, &wh);
  GLFWmonitor** monitors = glfwGetMonitors(&nmonitors);

  for (i = 0; i < nmonitors; i++)
  {
    mode = glfwGetVideoMode(monitors[i]);
    glfwGetMonitorPos(monitors[i], &mx, &my);
    mw = mode->width;
    mh = mode->height;

    overlap = max(0, min(wx + ww, mx + mw) - max(wx, mx)) * max(0, min(wy + wh, my + mh) - max(wy, my));

    if (bestoverlap < overlap)
    {
      bestoverlap = overlap;
      bestmonitor = monitors[i];
    }
  }

  return bestmonitor;
}

int main(int argc, const char* argv[])  // NOLINT(bugprone-exception-escape)
{
  bfLogSetColor(BIFROST_LOGGER_COLOR_CYAN, BIFROST_LOGGER_COLOR_GREEN, BIFROST_LOGGER_COLOR_FG_BOLD);
  std::printf("\n\n                 Bifrost Engine v%s\n\n\n", BIFROST_VERSION_STR);
  bfLogSetColor(BIFROST_LOGGER_COLOR_BLUE, BIFROST_LOGGER_COLOR_WHITE, 0x0);

  using namespace bifrost;
  namespace bfmeta = meta;
  namespace bf     = bifrost;

  static constexpr int INITIAL_WINDOW_SIZE[] = {1280, 720};

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

  if (!startupGLFW(nullptr, nullptr))
  {
    return ErrorCodes::GLFW_FAILED_TO_INIT;
  }

  /*
  glfwMakeContextCurrent(main_window);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    error_code = ErrorCodes::GLAD_FAILED_TO_INIT;
    goto shutdown_glfw;  // NOLINT(hicpp-avoid-goto)
  }
  */

  {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    WindowGLFW    window{};
    BifrostEngine engine{argc, argv};

    if (!window.open("Bifrost Engine"))
    {
      return -1;
    }

    g_Window = window.handle();

    glfwSetWindowSizeLimits(g_Window, 300, 70, GLFW_DONT_CARE, GLFW_DONT_CARE);

    const BifrostEngineCreateParams params =
     {
      {
       argv[0],
       0,
       GetModuleHandle(nullptr),
       glfwGetWin32Window(window.handle()),
      },
      INITIAL_WINDOW_SIZE[0],
      INITIAL_WINDOW_SIZE[1],
     };

    engine.init(params);

    editor::imgui::startup(engine.renderer().context(), window);

    engine.stateMachine().push<MainDemoLayer>();

    std::cout << "\n\nScripting Language Test Begin\n";

    VMParams vm_params{};
    vm_params.error_fn = &userErrorFn;
    vm_params.print_fn = [](BifrostVM*, const char* message) {
      bfLogSetColor(BIFROST_LOGGER_COLOR_BLACK, BIFROST_LOGGER_COLOR_YELLOW, 0x0);
      bfLogPush("Print From Script");
      bfLogPrint("(BTS) %s", message);
      bfLogPop();
      bfLogSetColor(BIFROST_LOGGER_COLOR_CYAN, BIFROST_LOGGER_COLOR_GREEN, BIFROST_LOGGER_COLOR_FG_BOLD);
    };
    vm_params.min_heap_size      = 20;
    vm_params.heap_size          = 200;
    vm_params.heap_growth_factor = 0.1f;
    VM vm{vm_params};

    const BifrostVMClassBind camera_clz_bindings = bf::vmMakeClassBinding<bf::Camera>("Camera", bf::vmMakeCtorBinding<bf::Camera>());

    vm.stackResize(5);

    vm.moduleLoad(0, BIFROST_VM_STD_MODULE_ALL);

    vm.moduleMake(0, "bifrost");
    vm.stackStore(0, camera_clz_bindings);
    vm.stackLoadVariable(1, 0, "Camera");
    vm.stackStore(1, "update", &bf::Camera::update);

    const BifrostVMClassBind clz_bind = bf::vmMakeClassBinding<TestClass>(
     "TestClass",
     bf::vmMakeCtorBinding<TestClass, int>(),
     bf::vmMakeMemberBinding<&TestClass::printf>("printf"));

    vm.stackResize(1);
    vm.moduleMake(0, "main");
    vm.stackStore(0, clz_bind);
    vm.stackStore<testFN>(0, "cppFn");

    float capture = 6.0f;
    vm.stackStore(0, "BigFunc", [&capture, my_obj]() mutable -> void {
      std::printf("Will this work out %f???\n", capture);
      ++capture;
      my_obj.x = 65;
    });
    vm.stackStore(0, "AnotherOne", std::function<const char*()>([]() { return "Ohhh storing an std::function\n"; }));

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

    while (!window.wantsToClose())
    {
      int window_width, window_height;
      glfwGetWindowSize(window.handle(), &window_width, &window_height);

      glfwPollEvents();

      while (window.hasNextEvent())
      {
        Event evt = window.getNextEvent();

        if (evt.type == EventType::ON_KEY_DOWN && evt.keyboard.key == 'P')
        {
          static bool isFullscreen = false;
          static int  old_info[4];

          if (isFullscreen)
          {
            glfwSetWindowMonitor(g_Window, nullptr, old_info[0], old_info[1], old_info[2], old_info[3], 60);
          }
          else
          {
            glfwGetWindowPos(g_Window, &old_info[0], &old_info[1]);
            glfwGetWindowSize(g_Window, &old_info[2], &old_info[3]);

            GLFWmonitor*       monitor = get_current_monitor(g_Window);
            const GLFWvidmode* mode    = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(g_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
          }

          isFullscreen = !isFullscreen;
        }

        editor::imgui::onEvent(evt);
        engine.onEvent(evt);
      }

      if (engine.beginFrame())
      {
        editor::imgui::beginFrame(
         engine.renderer().surface(),
         float(window_width),
         float(window_height),
         float(glfwGetTime()));
        engine.update();

        if (update_fn)
        {
          vm.stackResize(1);
          vm.stackLoadHandle(0, update_fn);

          if (vm.stackGetType(0) == BIFROST_VM_FUNCTION)
          {
            // vm.call(0);
          }
        }

        editor::imgui::endFrame(engine.renderer().mainCommandList());
        engine.endFrame();
      }

      // std::this_thread::sleep_for(std::chrono::milliseconds(15 * 1));
    }

    vm.stackDestroyHandle(update_fn);
    editor::imgui::shutdown();
    engine.deinit();
    window.close();
  }

  shutdownGLFW();

  return 0;
}

void ImGUIOverlay::onUpdate(BifrostEngine& engine, float delta_time)
{
  if (m_Name != std::string("ImGUI 0"))
    return;

  ImGui::ShowDemoWindow();

  ImGuiIO& io = ImGui::GetIO();

  const auto height = io.DisplaySize.y - 10.0f;

  ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f));
  ImGui::SetNextWindowSize(ImVec2(350.0f, height), ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f, height), ImVec2(600.0f, height));
  if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoMove))
  {
    ImGui::Button("Create Entity");

    ImGui::Separator();

    for (int i = 0; i < 100; ++i)
      ImGui::Selectable("Item");
  }
  ImGui::End();

  window("Game State Machine", [&engine]() {
    auto& state_machine = engine.stateMachine();

    for (auto& state : state_machine)
    {
      ImGui::PushID(&state);
      if (ImGui::TreeNode(state.name()))
      {
        if (&state == state_machine.head())
        {
          ImGui::TextColored(ImVec4{1.0f, 0.0f, 0.0f, 1.0f}, "Layer Head");
        }

        if (&state == state_machine.tail())
        {
          ImGui::TextColored(ImVec4{0.0f, 1.0f, 0.0f, 1.0f}, "Layer Tail");
        }

        if (&state == state_machine.overlayHead())
        {
          ImGui::TextColored(ImVec4{0.0f, 1.0f, 0.0f, 1.0f}, "Overlay Head");
        }

        if (&state == state_machine.overlayTail())
        {
          ImGui::TextColored(ImVec4{1.0f, 0.0f, 1.0f, 1.0f}, "Overlay Tail");
        }

        ImGui::Text("Prev: %s", state.prev() ? state.prev()->name() : "<null>");
        ImGui::Text("Next: %s", state.next() ? state.next()->name() : "<null>");

        ImGui::Separator();

        if (ImGui::Button("Push Before"))
        {
          state_machine.pushBefore<ImGUIOverlay>(state, "Useless");
        }

        ImGui::SameLine();

        if (ImGui::Button("Push After"))
        {
          state_machine.pushAfter<ImGUIOverlay>(state, "Garbage");
        }

        ImGui::SameLine();

        if (ImGui::Button("Remove"))
        {
          state_machine.remove(&state);
        }

        ImGui::TreePop();
      }
      ImGui::PopID();
    }
  });
}

void MainDemoLayer::onLoad(BifrostEngine& engine)
{
  bfLogPrint("MainDemoLayer::onLoad Start");
  engine.stateMachine().addOverlay<ImGUIOverlay>("ImGUI 0");
  engine.stateMachine().addOverlay<ImGUIOverlay>("ImGUI 1");
  engine.stateMachine().addOverlay<ImGUIOverlay>("ImGUI 2");
  bfLogPrint("MainDemoLayer::onLoad End");
}

#include "src/bifrost/asset_io/test.cpp"
