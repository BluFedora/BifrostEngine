#include "bifrost/core/bifrost_engine.hpp"

#include "bifrost/debug/bifrost_dbg_logger.h"  // bfLog*
#include "bifrost/ecs/bifrost_behavior.hpp"
#include "bifrost/ecs/bifrost_behavior_system.hpp"    // BehaviorSystem
#include "bifrost/ecs/bifrost_entity.hpp"             // Entity
#include "bifrost/platform/bifrost_platform.h"        // BifrostWindow
#include "bifrost/platform/bifrost_platform_event.h"  // bfEvent

Engine::Engine(char* main_memory, std::size_t main_memory_size, int argc, char* argv[]) :
  bfNonCopyMoveable<Engine>{},
  m_CmdlineArgs{argc, argv},
#if USE_CRT_HEAP
  m_MainMemory{},
#else
  m_MainMemory{main_memory, main_memory_size},
#endif
  m_TempMemory{static_cast<char*>(m_MainMemory.allocate(main_memory_size / 4)), main_memory_size / 4},
  m_TempAdapter{m_TempMemory},
  m_StateMachine{*this, m_MainMemory},
  m_Scripting{},
  m_Renderer{m_MainMemory},
  m_DebugRenderer{m_MainMemory},
  m_SceneStack{m_MainMemory},
  m_Assets{*this, m_MainMemory},
  m_Systems{m_MainMemory},
  m_CameraMemory{},
  m_CameraList{nullptr},
  m_CameraResizeList{nullptr},
  m_CameraDeleteList{nullptr},
  m_State{EngineState::RUNTIME_PLAYING}
{
#if USE_CRT_HEAP
  (void)main_memory;
#endif
}

AssetSceneHandle Engine::currentScene() const
{
  return m_SceneStack.isEmpty() ? AssetSceneHandle{} : m_SceneStack.back();
}

CameraRender* Engine::borrowCamera(const CameraRenderCreateParams& params)
{
  return m_CameraMemory.allocateT<CameraRender>(m_CameraList, m_Renderer.device(), m_Renderer.m_FrameInfo, params);
}

void Engine::resizeCamera(CameraRender* camera, int width, int height)
{
  camera->new_width  = width;
  camera->new_height = height;

  // If not already on the list.
  if (!camera->resize_list_next)
  {
    camera->resize_list_next = m_CameraResizeList;
    m_CameraResizeList       = camera;
  }
}

void Engine::returnCamera(CameraRender* camera)
{
  // Remove Camera from resize list.
  if (camera->resize_list_next)
  {
    CameraRender* prev_cam = m_CameraResizeList;

    while (prev_cam)
    {
      CameraRender* const next = prev_cam->resize_list_next;

      if (next == camera)
      {
        break;
      }

      prev_cam = next;
    }

    if (prev_cam)
    {
      prev_cam->resize_list_next = camera->resize_list_next;
    }
    else
    {
      m_CameraResizeList = camera->resize_list_next;
    }
  }

  // Remove From Active List
  if (camera->next)
  {
    camera->next->prev = camera->prev;
  }

  if (camera->prev)
  {
    camera->prev->next = camera->next;
  }
  else
  {
    m_CameraList = camera->next;
  }

  // Add to Delete List
  camera->next       = m_CameraDeleteList;
  m_CameraDeleteList = camera;
}

void Engine::openScene(const AssetSceneHandle& scene)
{
  m_SceneStack.clear();  // TODO: Scene Stacking.
  m_SceneStack.push(scene);
}

EntityRef Engine::createEntity(Scene& scene, const StringRange& name)
{
  Entity* const entity = m_MainMemory.allocateT<Entity>(scene, name);

  if (entity)
  {
    scene.m_RootEntities.push(entity);
  }

  return EntityRef{entity};
}

void Engine::init(const BifrostEngineCreateParams& params, BifrostWindow* main_window)
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

  gc::init(m_MainMemory);

  m_Renderer.init(params, main_window);
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
  vm_params.heap_size          = 150;
  vm_params.heap_growth_factor = 0.1f;
  vm_params.user_data          = this;

  m_Scripting.create(vm_params);

  m_Scripting.stackResize(1);
  m_Scripting.moduleMake(0, "bifrost");

  const BifrostVMClassBind behavior_clz_bindings = vmMakeClassBinding<BaseBehavior>(
   "Behavior"//,
   //vmMakeMemberBinding<&BaseBehavior::owner>("owner"),
   //vmMakeMemberBinding<&BaseBehavior::scene>("scene"),
   //vmMakeMemberBinding<&BaseBehavior::engine>("engine")
  );

  m_Scripting.stackStore(0, behavior_clz_bindings);

  addECSSystem<BehaviorSystem>();
  addECSSystem<CollisionSystem>();

  m_StateMachine.push<detail::CoreEngineGameStateLayer>();

  bfLogPop();
}

