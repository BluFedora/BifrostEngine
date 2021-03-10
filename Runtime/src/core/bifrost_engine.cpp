#include "bf/core/bifrost_engine.hpp"

#include "bf/JobSystem.hpp"                            // Job System
#include "bf/Platform.h"                               // BifrostWindow
#include "bf/anim2D/bf_animation_system.hpp"           // AnimationSystem
#include "bf/asset_io/bf_gfx_assets.hpp"               // ***Asset
#include "bf/bf_ui.hpp"                                // UI::
#include "bf/bf_version.h"                             // BF_VERSION_STR
#include "bf/ecs/bifrost_behavior.hpp"                 // BaseBehavior
#include "bf/ecs/bifrost_behavior_system.hpp"          // BehaviorSystem
#include "bf/ecs/bifrost_entity.hpp"                   // Entity
#include "bf/graphics/bifrost_component_renderer.hpp"  // ComponentRenderer

using namespace std::chrono_literals;

namespace bf
{
  static constexpr float k_SecToMs = 1000.0f;

  namespace detail
  {
    class CoreEngineGameStateLayer final : public IGameStateLayer
    {
     protected:
      void onEvent(Engine& engine, Event& event) override;

     public:
      const char* name() override { return "__CoreEngineLayer__"; }
    };
  }  // namespace detail

  void Input::onEvent(Event& evt)
  {
    if (evt.isMouseEvent())
    {
      const auto& mouse_evt = evt.mouse;

      m_MouseState.button_state = mouse_evt.button_state;
    }

    switch (evt.type)  // NOLINT(clang-diagnostic-switch-enum)
    {
      case BIFROST_EVT_ON_MOUSE_MOVE:
      {
        const auto& mouse_evt = evt.mouse;

        const Vector2i old_mouse_pos = m_MouseState.current_pos;

        m_MouseState.current_pos = {mouse_evt.x, mouse_evt.y};
        m_MouseState.delta_pos   = m_MouseState.current_pos - old_mouse_pos;

        break;
      }
      case BIFROST_EVT_ON_KEY_DOWN:
      case BIFROST_EVT_ON_KEY_UP:
      {
        KEY_STATE[evt.keyboard.key] = evt.type == BIFROST_EVT_ON_KEY_DOWN;
        break;
      }
      default:
        break;
    }
  }

  void Input::frameEnd()
  {
    m_MouseState.delta_pos = {0, 0};
  }

  Engine::Engine(char* main_memory, std::size_t main_memory_size, int argc, char* argv[]) :
    NonCopyMoveable<Engine>{},
    m_CmdlineArgs{argc, argv},
    m_ConsoleLogger{},
#if USE_CRT_HEAP
    m_MainMemory{},
#else
    m_MainMemory{main_memory, main_memory_size},
#endif
    m_TempMemory{static_cast<char*>(m_MainMemory.allocate(main_memory_size / 4)), main_memory_size / 4},
    m_Assets{*this, m_MainMemory},
    m_StateMachine{*this, m_MainMemory},
    m_Scripting{},
    m_SceneStack{m_MainMemory},
    m_Input{},
    m_Renderer{m_MainMemory},
    m_DebugRenderer{m_MainMemory},
    m_Gfx2D{nullptr},
    m_2DScreenCommands{nullptr},
    m_2DScreenRenderQueue{RenderQueueType::SCREEN_OVERLAY},
    m_2DScreenUBO{},
    m_CameraMemory{},
    m_CameraList{nullptr},
    m_CameraResizeList{nullptr},
    m_CameraDeleteList{nullptr},
    m_Systems{m_MainMemory},
    m_AnimationSystem{nullptr},
    m_ComponentRenderer{nullptr},
    m_BehaviorSystem{nullptr},
    m_TimeStep{},
    m_TimeStepLag{0ns},
    m_CurrentTime{},
    m_State{EngineState::RUNTIME_PLAYING},
    m_IsInMiddleOfFrame{false}
  {
#if USE_CRT_HEAP
    (void)main_memory;
#endif

    m_ConsoleLogger.user_data = this;
    m_ConsoleLogger.callback  = [](bfIDbgLogger* data, bfDbgLogInfo* info, va_list args) {
      (void)data;

      if (info->level != BF_LOGGER_LVL_POP)
      {
        static constexpr unsigned int TAB_SIZE = 4;

#if 0
         std::printf("%*c%s(%u): \n", TAB_SIZE * info->indent_level, ' ', info->func, info->line);
         std::printf("%*c", TAB_SIZE * info->indent_level * 2, ' ');
#else
        std::printf("%*c", TAB_SIZE * info->indent_level, ' ');

#endif
        std::vprintf(info->format, args);  // NOLINT(clang-diagnostic-format-nonliteral)
        std::printf("\n");
      }
    };
  }

