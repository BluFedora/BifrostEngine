//
// Shareef Abdoul-Raheem
// Tests a minimal use of the engine.
//
#include "bf/bf_ui.hpp"
#include "bf/gfx/bf_draw_2d.hpp"
#include <bf/bifrost.hpp>

#include "bf/JobSystem.hpp"

#include "PxPhysicsAPI.h"

#if BIFROST_PLATFORM_WINDOWS
#include <malloc.h>  // _aligned_malloc, _aligned_free
#else
#include <cstdlib>
#endif

/// PhysX Learning

namespace bf
{
  class PhysicsAllocator final : public physx::PxAllocatorCallback
  {
   public:
    void* allocate(size_t size, const char* typeName, const char* filename, int line) override
    {
      (void)typeName;
      (void)filename;
      (void)line;
#if BIFROST_PLATFORM_WINDOWS
      return _aligned_malloc(size, 16);
#else
      return std::aligned_alloc(16, size);
#endif
    }

    void deallocate(void* ptr) override
    {
#if BIFROST_PLATFORM_WINDOWS
      _aligned_free(ptr);
#else
      std::free(ptr);
#endif
    }
  };

  class PhysicsErrorCallback final : public physx::PxErrorCallback
  {
    void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override
    {
      constexpr auto serious_error_mask = physx::PxErrorCode::eABORT |
                                          physx::PxErrorCode::eINVALID_PARAMETER |
                                          physx::PxErrorCode::eINVALID_OPERATION |
                                          physx::PxErrorCode::eINTERNAL_ERROR |
                                          physx::PxErrorCode::eOUT_OF_MEMORY;

      bfLogError("[PHYSX](%i): %s (%s:%i)", code, message, file, line);

      if (code & serious_error_mask)
      {
#if BIFROST_PLATFORM_WINDOWS
        __debugbreak();
        assert(false);
#endif
      }
    }
  };

  class PhysicsCPUDispatcher final : public physx::PxCpuDispatcher
  {
   public:
    void submitTask(physx::PxBaseTask& task) override
    {
      job::Task* const bf_task = job::taskMake(
       [](job::Task* task) {
         physx::PxBaseTask* const physics_task = job::taskDataAs<physx::PxBaseTask*>(task);

         physics_task->run();
         physics_task->release();
       },
       nullptr);

      job::taskEmplaceData<physx::PxBaseTask*>(bf_task, &task);

      job::taskSubmit(bf_task, job::QueueType::HIGH);
    }

    uint32_t getWorkerCount() const override
    {
      return std::uint32_t(bfJob::numWorkers());
    }
  };

  class Physics
  {
   private:
    static physx::PxFilterFlags SampleSubmarineFilterShader(
     physx::PxFilterObjectAttributes attributes0,
     physx::PxFilterData             filterData0,
     physx::PxFilterObjectAttributes attributes1,
     physx::PxFilterData             filterData1,
     physx::PxPairFlags&             pairFlags,
     const void*                     constantBlock,
     physx::PxU32                    constantBlockSize)
    {
      // let triggers through
      if (physx::PxFilterObjectIsTrigger(attributes0) || physx::PxFilterObjectIsTrigger(attributes1))
      {
        pairFlags = physx::PxPairFlag::eTRIGGER_DEFAULT;
        return physx::PxFilterFlag::eDEFAULT;
      }
      // generate contacts for all that were not filtered above
      pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

      // trigger the contact callback for pairs (A,B) where
      // the filtermask of A contains the ID of B and vice versa.
      if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
        pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;

      return physx::PxFilterFlag::eDEFAULT;
    }

   private:
    PhysicsAllocator      m_Allocator     = {};
    PhysicsErrorCallback  m_ErrorCallback = {};
    physx::PxFoundation*  m_PxFoundation  = nullptr;
    physx::PxPhysics*     m_PhysX         = nullptr;
    PhysicsCPUDispatcher* m_CPUDispatcher = nullptr;
    physx::PxScene*       m_MainScene     = nullptr;

   public:
    void init()
    {
      m_PxFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_ErrorCallback);

      assert(m_PxFoundation && "Failed to initialize PhysX Foundation!");