bool Engine::beginFrame()
{
  tempMemory().clear();
  deleteCameras();
  resizeCameras();
  m_StateMachine.purgeStates();

  return m_Renderer.frameBegin();
}

void Engine::onEvent(Event& evt)
{
  for (auto it = m_StateMachine.rbegin(); !evt.isAccepted() && it != m_StateMachine.rend(); ++it)
  {
    it->onEvent(*this, evt);
  }
}

void Engine::fixedUpdate(float delta_time)
{
  for (auto& state : m_StateMachine)
  {
    state.onFixedUpdate(*this, delta_time);
  }
}

void Engine::drawEnd() const
{
  m_Renderer.endPass(m_Renderer.mainCommandList());
  m_Renderer.drawEnd();
}

void Engine::endFrame()
{
  m_Renderer.frameEnd();

  gc::collect(m_MainMemory);
}

void Engine::deinit()
{
  bfGfxDevice_flush(m_Renderer.device());

  // Released any resources from the game states.
  m_StateMachine.removeAll();

  // Must be cleared before the Asset System destruction since it contains handles to scenes.
  m_SceneStack.clear();

  m_Assets.clearDirtyList();
  m_Assets.setRootPath(nullptr);

  m_DebugRenderer.deinit();

  assert(m_CameraList == nullptr && "All cameras not returned to the engine before shutting down.");
  deleteCameras();

  m_Renderer.deinit();

  for (auto& system : m_Systems)
  {
    // system->onDeinit();
    m_MainMemory.deallocateT(system);
  }

  m_Systems.clear();

  gc::quit();

  // This happens after most systems because they could be holding bfValueHandles that needs to be released.
  m_Scripting.destroy();

  // Shutdown last since there could be errors logged on shutdown if any failures happen.
  bfLogger_deinit();
}

void Engine::update(float delta_time)
{
  m_DebugRenderer.update(delta_time);
  m_Renderer.m_GlobalTime += delta_time;  // TODO: Not very encapsolated.

  for (auto& system : m_Systems)
  {
    if (system->isEnabled())
    {
      system->onFrameBegin(*this, delta_time);
    }
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

void Engine::drawBegin(float render_alpha)
{
  const auto cmd_list = m_Renderer.mainCommandList();
  auto       scene    = currentScene();

  if (scene)
  {
    for (Light& light : scene->components<Light>())
    {
      m_Renderer.addLight(light);
    }
  }

  forEachCamera([this, &cmd_list, render_alpha, &scene](CameraRender* camera) {
    Camera_update(&camera->cpu_camera);

    m_Renderer.renderCameraTo(
     camera->cpu_camera,
     camera->gpu_camera,
     [this, &cmd_list, render_alpha, camera, &scene]() {
       if (scene)
       {
         for (MeshRenderer& renderer : scene->components<MeshRenderer>())
         {
           if (renderer.material() && renderer.model())
           {
             m_Renderer.bindMaterial(cmd_list, *renderer.material());
             m_Renderer.bindObject(cmd_list, camera->gpu_camera, renderer.owner());
             renderer.model()->draw(cmd_list);
           }
         }
       }

       for (auto& system : m_Systems)
       {
         if (system->isEnabled())
         {
           system->onFrameDraw(*this, render_alpha);
         }
       }

       m_DebugRenderer.draw(cmd_list, *camera, m_Renderer.m_FrameInfo, false);
     },
     [this, &cmd_list, camera]() {
       m_DebugRenderer.draw(cmd_list, *camera, m_Renderer.m_FrameInfo, true);
     });
  });

  m_Renderer.beginScreenPass(m_Renderer.mainCommandList());
}

void Engine::resizeCameras()
{
  CameraRender* camera = m_CameraResizeList;

  while (camera)
  {
    CameraRender* const next = camera->resize_list_next;
    camera->resize_list_next = nullptr;

    camera->resize();

    camera = next;
  }

  m_CameraResizeList = nullptr;
}

void Engine::deleteCameras()
{
  if (m_CameraDeleteList)
  {
    bfGfxDevice_flush(m_Renderer.device());

    CameraRender* camera = m_CameraDeleteList;

    while (camera)
    {
      CameraRender* const next = camera->next;

      m_CameraMemory.deallocateT(camera);

      camera = next;
    }

    m_CameraDeleteList = nullptr;
  }
}

namespace bifrost
{
  void detail::CoreEngineGameStateLayer::onEvent(Engine& engine, Event& event)
  {
    // This is the bottom most layer so just accept the event.
    event.accept();
  }
}  // namespace bifrost