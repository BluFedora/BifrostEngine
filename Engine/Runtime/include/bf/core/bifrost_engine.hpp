#ifndef BF_ENGINE_HPP
#define BF_ENGINE_HPP

#include "bf/asset_io/bifrost_assets.hpp"
#include "bf/asset_io/bifrost_scene.hpp"
#include "bf/bf_dbg_logger.h"  // bfLog*
#include "bf/ecs/bifrost_iecs_system.hpp"
#include "bf/gfx/bf_render_queue.hpp"
#include "bf/graphics/bifrost_debug_renderer.hpp"
#include "bf/graphics/bifrost_standard_renderer.hpp"
#include "bifrost/bifrost_vm.hpp"
#include "bifrost_game_state_machine.hpp"
#include "bifrost_igame_state_layer.hpp"

#include <chrono>   // high_resolution_clock
#include <utility>  // pair

#define USE_CRT_HEAP 0

#if USE_CRT_HEAP
#include "bifrost/memory/bf_crt_allocator.hpp"
#else
#include "bf/FreeListAllocator.hpp"
#endif

#if USE_CRT_HEAP
using MainHeap = bf::CRTAllocator;
#else
using MainHeap = bf::FreeListAllocator;
#endif

struct EngineCreateParams : public bfGfxContextCreateParams
{
  int fixed_frame_rate = 60;
};

namespace bf
{
  static constexpr int k_MaxNumCamera = 4;

  class AnimationSystem;
  class ComponentRenderer;
  class BehaviorSystem;
  struct CommandBuffer2D;

  struct CameraRenderCreateParams
  {
    int width;
    int height;
  };

  struct RenderView
  {
    friend class Engine;

    // Flags
    static constexpr std::uint8_t DO_DRAW = bfBit(0);

    bfGfxDeviceHandle device;
    BifrostCamera     cpu_camera;
    CameraGPUData     gpu_camera;
    int               old_width;
    int               old_height;
    int               new_width;
    int               new_height;
    RenderQueue       opaque_render_queue;
    RenderQueue       transparent_render_queue;
    RenderQueue       overlay_scene_render_queue;
    RenderQueue       screen_overlay_render_queue;
    RenderView*       prev;
    RenderView*       next;
    RenderView*       resize_list_next;
    std::uint8_t      flags;

    RenderView(RenderView*& head, bfGfxDeviceHandle device, bfGfxFrameInfo frame_info, const CameraRenderCreateParams& params) :
      device{device},
      cpu_camera{},
      gpu_camera{},
      old_width{params.width},
      old_height{params.height},
      new_width{params.width},
      new_height{params.height},
      opaque_render_queue{RenderQueue::SORT_COMMANDS | RenderQueue::SORT_DEPTH_FTB},
      transparent_render_queue{RenderQueue::SORT_COMMANDS},
      overlay_scene_render_queue{RenderQueue::SORT_COMMANDS | RenderQueue::SORT_DEPTH_FTB},
      screen_overlay_render_queue{},
      prev{nullptr},
      next{head},
      resize_list_next{nullptr},
      flags{DO_DRAW}
    {
      if (head)
      {
        head->prev = this;
        // head->next = /* same */;
      }

      head = this;

      const Vec3f cam_pos = {0.0f, 0.0f, 4.0f, 1.0f};
      Camera_init(&cpu_camera, &cam_pos, nullptr, 0.0f, 0.0f);
      Camera_onResize(&cpu_camera, params.width, params.height);
      gpu_camera.init(device, frame_info, params.width, params.height);
    }

    void clearCommandQueues()
    {
      opaque_render_queue.clear();
      transparent_render_queue.clear();
      overlay_scene_render_queue.clear();
      screen_overlay_render_queue.clear();
    }

    ~RenderView()
    {
      gpu_camera.deinit(device);
    }

   private:
    void resize()
    {
      if (old_width != new_width || old_height != new_height)
      {
        Camera_onResize(&cpu_camera, unsigned(new_width), unsigned(new_height));
        gpu_camera.resize(device, new_width, new_height);

        old_width  = new_width;
        old_height = new_height;
      }
    }
  };

  using ButtonFlags = std::uint8_t;

  struct MouseInputState
  {
    Vector2i    current_pos  = {0, 0};
    Vector2i    delta_pos    = {0, 0};
    ButtonFlags button_state = 0x00000000;
  };

  class Input : NonCopyMoveable<Input>
  {
    friend class Engine;

   private:
    MouseInputState m_MouseState = {};

   private:
    void onEvent(Event& evt);
    void frameEnd();

   public:
    char KEY_STATE[1024] = {0};

    const MouseInputState& mouseState() const { return m_MouseState; }
    Vector2i               mousePos() const { return mouseState().current_pos; }
    Vector2i               mousePosDelta() const { return mouseState().delta_pos; }
  };