  ARC<SceneAsset> Engine::currentScene() const
  {
    return m_SceneStack.isEmpty() ? ARC<SceneAsset>{} : m_SceneStack.back();
  }

  RenderView* Engine::borrowCamera(const CameraRenderCreateParams& params)
  {
    return m_CameraMemory.allocateT<RenderView>(m_CameraList, m_Renderer.device(), m_Renderer.m_FrameInfo, params);
  }

  void Engine::resizeCamera(RenderView* camera, int width, int height)
  {
    if (camera->old_width != width || camera->old_height != height)
    {
      camera->new_width  = width;
      camera->new_height = height;

      // If not already on the resize list.
      if (!camera->resize_list_next)
      {
        camera->resize_list_next = m_CameraResizeList;
        m_CameraResizeList       = camera;
      }
    }
  }

  void Engine::returnCamera(RenderView* camera)
  {
    // Remove Camera from resize list.
    if (camera->resize_list_next)
    {
      RenderView* prev_cam = m_CameraResizeList;

      while (prev_cam)
      {
        RenderView* const next = prev_cam->resize_list_next;

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

  void Engine::openScene(const ARC<SceneAsset>& scene)
  {
    m_SceneStack.clear();  // TODO: Scene Stacking.

    if (scene)
    {
      m_SceneStack.push(scene);
    }
  }

  EntityRef Engine::createEntity(Scene& scene, const StringRange& name)
  {
    Entity* const entity = m_MainMemory.allocateT<Entity>(scene, name);

    if (entity)
    {
      scene.m_RootEntities.pushBack(*entity);
    }

    return EntityRef{entity};
  }

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

  void Engine::init(const EngineCreateParams& params, bfWindow* main_window)
  {
    bfLogAdd(&m_ConsoleLogger);
    bfJob::initialize();
    ClassID::Init();

    bfLogPush("Engine(v%s) Init of App: '%s'", BF_VERSION_STR, params.app_name);

    m_Assets.registerFileExtensions({".png", ".jpg", ".jpeg", ".ppm", ".pgm", ".bmp", ".tga", ".psd"}, &assetImportTexture);
    m_Assets.registerFileExtensions({".material"}, &assetImportMaterial);
    m_Assets.registerFileExtensions({".obj", ".fbx", ".md5mesh"}, &assetImportModel);
    m_Assets.registerFileExtensions({".scene"}, &assetImportScene);
    m_Assets.registerFileExtensions({".srsm.bytes"}, &assetImportSpritesheet);

    gc::init(m_MainMemory);

    m_Renderer.init(params, main_window);
    m_DebugRenderer.init(m_Renderer);
    m_Gfx2D = m_MainMemory.allocateT<CommandBuffer2D>(m_Renderer.glslCompiler(), m_Renderer.context());

    VMParams vm_params{};
    vm_params.error_fn = &userErrorFn;
    vm_params.print_fn = [](BifrostVM*, const char* message) {
      bfLogSetColor(BF_LOGGER_COLOR_BLACK, BF_LOGGER_COLOR_YELLOW, 0x0);
      bfLogPush("Print From Script");
      bfLogPrint("(BTS) %s", message);
      bfLogPop();
      bfLogSetColor(BF_LOGGER_COLOR_CYAN, BF_LOGGER_COLOR_GREEN, BF_LOGGER_COLOR_FG_BOLD);
    };
    vm_params.min_heap_size      = 20;
    vm_params.heap_size          = 150;
    vm_params.heap_growth_factor = 0.1f;
    vm_params.user_data          = this;

    m_Scripting.create(vm_params);

    m_Scripting.stackResize(1);
    m_Scripting.moduleMake(0, "bf");

    const BifrostVMClassBind behavior_clz_bindings = vmMakeClassBinding<BaseBehavior>(
     "Behavior"  //,
                 //vmMakeMemberBinding<&BaseBehavior::owner>("owner"),
                 //vmMakeMemberBinding<&BaseBehavior::scene>("scene"),
                 //vmMakeMemberBinding<&BaseBehavior::engine>("engine")
    );

    m_Scripting.stackStore(0, behavior_clz_bindings);

    m_BehaviorSystem    = addECSSystem<BehaviorSystem>();
    m_AnimationSystem   = addECSSystem<AnimationSystem>(m_MainMemory);
    m_ComponentRenderer = addECSSystem<ComponentRenderer>();

    m_StateMachine.push<detail::CoreEngineGameStateLayer>();

    UI::Init();

    bfLogPop();

    m_TimeStep    = std::chrono::milliseconds(int((1.0f / float(params.fixed_frame_rate)) * k_SecToMs));
    m_CurrentTime = UpdateLoopClock::now();

    /////

    const auto limits  = bfGfxDevice_limits(m_Renderer.device());
    m_2DScreenCommands = m_MainMemory.allocateT<CommandBuffer2D>(m_Renderer.glslCompiler(), m_Renderer.context());
    m_2DScreenUBO.create(m_Renderer.device(), BF_BUFFER_USAGE_UNIFORM_BUFFER | BF_BUFFER_USAGE_PERSISTENTLY_MAPPED_BUFFER, m_Renderer.frameInfo(), limits.uniform_buffer_offset_alignment);

    ///////
  }

  void Engine::onEvent(bfWindow* window, Event& evt)
  {
    (void)window;

    m_Input.onEvent(evt);

    // Loops backwards through the state machine stopping when the even gets accepted.
    const auto rend_state_machine_it = m_StateMachine.rend();

    for (auto it = m_StateMachine.rbegin(); !evt.isAccepted() && it != rend_state_machine_it; ++it)
    {
      it->onEvent(*this, evt);
    }
  }

  void Engine::tick()
  {
    static constexpr float k_NanoSecToSec = 1.0f / 1000000000.0f;

    const UpdateLoopTimePoint      new_time   = UpdateLoopClock::now();
    const std::chrono::nanoseconds delta_time = new_time - m_CurrentTime;

    m_CurrentTime = new_time;
    m_TimeStepLag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

    if (!m_IsInMiddleOfFrame)
    {
      m_IsInMiddleOfFrame = true;
      if (beginFrame())
      {
        const float dt_seconds   = float(std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time).count()) * k_NanoSecToSec;
        const float render_alpha = float(m_TimeStepLag.count()) / float(m_TimeStep.count());

        update(dt_seconds);

        while (m_TimeStepLag >= m_TimeStep)
        {
          fixedUpdate(float(m_TimeStep.count()) * k_NanoSecToSec);
          m_TimeStepLag -= m_TimeStep;
        }

        draw(render_alpha);
        endFrame();
      }
      m_IsInMiddleOfFrame = false;
    }
  }

  void Engine::deinit()
  {
    bfGfxDevice_flush(m_Renderer.device());

    UI::ShutDown();

    // Released any resources from the game states.
    m_StateMachine.removeAll();

    // This must happen before the Entity GC so that they are all ready to be collected.
    for (auto& scene : m_SceneStack)
    {
      scene->removeAllEntities();
    }

    // Entity Garbage Must be collected before 'm_SceneStack' is cleared.
    gc::collect(m_MainMemory);

    // Must be cleared before the Asset System destruction since it contains handles to scenes.
    m_SceneStack.clear();

    m_Assets.clearDirtyList();
    m_Assets.setRootPath(nullptr);

    assert(m_CameraList == nullptr && "All cameras not returned to the engine before shutting down.");
    deleteCameras();

    // Must happen before 'm_Renderer.deinit' since ECS systems use renderer resources.
    for (auto& system : m_Systems)
    {
      system->onDeinit(*this);
      m_MainMemory.deallocateT(system);
    }
    m_Systems.clear();

    m_2DScreenUBO.destroy(m_Renderer.device());
    m_MainMemory.deallocateT(m_2DScreenCommands);
    m_MainMemory.deallocateT(m_Gfx2D);
    m_DebugRenderer.deinit();
    m_Renderer.deinit();

    gc::quit();

    // This happens after most systems because they could be holding 'bfValueHandle's that needs to be released.
    m_Scripting.destroy();

    bfJob::shutdown();

    // Shutdown last since there could be errors logged on shutdown if any failures happen.
    bfLogRemove(&m_ConsoleLogger);
  }

  bool Engine::beginFrame()
  {
    tempMemory().clear();
    deleteCameras();
    resizeCameras();
    m_StateMachine.purgeStates();
    UI::BeginFrame();

    return m_Renderer.frameBegin();
  }

  void Engine::fixedUpdate(float delta_time)
  {
    for (auto& state : m_StateMachine)
    {
      state.onFixedUpdate(*this, delta_time);
    }
  }

  void Engine::update(float delta_time)
  {
    bfJob::tick();

    // NOTE(SR):
    //   The Debug Renderer must update before submission of debug draw commands to allow 
    //   a duration of 0.0f seconds for debug primitives that we will remove _next_ frame.
    m_DebugRenderer.update(delta_time);

    const int framebuffer_width  = int(bfTexture_width(m_Renderer.m_MainSurface));
    const int framebuffer_height = int(bfTexture_height(m_Renderer.m_MainSurface));

    // Cleared at the beginning of update so any update function can submit draw commands.
    m_Gfx2D->clear(Rect2i{0, 0, framebuffer_width, framebuffer_height});

    UI::Update(delta_time);

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

    m_Renderer.m_GlobalTime += delta_time;
  }

  void Engine::draw(float render_alpha)
  {
    const bfGfxCommandListHandle cmd_list = m_Renderer.mainCommandList();
    auto                         scene    = currentScene();

    if (scene)
    {
      for (Light& light : scene->components<Light>())
      {
        m_Renderer.addLight(light);
      }
    }

    const float framebuffer_width  = float(bfTexture_width(m_Renderer.m_MainSurface));
    const float framebuffer_height = float(bfTexture_height(m_Renderer.m_MainSurface));

    UI::Render(*m_Gfx2D, framebuffer_width, framebuffer_height);

    forEachCamera([this, render_alpha](RenderView* camera) {
      Camera_update(&camera->cpu_camera);

      if (camera->flags & RenderView::DO_DRAW)
      {
        camera->clearCommandQueues();
        m_DebugRenderer.draw(*camera, m_Renderer.m_FrameInfo);

        m_Gfx2D->renderToQueue(camera->screen_overlay_render_queue);

        for (auto& system : m_Systems)
        {
          if (system->isEnabled())
          {
            system->onFrameDraw(*this, *camera, render_alpha);
          }
        }

        for (auto& state : m_StateMachine)
        {
          state.onDraw(*this, *camera, render_alpha);
        }

        m_Renderer.renderCameraTo(*camera);
      }
    });

    m_Renderer.beginScreenPass(cmd_list);
    m_2DScreenCommands->clear(Rect2i{0, 0, (int)framebuffer_width, (int)framebuffer_height});
    m_2DScreenRenderQueue.clear();

    for (auto& state : m_StateMachine)
    {
      state.onRenderBackbuffer(*this, render_alpha);
    }

    m_2DScreenCommands->renderToQueue(m_2DScreenRenderQueue);

    {
      const auto frame_info = m_Renderer.frameInfo();

      const float k_ScaleFactorDPI = 1.0f;  // TODO(SR): Need to grab this value based on what window is being drawn onto.

      CameraOverlayUniformData* const cam_screen_data = m_2DScreenUBO.currentElement(frame_info);

      static constexpr decltype(&Mat4x4_orthoVk) orthos_fns[] = {&Mat4x4_orthoVk, &Mat4x4_ortho};

      orthos_fns[bfPlatformGetGfxAPI() == BIFROST_PLATFORM_GFX_OPENGL](
       &cam_screen_data->u_CameraProjection,
       0.0f,
       framebuffer_width / k_ScaleFactorDPI,
       framebuffer_height / k_ScaleFactorDPI,
       0.0f,
       0.0f,
       1.0f);

      m_2DScreenUBO.flushCurrent(frame_info);

      bfDescriptorSetInfo desc_set_camera = bfDescriptorSetInfo_make();

      const bfBufferSize offset = m_2DScreenUBO.offset(frame_info);
      const bfBufferSize size   = MultiBuffer<CameraOverlayUniformData>::elementSize();

      bfDescriptorSetInfo_addUniform(&desc_set_camera, 0, 0, &offset, &size, &m_2DScreenUBO.handle(), 1);

      m_2DScreenRenderQueue.execute(m_Renderer.m_MainCmdList, desc_set_camera);
    }

    m_Renderer.endPass();
    m_Renderer.drawEnd();
  }

  void Engine::endFrame()
  {
    m_Input.frameEnd();
    m_Renderer.frameEnd();

    gc::collect(m_MainMemory);
  }

  void Engine::resizeCameras()
  {
    RenderView* camera = m_CameraResizeList;

    while (camera)
    {
      RenderView* const next = std::exchange(camera->resize_list_next, nullptr);

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

      RenderView* camera = m_CameraDeleteList;

      while (camera)
      {
        RenderView* const next = camera->next;

        m_CameraMemory.deallocateT(camera);

        camera = next;
      }

      m_CameraDeleteList = nullptr;
    }
  }

  namespace detail
  {
    void CoreEngineGameStateLayer::onEvent(Engine& engine, Event& event)
    {
      (void)engine;

      // This is the bottom most layer so just accept the event.
      event.accept();
    }
  }  // namespace detail

}  // namespace bf