      physx::PxPvd* pvd_instance;

#if 0
      pvd_instance                     = physx::PxCreatePvd(*m_PxFoundation);
      physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
      pvd_instance->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
#else
      pvd_instance = nullptr;
#endif

      const bool do_track_allocations = true;

      physx::PxTolerancesScale tolerance_scale = {};

      m_PhysX = PxCreatePhysics(PX_PHYSICS_VERSION, *m_PxFoundation, tolerance_scale, do_track_allocations, pvd_instance);

      assert(m_PhysX && "Failed to initialize PhysX Physics Object!");

#if 0
      mCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(scale));
if (!mCooking)
    fatalError("PxCreateCooking failed!");

      if (!PxInitExtensions(*m_PhysX, pvd_instance))
      {
        assert(false);
      }
#endif

      m_CPUDispatcher = new PhysicsCPUDispatcher();

      physx::PxSceneDesc scene_desc = {tolerance_scale};

      // Set `scene_desc` callbacks here

      scene_desc.gravity = {0.0f, -9.8f * 0.4f, 0.0f};

      scene_desc.filterShader  = &SampleSubmarineFilterShader;
      scene_desc.cpuDispatcher = m_CPUDispatcher;

      m_MainScene = m_PhysX->createScene(scene_desc);

      // Test Code

      using namespace physx;

      PxRigidStatic* floor_actor = m_PhysX->createRigidStatic(PxTransform(PxMat44(PxIdentity)));
      PxShape* const floor_shape = m_PhysX->createShape(
       PxBoxGeometry(10.0f, 0.2f, 10.0f),
       *m_PhysX->createMaterial(0.5f, 0.5f, 0.1f),
       true,
       PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE);

      floor_actor->attachShape(*floor_shape);
      floor_actor->setGlobalPose(PxTransform({0.0f, -5.0f, 0.0f}));

