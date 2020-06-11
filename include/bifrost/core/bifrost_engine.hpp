#ifndef BIFROST_ENGINE_HPP
#define BIFROST_ENGINE_HPP

#include "bifrost/asset_io/bifrost_assets.hpp"
#include "bifrost/asset_io/bifrost_scene.hpp"
#include "bifrost/debug/bifrost_dbg_logger.h"
#include "bifrost/ecs/bifrost_iecs_system.hpp"
#include "bifrost/event/bifrost_window_event.hpp"
#include "bifrost/graphics/bifrost_debug_renderer.hpp"
#include "bifrost/graphics/bifrost_standard_renderer.hpp"
#include "bifrost/memory/bifrost_freelist_allocator.hpp"
#include "bifrost/memory/bifrost_linear_allocator.hpp"
#include "bifrost/script/bifrost_vm.hpp"
#include "bifrost_game_state_machine.hpp"
#include "bifrost_igame_state_layer.hpp"

#include <utility>

#include "glfw/glfw3.h"

using namespace bifrost;

struct BifrostEngineCreateParams : public bfGfxContextCreateParams
{
  std::uint32_t width;
  std::uint32_t height;
};

extern GLFWwindow* g_Window;

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

namespace bifrost::detail
{
  class CoreEngineGameStateLayer : public IGameStateLayer
  {
   protected:
    void onEvent(BifrostEngine& engine, Event& event) override;

   public:
    const char* name() override { return "__CoreEngineLayer__"; }
  };
}  // namespace bifrost

class BifrostEngine : private bfNonCopyMoveable<BifrostEngine>
{
  // TEMP CODE START
 public:
  BifrostCamera Camera;
  // TEMP CODE END

 private:
  std::pair<int, const char**> m_CmdlineArgs;
  FreeListAllocator            m_MainMemory;
  LinearAllocator              m_TempMemory;
  NoFreeAllocator              m_TempAdapter;
  GameStateMachine             m_StateMachine;
  VM                           m_Scripting;
  StandardRenderer             m_Renderer;
  DebugRenderer                m_DebugRenderer;
  Array<AssetSceneHandle>      m_SceneStack;
  Assets                       m_Assets;
  Array<IECSSystem*>           m_Systems;
  IBaseWindow&                 m_Window;

 public:
  explicit BifrostEngine(IBaseWindow& window, char* main_memory, std::size_t main_memory_size, int argc, const char* argv[]);

  FreeListAllocator& mainMemory() { return m_MainMemory; }
  LinearAllocator&   tempMemory() { return m_TempMemory; }
  IMemoryManager&    tempMemoryNoFree() { return m_TempAdapter; }
  GameStateMachine&  stateMachine() { return m_StateMachine; }
  VM&                scripting() { return m_Scripting; }
  StandardRenderer&  renderer() { return m_Renderer; }
  DebugRenderer&     debugDraw() { return m_DebugRenderer; }
  Assets&            assets() { return m_Assets; }
  IBaseWindow&       window() const { return m_Window; }

  AssetSceneHandle currentScene() const;

  void openScene(const AssetSceneHandle& scene)
  {
    m_SceneStack.clear();  // TODO: Scene Stacking.
    m_SceneStack.push(scene);
  }

  template<typename T>
  void addECSSystem()
  {
    m_Systems.push(m_MainMemory.allocateT<T>());
  }

  void init(const BifrostEngineCreateParams& params)
  {
    IBifrostDbgLogger logger_config{
     nullptr,
     [](void* data, BifrostDbgLogInfo* info, va_list args) {
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
         std::vprintf(info->format, args);
         std::printf("\n");
       }
     }};

    bfLogger_init(&logger_config);

    bfLogPush("Engine Init of App: %s", params.app_name);

    m_Renderer.init(params);
    m_DebugRenderer.init(m_Renderer);

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
    vm_params.user_data          = this;

    m_Scripting.create(vm_params);

    m_StateMachine.push<detail::CoreEngineGameStateLayer>();

    bfLogPop();
  }

  [[nodiscard]] bool beginFrame()
  {
    m_StateMachine.purgeStates();

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
    m_DebugRenderer.update(delta_time);
    m_Renderer.m_GlobalTime += delta_time;  // TODO: Not very encapsolated.

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

    auto scene = currentScene();

    if (scene)
    {
      scene->update(tempMemory(), debugDraw());
    }

    for (auto& system : m_Systems)
    {
      if (system->isEnabled())
      {
        system->onFrameUpdate(*this, delta_time);
      }
    }

    for (auto& system : m_Systems)
    {
      if (system->isEnabled())
      {
        system->onFrameEnd(*this, delta_time);
      }
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
          renderer.model()->draw(cmd_list);
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

    m_DebugRenderer.draw(cmd_list, Camera, m_Renderer.m_FrameInfo, false);
    m_Renderer.endPass();

    // SSAO
    m_Renderer.beginSSAOPass(Camera);
    m_Renderer.endPass();

    // Lighting
    m_Renderer.beginLightingPass(Camera);
    m_DebugRenderer.draw(cmd_list, Camera, m_Renderer.m_FrameInfo, true);
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
    bfGfxDevice_flush(m_Renderer.device());

    m_StateMachine.removeAll();
    m_Scripting.destroy();
    m_SceneStack.clear();
    m_Assets.saveAssets();
    m_Assets.setRootPath(nullptr);
    m_DebugRenderer.deinit();
    m_Renderer.deinit();

    for (auto& system : m_Systems)
    {
      // system->onDeinit();
      m_MainMemory.deallocateT(system);
    }

    m_Systems.clear();
    bfLogger_deinit();
  }
};

#endif /* BIFROST_ENGINE_HPP */
