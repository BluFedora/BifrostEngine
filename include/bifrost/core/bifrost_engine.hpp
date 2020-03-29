#ifndef BIFROST_ENGINE_HPP
#define BIFROST_ENGINE_HPP

#include "bifrost/asset_io/bifrost_assets.hpp"
#include "bifrost/asset_io/bifrost_scene.hpp"
#include "bifrost/debug/bifrost_dbg_logger.h"
#include "bifrost/ecs/bifrost_iecs_system.hpp"  // IECSSystem
#include "bifrost/event/bifrost_window_event.hpp"
#include "bifrost/graphics/bifrost_standard_renderer.hpp"
#include "bifrost/memory/bifrost_freelist_allocator.hpp"
#include "bifrost/memory/bifrost_linear_allocator.hpp"
#include "bifrost/script/bifrost_vm.hpp"
#include "bifrost_game_state_machine.hpp"
#include "bifrost_igame_state_layer.hpp"

#include <tuple>
#include <utility>

#include "glfw/glfw3.h"

using namespace bifrost;

struct BifrostEngineCreateParams : public bfGfxContextCreateParams
{
  std::uint32_t width;
  std::uint32_t height;
};

static GLFWwindow* g_Window = nullptr;

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

// const auto limits = bfGfxDevice_limits(m_GfxDevice);
// m_AlignedUBOSize  = alignedUpSize(sizeof(m_ModelView), (size_t)limits.uniform_buffer_offset_alignment);

class BifrostEngine : private bfNonCopyMoveable<BifrostEngine>
{
  // TEMP CODE START
 public:
  struct Camera_t Camera;

  void cameraControls()
  {
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
        std::get<1>(control)(&Camera, std::get<2>(control));
      }
    }

    int width, height;
    glfwGetWindowSize(g_Window, &width, &height);

    Camera_onResize(&Camera, width, height);
    Camera_update(&Camera);
  }

  // TEMP CODE END

 private:
  std::pair<int, const char**> m_CmdlineArgs;
  FreeListAllocator            m_MainMemory;
  LinearAllocator              m_TempMemory;
  NoFreeAllocator              m_TempAdapter;
  GameStateMachine             m_StateMachine;
  VM                           m_Scripting;
  StandardRenderer             m_Renderer;
  Array<AssetSceneHandle>      m_SceneStack;
  Assets                       m_Assets;
  Array<IECSSystem*>           m_Systems;

 public:
  BifrostEngine(char* main_memory, std::size_t main_memory_size, int argc, const char* argv[]) :
    Camera{},
    m_CmdlineArgs{argc, argv},
    m_MainMemory{main_memory, main_memory_size},
    m_TempMemory{static_cast<char*>(m_MainMemory.allocate(main_memory_size / 4)), main_memory_size / 4},
    m_TempAdapter{m_TempMemory},
    m_StateMachine{*this, m_MainMemory},
    m_Scripting{},
    m_Renderer{m_MainMemory},
    m_SceneStack{m_MainMemory},
    m_Assets{*this, m_MainMemory},
    m_Systems{m_MainMemory}
  {
    // TEMP CODE BGN
    Vec3f cam_pos = {0.0f, 0.0f, 4.0f, 1.0f};
    Vec3f cam_up  = {0.0f, 1.0f, 0.0f, 0.0f};
    Camera_init(&Camera, &cam_pos, &cam_up, 0.0f, 0.0f);
    // TEMP CODE END
  }

  FreeListAllocator& mainMemory() { return m_MainMemory; }
  LinearAllocator&   tempMemory() { return m_TempMemory; }
  IMemoryManager&    tempMemoryNoFree() { return m_TempAdapter; }
  GameStateMachine&  stateMachine() { return m_StateMachine; }
  VM&                scripting() { return m_Scripting; }
  StandardRenderer&  renderer() { return m_Renderer; }
  Assets&            assets() { return m_Assets; }

  AssetSceneHandle currentScene() const
  {
    if (m_SceneStack.size())
    {
      return m_SceneStack.back();
    }

    return {};
  }

  void openScene(const AssetSceneHandle& scene)
  {
    m_SceneStack.clear();  // TODO: Scene Stacking.
    m_SceneStack.push(scene);
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

    m_Renderer.init(params);

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

    cameraControls();
    return m_Renderer.frameBegin(Camera);
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
    for (auto& system : m_Systems)
    {
      if (!system->isEnabled())
      {
        continue;
      }

      system->onFrameBegin(*this, delta_time);
    }

    for (auto& state : m_StateMachine)
    {
      state.onUpdate(*this, delta_time);
    }

    for (auto& system : m_Systems)
    {
      if (!system->isEnabled())
      {
        continue;
      }

      system->onFrameUpdate(*this, delta_time);
    }

    for (auto& system : m_Systems)
    {
      if (!system->isEnabled())
      {
        continue;
      }

      system->onFrameEnd(*this, delta_time);
    }
  }

  void drawBegin(float render_alpha)
  {
    const auto cmd_list = m_Renderer.mainCommandList();

    m_Renderer.beginGBufferPass();

    auto scene = currentScene();

    if (scene)
    {
      for (MeshRenderer& renderer : scene->components<MeshRenderer>())
      {
        if (renderer.material() && renderer.model())
        {
          m_Renderer.bindMaterial(cmd_list, *renderer.material());
          m_Renderer.bindObject(cmd_list, renderer.owner());
          renderer.model()->draw(m_Renderer.mainCommandList());
        }
      }

      for (Light& light : scene->components<Light>())
      {
        m_Renderer.addLight(light);
      }
    }

    for (auto& system : m_Systems)
    {
      if (!system->isEnabled())
      {
        continue;
      }

      system->onFrameDraw(*this, render_alpha);
    }

    m_Renderer.endPass();

    // SSAO
    m_Renderer.beginSSAOPass(Camera);
    m_Renderer.endPass();

    // Lighting
    m_Renderer.beginLightingPass(Camera);
    m_Renderer.endPass();

    m_Renderer.beginScreenPass();
  }

  void drawEnd() const
  {
    m_Renderer.endPass();

    m_Renderer.frameEnd();
  }

  void deinit()
  {
    m_StateMachine.removeAll();
    m_Scripting.destroy();
    m_SceneStack.clear();
    m_Assets.saveAssets();
    m_Assets.setRootPath(nullptr);
    m_Renderer.deinit();

    for (auto& system : m_Systems)
    {
      m_MainMemory.deallocateT(system);
    }

    m_Systems.clear();
    bfLogger_deinit();
  }
};

#endif /* BIFROST_ENGINE_HPP */