  enum class EngineState : std::uint8_t
  {
    RUNTIME_PLAYING,
    EDITOR_PLAYING,
    PAUSED,
  };

  class Engine : private NonCopyMoveable<Engine>
  {
   private:
    using CommandLineArgs     = std::pair<int, char**>;
    using CameraRenderMemory  = PoolAllocator<RenderView, k_MaxNumCamera>;
    using UpdateLoopClock     = std::chrono::high_resolution_clock;
    using UpdateLoopTimePoint = UpdateLoopClock::time_point;

   private:
    // Configuration / Debug

    CommandLineArgs m_CmdlineArgs;
    bfIDbgLogger    m_ConsoleLogger;

    // Memory Allocators

    MainHeap        m_MainMemory;
    LinearAllocator m_TempMemory;

    // Core Low Level Systems

    Assets                 m_Assets;
    GameStateMachine       m_StateMachine;
    VM                     m_Scripting;
    Array<ARC<SceneAsset>> m_SceneStack;
    Input                  m_Input;

    // Rendering

    StandardRenderer                      m_Renderer;
    DebugRenderer                         m_DebugRenderer;
    CommandBuffer2D*                      m_Gfx2D;
    CommandBuffer2D*                      m_2DScreenCommands;
    RenderQueue                           m_2DScreenRenderQueue;
    MultiBuffer<CameraOverlayUniformData> m_2DScreenUBO;
    CameraRenderMemory                    m_CameraMemory;
    RenderView*                           m_CameraList;
    RenderView*                           m_CameraResizeList;
    RenderView*                           m_CameraDeleteList;

    // IECSSystem (High Level Systems)

    Array<IECSSystem*> m_Systems;
    AnimationSystem*   m_AnimationSystem;
    ComponentRenderer* m_ComponentRenderer;
    BehaviorSystem*    m_BehaviorSystem;

    // Update Loop

    std::chrono::nanoseconds m_TimeStep;
    std::chrono::nanoseconds m_TimeStepLag;
    UpdateLoopTimePoint      m_CurrentTime;

    // Misc

    EngineState m_State;
    bool        m_IsInMiddleOfFrame;

   public:
    explicit Engine(char* main_memory, std::size_t main_memory_size, int argc = 0, char* argv[] = nullptr);

    // Subsystem Accessors

    MainHeap&          mainMemory() { return m_MainMemory; }
    LinearAllocator&   tempMemory() { return m_TempMemory; }
    GameStateMachine&  stateMachine() { return m_StateMachine; }
    VM&                scripting() { return m_Scripting; }
    StandardRenderer&  renderer() { return m_Renderer; }
    DebugRenderer&     debugDraw() { return m_DebugRenderer; }
    CommandBuffer2D&   gfx2D() const { return *m_Gfx2D; }
    CommandBuffer2D&   gfx2DScreen() const { return *m_2DScreenCommands; }
    Assets&            assets() { return m_Assets; }
    Input&             input() { return m_Input; }
    AnimationSystem&   animationSys() const { return *m_AnimationSystem; }
    ComponentRenderer& rendererSys() const { return *m_ComponentRenderer; }
    BehaviorSystem&    behaviorSys() const { return *m_BehaviorSystem; }
    ARC<SceneAsset>    currentScene() const;
    EngineState        state() const { return m_State; }
    void               setState(EngineState value) { m_State = value; }

    // Low Level Camera API

    RenderView* borrowCamera(const CameraRenderCreateParams& params);
    void        resizeCamera(RenderView* camera, int width, int height);
    void        returnCamera(RenderView* camera);

    template<typename F>
    void forEachCamera(F&& callback)
    {
      RenderView* camera = m_CameraList;

      while (camera)
      {
        callback(camera);
        camera = camera->next;
      }
    }

    // Scene Management API

    void      openScene(const ARC<SceneAsset>& scene);
    EntityRef createEntity(Scene& scene, const StringRange& name = "New Entity");

    // "System" Functions to be called by the Application

    template<typename T, typename... Args>
    T* addECSSystem(Args&&... args)
    {
      T* const sys = m_MainMemory.allocateT<T>(std::forward<Args>(args)...);
      m_Systems.push(sys);
      sys->onInit(*this);

      return sys;
    }

    void init(const EngineCreateParams& params, bfWindow* main_window);
    void onEvent(bfWindow* window, Event& evt);
    void tick();
    void deinit();

   private:
    //
    // Engine::tick() calls these functions in this order.
    // If 'Engine::beginFrame' returns false no other function
    // is called that frame.
    //

    [[nodiscard]] bool beginFrame();
    void               fixedUpdate(float delta_time);
    void               update(float delta_time);
    void               draw(float render_alpha);
    void               endFrame();

   private:
    void resizeCameras();
    void deleteCameras();
  };
}  // namespace bf

#endif /* BF_ENGINE_HPP */