      m_MainScene->addActor(*floor_actor);
    }

    void addActor()
    {
      using namespace physx;

      // m_PhysX->createAggregate()

      PxRigidDynamic* actor = m_PhysX->createRigidDynamic(PxTransform(PxMat44(PxIdentity)));

      PxMaterial*    material = m_PhysX->createMaterial(0.5f, 0.5f, 0.1f);
      PxShape* const shape    = m_PhysX->createShape(
       // PxSphereGeometry(1.0f),
       PxBoxGeometry(0.5f, 0.5f, 0.5f),
       *material,
       true,
       PxShapeFlag::eSIMULATION_SHAPE | PxShapeFlag::eSCENE_QUERY_SHAPE);

      actor->attachShape(*shape);

      actor->setLinearVelocity({0.2f, 0.0f, 0.0f}, true);

      shape->release();

      m_MainScene->addActor(*actor);
    }

    void beginFrame(float dt)
    {
      m_MainScene->simulate(dt, nullptr, nullptr, 0u, true);
    }

    void endFrame()
    {
      m_MainScene->fetchResults(true, nullptr);
    }

    void draw(DebugRenderer& dbg_draw)
    {
      physx::PxActor* actors[256];
      physx::PxU32    num_actors = m_MainScene->getActors(physx::PxActorTypeFlag::eRIGID_STATIC | physx::PxActorTypeFlag::eRIGID_DYNAMIC, actors, physx::PxU32(bfCArraySize(actors)), 0);

      for (physx::PxU32 i = 0; i < num_actors; ++i)
      {
        physx::PxActor* const actor         = actors[i];
        physx::PxBounds3      bounds        = actor->getWorldBounds(1.0f);
        physx::PxVec3         bounds_center = bounds.getCenter();
        physx::PxVec3         bounds_dim    = bounds.getDimensions();
        physx::PxTransform    global_pose;

        physx::PxShape* shapes[128];
        physx::PxU32    num_shapes;

        switch (actor->getType())
        {
          case physx::PxActorType::eRIGID_STATIC:
          {
            physx::PxRigidStatic* const rb = actor->is<physx::PxRigidStatic>();

            global_pose = rb->getGlobalPose();
            num_shapes  = rb->getShapes(shapes, physx::PxU32(bfCArraySize(shapes)), 0);
            break;
          }
          case physx::PxActorType::eRIGID_DYNAMIC:
          {
            physx::PxRigidDynamic* const rb = actor->is<physx::PxRigidDynamic>();

            global_pose = rb->getGlobalPose();
            num_shapes  = rb->getShapes(shapes, physx::PxU32(bfCArraySize(shapes)), 0);
            break;
          }
          case physx::PxActorType::eARTICULATION_LINK:
          case physx::PxActorType::eACTOR_COUNT:
          case physx::PxActorType::eACTOR_FORCE_DWORD:
          default:
            num_shapes = 0u;
            break;
        }

        for (physx::PxU32 j = 0; j < num_shapes; ++j)
        {
          physx::PxShape* const   shape    = shapes[j];
          physx::PxGeometryHolder geometry = shape->getGeometry();

          switch (shape->getGeometryType())
          {
            case physx::PxGeometryType::eSPHERE:
            {
              break;
            }
            case physx::PxGeometryType::ePLANE:
            {
              break;
            }
            case physx::PxGeometryType::eCAPSULE:
            {
              break;
            }
            case physx::PxGeometryType::eBOX:
            {
              const physx::PxBoxGeometry& box      = geometry.box();
              const physx::PxVec3         half_ext = box.halfExtents;

              physx::PxVec3 min_max_points[] =
               {
                {-half_ext.x, -half_ext.y, -half_ext.z},
                {+half_ext.x, -half_ext.y, -half_ext.z},
                {-half_ext.x, +half_ext.y, -half_ext.z},
                {+half_ext.x, +half_ext.y, -half_ext.z},
                {-half_ext.x, -half_ext.y, +half_ext.z},
                {+half_ext.x, -half_ext.y, +half_ext.z},
                {-half_ext.x, +half_ext.y, +half_ext.z},
                {+half_ext.x, +half_ext.y, +half_ext.z},
               };

              for (auto& point : min_max_points)
              {
                point = global_pose.transform(point);
              }

              const Vector3f points_to_draw[] =
               {
                {min_max_points[0].x, min_max_points[0].y, min_max_points[0].z},
                {min_max_points[1].x, min_max_points[1].y, min_max_points[1].z},
                {min_max_points[2].x, min_max_points[2].y, min_max_points[2].z},
                {min_max_points[3].x, min_max_points[3].y, min_max_points[3].z},
                {min_max_points[4].x, min_max_points[4].y, min_max_points[4].z},
                {min_max_points[5].x, min_max_points[5].y, min_max_points[5].z},
                {min_max_points[6].x, min_max_points[6].y, min_max_points[6].z},
                {min_max_points[7].x, min_max_points[7].y, min_max_points[7].z},
               };

              const auto line_clr = bfColor4u_fromUint32(0xFFFFFFFF);

              dbg_draw.addLine(points_to_draw[0], points_to_draw[1], line_clr);
              dbg_draw.addLine(points_to_draw[0], points_to_draw[2], line_clr);
              dbg_draw.addLine(points_to_draw[0], points_to_draw[4], line_clr);
              dbg_draw.addLine(points_to_draw[1], points_to_draw[3], line_clr);
              dbg_draw.addLine(points_to_draw[1], points_to_draw[5], line_clr);
              dbg_draw.addLine(points_to_draw[2], points_to_draw[3], line_clr);
              dbg_draw.addLine(points_to_draw[2], points_to_draw[6], line_clr);
              dbg_draw.addLine(points_to_draw[3], points_to_draw[7], line_clr);
              dbg_draw.addLine(points_to_draw[4], points_to_draw[5], line_clr);
              dbg_draw.addLine(points_to_draw[4], points_to_draw[6], line_clr);
              dbg_draw.addLine(points_to_draw[5], points_to_draw[7], line_clr);
              dbg_draw.addLine(points_to_draw[6], points_to_draw[7], line_clr);
              break;
            }
            case physx::PxGeometryType::eCONVEXMESH:
            {
              break;
            }
            case physx::PxGeometryType::eTRIANGLEMESH:
            {
              break;
            }
            case physx::PxGeometryType::eHEIGHTFIELD:
            {
              break;
            }
            default:
              break;
          }
        }
      }
    }

    void shutdown()
    {
      m_MainScene->release();
      delete m_CPUDispatcher;
      m_PhysX->release();
      m_PxFoundation->release();
    }
  };
}  // namespace bf

