#ifndef BF_ENGINE_HPP
#define BF_ENGINE_HPP

#include "bf/LinearAllocator.hpp"
#include "bf/PoolAllocator.hpp"
#include "bf/asset_io/bifrost_assets.hpp"
#include "bf/asset_io/bifrost_scene.hpp"
#include "bf/ecs/bifrost_iecs_system.hpp"
#include "bf/graphics/bifrost_debug_renderer.hpp"
#include "bf/graphics/bifrost_standard_renderer.hpp"
#include "bifrost/bifrost_vm.hpp"
#include "bifrost_game_state_machine.hpp"
#include "bifrost_igame_state_layer.hpp"

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
  static constexpr int k_MaxNumCamera = 16;

  class AnimationSystem;
  class CollisionSystem;
  class ComponentRenderer;
  class BehaviorSystem;
  struct Gfx2DPainter;

  struct CameraRenderCreateParams
  {
    int width;
    int height;
  };

  struct CameraRender
  {
    friend class Engine;

    bfGfxDeviceHandle device;
    BifrostCamera     cpu_camera;
    CameraGPUData     gpu_camera;
    int               old_width;
    int               old_height;
    int               new_width;
    int               new_height;
    CameraRender*     prev;
    CameraRender*     next;
    CameraRender*     resize_list_next;

    CameraRender(CameraRender*& head, bfGfxDeviceHandle device, bfGfxFrameInfo frame_info, const CameraRenderCreateParams& params) :
      device{device},
      cpu_camera{},
      gpu_camera{},
      old_width{params.width},
      old_height{params.height},
      new_width{params.width},
      new_height{params.height},
      prev{nullptr},
      next{head},
      resize_list_next{nullptr}
    {
      if (head)
      {
        head->prev = this;
        // head->next = /* same */;
      }

      head = this;

      const Vec3f cam_pos = {0.0f, 0.0f, 4.0f, 1.0f};
      Camera_init(&cpu_camera, &cam_pos, nullptr, 0.0f, 0.0f);
      gpu_camera.init(device, frame_info, params.width, params.height);
    }

    ~CameraRender()
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

  // using Window      = bfWindow;
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
}  // namespace bf

#include <chrono>   // high_resolution_clock
#include <utility>  // pair

namespace bf
{
  class Engine : private NonCopyMoveable<Engine>
  {
   private:
    using CommandLineArgs     = std::pair<int, char**>;
    using CameraRenderMemory  = PoolAllocator<CameraRender, k_MaxNumCamera>;
    using UpdateLoopClock     = std::chrono::high_resolution_clock;
    using UpdateLoopTimePoint = typename UpdateLoopClock::time_point;

   private:
    // Configuration

    CommandLineArgs m_CmdlineArgs;

    // Memory Allocators

    MainHeap        m_MainMemory;
    LinearAllocator m_TempMemory;
    NoFreeAllocator m_TempAdapter;

    // Core Low Level Systems

    Assets                 m_Assets;
    GameStateMachine       m_StateMachine;
    VM                     m_Scripting;
    Array<ARC<SceneAsset>> m_SceneStack;
    Input                  m_Input;

    // Rendering

    StandardRenderer   m_Renderer;
    DebugRenderer      m_DebugRenderer;
    Gfx2DPainter*      m_Renderer2D;
    CameraRenderMemory m_CameraMemory;
    CameraRender*      m_CameraList;
    CameraRender*      m_CameraResizeList;
    CameraRender*      m_CameraDeleteList;

    // IECSSystem (High Level Systems)

    Array<IECSSystem*> m_Systems;
    AnimationSystem*   m_AnimationSystem;
    CollisionSystem*   m_CollisionSystem;
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
    IMemoryManager&    tempMemoryNoFree() { return m_TempAdapter; }
    GameStateMachine&  stateMachine() { return m_StateMachine; }
    VM&                scripting() { return m_Scripting; }
    StandardRenderer&  renderer() { return m_Renderer; }
    DebugRenderer&     debugDraw() { return m_DebugRenderer; }
    Gfx2DPainter&      renderer2D() const { return *m_Renderer2D; }
    Assets&            assets() { return m_Assets; }
    Input&             input() { return m_Input; }
    AnimationSystem&   animationSys() const { return *m_AnimationSystem; }
    CollisionSystem&   collisionSys() const { return *m_CollisionSystem; }
    ComponentRenderer& rendererSys() const { return *m_ComponentRenderer; }
    BehaviorSystem&    behaviorSys() const { return *m_BehaviorSystem; }
    ARC<SceneAsset>    currentScene() const;
    EngineState        state() const { return m_State; }
    void               setState(EngineState value) { m_State = value; }

    // Low Level Camera API

    CameraRender* borrowCamera(const CameraRenderCreateParams& params);
    void          resizeCamera(CameraRender* camera, int width, int height);
    void          returnCamera(CameraRender* camera);

    template<typename F>
    void forEachCamera(F&& callback)
    {
      CameraRender* camera = m_CameraList;

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
