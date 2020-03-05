#ifndef BIFROST_ENGINE_HPP
#define BIFROST_ENGINE_HPP

#include "bifrost/asset_io/test.h"
#include "bifrost/bifrost.hpp"
#include "bifrost/ecs/bifrost_iecs_system.hpp"  // IECSSystem
#include "bifrost/event/bifrost_window_event.hpp"
#include "bifrost/memory/bifrost_freelist_allocator.hpp"

#include "glfw/glfw3.h"

using namespace bifrost;

struct BifrostEngineCreateParams : public bfGfxContextCreateParams
{
  std::uint32_t width;
  std::uint32_t height;
};

class Model final
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

static BifrostEngine* g_Engine = nullptr;
static GLFWwindow*    g_Window = nullptr;
static Model          s_Model;

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
  explicit BaseRenderer() :
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

  bfGfxContextHandle     context() const { return m_GfxBackend; }
  bfTextureHandle        surface() const { return m_MainSurface; }
  bfGfxCommandListHandle mainCommandList() const { return m_MainCmdList; }

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

    s_Model.load(m_GfxDevice, "assets/models/rhino.obj");
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

    const std::tuple<int, void (*)(::Camera*, float), float> CameraControls[] =
     {
      {GLFW_KEY_W, &Camera_moveForward, camera_move_speed},
      {GLFW_KEY_A, &Camera_moveLeft, camera_move_speed},
      {GLFW_KEY_S, &Camera_moveBackward, camera_move_speed},
      {GLFW_KEY_D, &Camera_moveRight, camera_move_speed},
      {GLFW_KEY_Q, &Camera_moveUp, camera_move_speed},
      {GLFW_KEY_E, &Camera_moveDown, camera_move_speed},
      {GLFW_KEY_R, &Camera_addPitch, -0.01f},
      {GLFW_KEY_F, &Camera_addPitch, 0.01f},
      {GLFW_KEY_H, &Camera_addYaw, 0.01f},
      {GLFW_KEY_G, &Camera_addYaw, -0.01f},
     };

    for (const auto& control : CameraControls)
    {
      if (glfwGetKey(g_Window, std::get<0>(control)) == GLFW_PRESS)
      {
        std::get<1>(control)(&m_Camera, std::get<2>(control));
      }
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

      // bfGfxCmdList_bindDescriptorSets(m_MainCmdList, 0, &m_TestMaterial, 1);
      // s_Model.draw(m_MainCmdList);

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

class BifrostEngine : private bfNonCopyMoveable<BifrostEngine>
{
 private:
  std::pair<int, const char**> m_CmdlineArgs;
  FreeListAllocator            m_MainMemory;
  GameStateMachine             m_StateMachine;
  VM                           m_Scripting;
  BaseRenderer                 m_Renderer;
  Scene*                       m_CurrentScene;
  Assets                       m_Assets;
  Array<IECSSystem*>           m_Systems;

 public:
  BifrostEngine(char* main_memory, std::size_t main_memmory_size, int argc, const char* argv[]) :
    m_CmdlineArgs{argc, argv},
    m_MainMemory{main_memory, main_memmory_size},
    m_StateMachine{*this, m_MainMemory},
    m_Scripting{},
    m_Renderer{},
    m_CurrentScene{nullptr},
    m_Assets{*this, m_MainMemory},
    m_Systems{m_MainMemory}
  {
  }

  FreeListAllocator& mainMemory() { return m_MainMemory; }
  GameStateMachine&  stateMachine() { return m_StateMachine; }
  VM&                scripting() { return m_Scripting; }
  BaseRenderer&      renderer() { return m_Renderer; }
  Assets&            assets() { return m_Assets; }

  Scene* currentScene() const
  {
    return m_CurrentScene;
  }

  void loadScene(Scene* scene)
  {
    m_CurrentScene = scene;
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

    m_Scripting.create(vm_params);

    bfLogPop();
  }

  [[nodiscard]] bool beginFrame()
  {
    m_StateMachine.purgeStates();

    return m_Renderer.frameBegin();
  }

  void onEvent(Event& evt)
  {
    for (auto it = m_StateMachine.rbegin(); !evt.isAccepted() && it != m_StateMachine.rend(); ++it)
    {
      it->onEvent(*this, evt);
    }
  }

  void fixedUpdate(float delta_time)
  {
    for (auto& state : m_StateMachine)
    {
      state.onFixedUpdate(*this, delta_time);
    }
  }

  void update(float delta_time)
  {
    for (auto& state : m_StateMachine)
    {
      state.onUpdate(*this, delta_time);
    }
  }

  void drawBegin()
  {
    m_Renderer.frameUpdate();
  }

  void drawEnd()
  {
    m_Renderer.frameEnd();
  }

  void deinit()
  {
    m_StateMachine.removeAll();
    m_Scripting.destroy();
    bfLogger_deinit();
    m_Renderer.cleanup();
    m_Systems.clear();
  }
};

#endif /* BIFROST_ENGINE_HPP */