/// PhysX Learning

// Physics Concepts
//
// RigidBody
//   Type: Dynamic   (Normal)
//         Kinematic (Moved Manually by forces, but can hit things, not affected by gravity)
//         Static    (Cannot be moved)
//
//   Shapes[]: Lists of shapes
//
//
// Shape:
//   Flags: IsTrigger (IsSolid must be false)
//          IsSolid   (IsTrigger must be false)
//
//   Geometry: The shape to use.
//
//   Relative pose.
//
//
// Geometry: Collision shape centered at origin.
//
//   Plane: Only can be used with static actors.
//
//

using namespace bf;

struct RuntimeGameState final : public IGameStateLayer
{
  bf::RenderView* m_MainCamera = nullptr;
  Physics         m_PhysicTest = {};

  void onCreate(Engine& engine) override
  {
    m_PhysicTest.init();
    m_MainCamera = engine.borrowCamera(CameraRenderCreateParams{1280, 720});
  }

  void onUpdate(Engine& engine, float delta_time) override
  {
    m_PhysicTest.draw(engine.debugDraw());

    const auto camera_move_speed = 2.2f * delta_time;

    const std::tuple<int, void (*)(::BifrostCamera*, float), float> camera_controls[] =
     {
      {BIFROST_KEY_W, &Camera_moveForward, camera_move_speed},
      {BIFROST_KEY_A, &Camera_moveLeft, camera_move_speed},
      {BIFROST_KEY_S, &Camera_moveBackward, camera_move_speed},
      {BIFROST_KEY_D, &Camera_moveRight, camera_move_speed},
      {BIFROST_KEY_Q, &Camera_moveUp, camera_move_speed},
      {BIFROST_KEY_E, &Camera_moveDown, camera_move_speed},
      {BIFROST_KEY_R, &Camera_addPitch, -0.01f},
      {BIFROST_KEY_F, &Camera_addPitch, +0.01f},
      {BIFROST_KEY_H, &Camera_addYaw, +0.01f},
      {BIFROST_KEY_G, &Camera_addYaw, -0.01f},
     };

    for (const auto& control : camera_controls)
    {
      if (engine.input().KEY_STATE[std::get<0>(control)])
      {
        std::get<1>(control)(&m_MainCamera->cpu_camera, std::get<2>(control));
      }
    }

    static WindowState s_PhysicsWindowState = {true, {5, 5}, {200.0f, 100.0f}};

    if (UI::BeginWindow("Physics Test", s_PhysicsWindowState))
    {
      if (UI::Button("Add Cube"))
      {
        m_PhysicTest.addActor();
      }

      UI::EndWindow();
    }
  }

  void onFixedUpdate(Engine& engine, float delta_time) override
  {
    //
    m_PhysicTest.beginFrame(delta_time);
    {
      // Physics is Simulating
    }
    m_PhysicTest.endFrame();
    //
  }

  void onRenderBackbuffer(Engine& engine, float alpha) override
  {
    const float framebuffer_width  = float(bfTexture_width(engine.renderer().m_MainSurface));
    const float framebuffer_height = float(bfTexture_height(engine.renderer().m_MainSurface));

    engine.resizeCamera(m_MainCamera, int(framebuffer_width), int(framebuffer_height));

    auto& gfx = engine.gfx2DScreen();

    const Brush* const green_brush  = gfx.makeBrush(0xFF00FFFF);
    const Brush* const screen_brush = gfx.makeBrush(m_MainCamera->gpu_camera.composite_buffer);

    gfx.fillRect(screen_brush, AxisQuad::make(Rect2f{0.0f, 0.0f, framebuffer_width, framebuffer_height}));
    gfx.fillRect(green_brush, AxisQuad::make(Rect2f{float(engine.input().mousePos().x), float(engine.input().mousePos().y), 5.0f, 5.0f}));
  }

  void onDestroy(Engine& engine) override
  {
    engine.returnCamera(m_MainCamera);
    m_PhysicTest.shutdown();
  }

  const char* name() override { return "RuntimeGameState"; }

  const float k_InvalidMousePos = -1.0f;
  Vector2f    m_OldMousePos     = {k_InvalidMousePos, k_InvalidMousePos};
  bool        m_IsDraggingMouse = false;
  float       m_MouseLookSpeed  = 0.01f;

  void onEvent(Engine& engine, bfEvent& event) override
  {
    auto& mouse_evt = event.mouse;

    if (event.type == BIFROST_EVT_ON_MOUSE_DOWN || event.type == BIFROST_EVT_ON_MOUSE_UP)
    {
      m_OldMousePos = {k_InvalidMousePos, k_InvalidMousePos};

      if (event.type == BIFROST_EVT_ON_MOUSE_DOWN)
      {
        if (mouse_evt.target_button == BIFROST_BUTTON_MIDDLE)
        {
          m_IsDraggingMouse = true;
        }
      }
      else
      {
        m_IsDraggingMouse = false;
        event.accept();
      }
    }
    else if (event.type == BIFROST_EVT_ON_MOUSE_MOVE)
    {
      if (m_IsDraggingMouse && mouse_evt.button_state & BIFROST_BUTTON_MIDDLE)
      {
        const float newx = float(mouse_evt.x);
        const float newy = float(mouse_evt.y);

        if (m_OldMousePos.x == k_InvalidMousePos) { m_OldMousePos.x = newx; }
        if (m_OldMousePos.y == k_InvalidMousePos) { m_OldMousePos.y = newy; }

        if (m_MainCamera)
        {
          Camera_mouse(
           &m_MainCamera->cpu_camera,
           (newx - m_OldMousePos.x) * m_MouseLookSpeed,
           (newy - m_OldMousePos.y) * -m_MouseLookSpeed);
        }

        m_OldMousePos.x = newx;
        m_OldMousePos.y = newy;
      }
    }

    UI::PumpEvents(&event);
  }
};

enum class ReturnCode : int
{
  SUCCESS,
  FAILED_TO_INITIALIZE_PLATFORM,
  FAILED_TO_CREATE_MAIN_WINDOW,
  FAILED_TO_ALLOCATE_ENGINE_MEMORY,
};

#define MainQuit(code, label) \
  err_code = (code);          \
  goto label

int main(int argc, char* argv[])
{
  ReturnCode err_code = ReturnCode::SUCCESS;

  if (bfPlatformInit({argc, argv, nullptr, nullptr}))
  {
    bfWindow* const main_window = bfPlatformCreateWindow("Runtime Standalone Test", 1280, 720, k_bfWindowFlagsDefault & ~k_bfWindowFlagIsMaximizedOnShow);

    if (main_window)
    {
      try
      {
        const std::size_t             engine_memory_size = bfMegabytes(100);
        const std::unique_ptr<char[]> engine_memory      = std::make_unique<char[]>(engine_memory_size);
        std::unique_ptr<Engine>       engine             = std::make_unique<Engine>(engine_memory.get(), engine_memory_size, argc, argv);
        const EngineCreateParams      params             = {{argv[0], 0}, 60};

        engine->init(params, main_window);
        engine->stateMachine().push<RuntimeGameState>();

        main_window->user_data = engine.get();
        main_window->event_fn  = [](bfWindow* window, bfEvent* event) { static_cast<Engine*>(window->user_data)->onEvent(window, *event); };
        main_window->frame_fn  = [](bfWindow* window) { static_cast<Engine*>(window->user_data)->tick(); };

        bfPlatformDoMainLoop(main_window);
        engine->deinit();
      }
      catch (std::bad_alloc&)
      {
        MainQuit(ReturnCode::FAILED_TO_ALLOCATE_ENGINE_MEMORY, quit_window);
      }

    quit_window:
      bfPlatformDestroyWindow(main_window);
    }
    else
    {
      MainQuit(ReturnCode::FAILED_TO_CREATE_MAIN_WINDOW, quit_platform);
    }
  }
  else
  {
    MainQuit(ReturnCode::FAILED_TO_INITIALIZE_PLATFORM, quit_main);
  }

quit_platform:
  bfPlatformQuit();

quit_main:
  return int(err_code);
}

#undef MainQuit
