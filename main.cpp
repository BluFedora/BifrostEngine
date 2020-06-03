#define NOMINMAX

#include "bifrost/ecs/bifrost_collision_system.hpp"
#include "demo/game_state_layers/main_demo.hpp"

#include <bifrost/bifrost.hpp>
#include <bifrost/bifrost_version.h>
#include <bifrost/editor/bifrost_editor_overlay.hpp>
#include <bifrost/platform/bifrost_window_glfw.hpp>
#include <bifrost_editor/bifrost_imgui_glfw.hpp>

#include <chrono>
#include <functional>
#include <future>
#include <glfw/glfw3.h>
#include <iostream>
#include <utility>

#if BIFROST_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif BIFROST_PLATFORM_MACOS
// #define GLFW_EXPOSE_NATIVE_COCOA
#endif

#include <glfw/glfw3native.h>

#undef GLFW_EXPOSE_NATIVE_WIN32
#undef GLFW_EXPOSE_NATIVE_COCOA

struct TestClass final
{
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

void testFN()
{
  std::cout << "does this work?" << std::endl;
}

static char source[] = R"(
  import "main"    for TestClass, cppFn, BigFunc, AnotherOne;
  import "bifrost" for Camera;
  import "std:io"  for print;

  class MyBase
  {
    func ctor()
    {
      print("MyBase::ctor");
    }

    func baseClassCall()
    {
      print("This is from the base class.");
    }
  };

  class Derived : MyBase
  {
    func ctor()
    {
      super.ctor(self);
      print("Derived::ctor");
    }

    func test()
    {
      print(super);
      self:baseClassCall();
    }
  };

  var d = new Derived();
  d:test();

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

  func update(dt)
  {
    cam:update(dt);

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

namespace ErrorCodes
{
  static constexpr int GLFW_FAILED_TO_INIT = -1;
}  // namespace ErrorCodes

namespace bifrost
{
  class Camera
  {
   private:
    float m_ElapsedTime = 0.0f;

   public:
    void update(float dt)
    {
      m_ElapsedTime += dt;
    }
  };
}  // namespace bifrost

static GLFWmonitor* get_current_monitor(GLFWwindow* window)
{
  int nmonitors;
  int wx, wy, ww, wh;

  int           bestoverlap = 0;
  GLFWmonitor*  bestmonitor = nullptr;
  GLFWmonitor** monitors    = glfwGetMonitors(&nmonitors);

  glfwGetWindowPos(window, &wx, &wy);
  glfwGetWindowSize(window, &ww, &wh);

  for (int i = 0; i < nmonitors; i++)
  {
    int                mx, my;
    const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
    const int          mw   = mode->width;
    const int          mh   = mode->height;

    glfwGetMonitorPos(monitors[i], &mx, &my);

    const int overlap = std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx)) * std::max(
                                                                                      0, std::min(wy + wh, my + mh) - std::max(wy, my));

    if (bestoverlap < overlap)
    {
      bestoverlap = overlap;
      bestmonitor = monitors[i];
    }
  }

  return bestmonitor;
}

static constexpr int INITIAL_WINDOW_SIZE[] = {1280, 720};

#ifdef _WIN32
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement                = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif  //  _WIN32

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

using TestCaseFn = void (*)();

static void Test2DTransform()
{
  //
  // This is a test showing you can
  // replace a matrix multiplication with an add of axis vectors for
  // 2D sprite like objects.
  //

  const glm::vec3 translation = glm::vec3{30.0f, 20.0f, 10.0f};
  const glm::mat4 transform =
   glm::translate(translation) *
   glm::eulerAngleXYZ(glm::radians(65.0f), glm::radians(20.0f), glm::radians(20.0f)) *
   glm::scale(glm::identity<glm::mat4>(), glm::vec3{1.5f, 3.4f, 9.1f});

  const glm::vec4 points0[] =
   {
    transform * glm::vec4{0.0f, 0.0f, 0.0f, 1.0},
    transform * glm::vec4{1.0f, 0.0f, 0.0f, 1.0},
    transform * glm::vec4{0.0f, 1.0f, 0.0f, 1.0},
    transform * glm::vec4{1.0f, 1.0f, 0.0f, 1.0},
   };

  const glm::vec4 x_axis = transform * glm::vec4{1.0f, 0.0, 0.0, 0.0};
  const glm::vec4 y_axis = transform * glm::vec4{0.0f, 1.0, 0.0, 0.0};

  const glm::vec4 points1[] =
   {
    glm::vec4{translation, 1.0f},
    glm::vec4{translation, 1.0f} + x_axis,
    glm::vec4{translation, 1.0f} + y_axis,
    glm::vec4{translation, 1.0f} + x_axis + y_axis,
   };

  for (int i = 0; i < 4; ++i)
  {
    if (points0[i] != points1[i])
    {
#if BIFROST_PLATFORM_WINDOWS
      // This test failed.
      // __debugbreak();
      DebugBreak();
#endif
    }
  }
}

static void TestQuaternions()
{
  Quaternionf rotation_quat = bfQuaternionf_init(0.7536897f, -0.2228076f, 0.4766513f, -0.3938427f);
  Vector3f    rotation_euler;
  bfQuaternionf_toEulerRad(&rotation_quat, &rotation_euler);

  std::printf("To:   Quaternion(%f, %f, %f, %f)\n", rotation_quat.x, rotation_quat.y, rotation_quat.z, rotation_quat.w);
  std::printf("To:   EulerAngles(%f, %f, %f    )\n", rotation_euler.x, rotation_euler.y, rotation_euler.z);

  rotation_quat = bfQuaternionf_fromEulerRad(rotation_euler.x, rotation_euler.y, rotation_euler.z);

  bfQuaternionf_toEulerRad(&rotation_quat, &rotation_euler);

  std::printf("From: Quaternion(%f, %f, %f, %f)\n", rotation_quat.x, rotation_quat.y, rotation_quat.z, rotation_quat.w);
  std::printf("From: EulerAngles(%f, %f, %f    )\n", rotation_euler.x, rotation_euler.y, rotation_euler.z);
}

static void TestApplyReturningVoid_01(float t)
{
  std::printf("TestApplyReturningVoid_01(%f)\n", t);
}

static float TestApplyReturningVoid_02(float t)
{
  std::printf("TestApplyReturningVoid_02(%f)\n", t);
  return t;
}

template<typename F>
static decltype(auto) TestApplyReturningVoid_Test(F&& func, const std::tuple<float>& args)
{
  return std::apply(func, args);
}

static void TestApplyReturningVoid()
{
  const std::tuple<float> args{5.3f};

  TestApplyReturningVoid_Test(TestApplyReturningVoid_01, args);
  TestApplyReturningVoid_Test(TestApplyReturningVoid_02, args);
}

enum class MetaSystemEnumTest
{
  E0,
  E1,
};

struct MetaSystemObjectTest : BaseObject<MetaSystemObjectTest>
{
  int field = 5;

  MetaSystemEnumTest TestFunction(int t)
  {
    std::printf("MetaSystemObjectTest::TestFunction(%f)\n", float(t));
    return MetaSystemEnumTest::E1;
  }
};

BIFROST_META_REGISTER(MetaSystemEnumTest){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   enum_info<MetaSystemEnumTest>("MetaSystemEnumTest"),  //
   enum_element("E0", MetaSystemEnumTest::E0),           //
   enum_element("E1", MetaSystemEnumTest::E1)            //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(MetaSystemObjectTest)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<MetaSystemObjectTest>("MetaSystemObjectTest"),      //
     field("field", &MetaSystemObjectTest::field),                  //
     function("TestFunction", &MetaSystemObjectTest::TestFunction)  //
    )
  BIFROST_META_END()
}

static void TestMetaSystem()
{
  MetaSystemObjectTest test;

  const auto test_class_info = test.type();
  const auto test_fn         = test_class_info->findMethod("TestFunction");

  const auto result0 = test_fn->invoke(&test, "Non Compatible");
  const auto result1 = test_fn->invoke(&test, 3.4f);
  const auto result2 = test_fn->invoke(&test, 6);

  std::printf("result0.type = %i\n", int(result0.type()));
  std::printf("result1.type = %i\n", int(result1.type()));
  std::printf("result2.type = %i\n", int(result2.type()));

  if (result1.is<meta::MetaObject>())
  {
    const meta::MetaObject& meta_obj = result1.as<meta::MetaObject>();

    auto enum_str = meta_obj.type_info->enumToString((MetaSystemEnumTest)meta_obj.enum_value);

    std::printf("result1.value = %.*s\n", int(enum_str.length()), enum_str.begin());
  }

  std::printf("test.field = %i\n", test.field);

  test_class_info->findProperty("field")->set(makeVariant(&test), 3.0f);

  std::printf("test.field = %i\n", test.field);
}

static constexpr TestCaseFn s_Test[] = {&Test2DTransform, &TestQuaternions, &TestApplyReturningVoid, &TestMetaSystem};

int main(int argc, const char* argv[])  // NOLINT(bugprone-exception-escape)
{
  for (const auto& test_fn : s_Test)
  {
    test_fn();
  }

  namespace bfmeta = meta;
  namespace bf     = bifrost;

  bfLogSetColor(BIFROST_LOGGER_COLOR_CYAN, BIFROST_LOGGER_COLOR_GREEN, BIFROST_LOGGER_COLOR_FG_BOLD);
  std::printf("\n\n                 Bifrost Engine v%s\n\n\n", BIFROST_VERSION_STR);
  bfLogSetColor(BIFROST_LOGGER_COLOR_BLUE, BIFROST_LOGGER_COLOR_WHITE, 0x0);

  TestClass my_obj = {74, "This message will be in Y"};

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

  const std::size_t main_memory_size = 100000000u;
  char*             main_memory      = new char[main_memory_size];

  {
    WindowGLFW    window{};
    BifrostEngine engine{window, main_memory, main_memory_size, argc, argv};

    engine.addECSSystem<CollisionSystem>();

    if (!window.open("Mjolnir Editor 2020"))
    {
      return -1;
    }

    g_Window = window.handle();

    glfwSetWindowSizeLimits(window.handle(), 300, 70, GLFW_DONT_CARE, GLFW_DONT_CARE);

    const BifrostEngineCreateParams params =
     {
      {
       argv[0],
       0,
#if BIFROST_PLATFORM_WINDOWS
       GetModuleHandle(nullptr),
       glfwGetWin32Window(window.handle()),
#elif BIFROST_PLATFORM_MACOS
       nullptr,
       window.handle(),
#else
#error Unsupported platform.
#endif
      },
      INITIAL_WINDOW_SIZE[0],
      INITIAL_WINDOW_SIZE[1],
     };

    engine.init(params);

    imgui::startup(engine.renderer().context(), window);

    engine.stateMachine().push<MainDemoLayer>();
    engine.stateMachine().addOverlay<editor::EditorOverlay>();

    VM& vm = engine.scripting();

    const BifrostVMClassBind camera_clz_bindings = bf::vmMakeClassBinding<Camera>("Camera", bf::vmMakeCtorBinding<Camera>());

    vm.stackResize(5);

    vm.moduleLoad(0, BIFROST_VM_STD_MODULE_ALL);
    vm.moduleMake(0, "bifrost");
    vm.stackStore(0, camera_clz_bindings);
    vm.stackLoadVariable(1, 0, "Camera");
    vm.stackStore(1, "update", &Camera::update);

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

    const auto CurrentTimeSeconds = []() -> float {
      return float(glfwGetTime());
    };

    static constexpr float frame_rate       = 60.0f;
    static constexpr float min_frame_rate   = 4.0f;
    static constexpr float time_step_ms     = 1.0f / frame_rate;
    static constexpr float max_time_step_ms = 1.0f / min_frame_rate;

    float current_time     = CurrentTimeSeconds();
    float time_accumulator = 0.0f;

    while (!window.wantsToClose())
    {
      const float new_time   = CurrentTimeSeconds();
      const float delta_time = std::min(new_time - current_time, max_time_step_ms);

      current_time = new_time;
      time_accumulator += delta_time;

      int window_width, window_height;
      glfwGetWindowSize(window.handle(), &window_width, &window_height);

      glfwPollEvents();

      while (window.hasNextEvent())
      {
        Event evt = window.getNextEvent();

        if (evt.type == EventType::ON_KEY_DOWN && evt.keyboard.key == 'P' && evt.keyboard.modifiers & KeyboardEvent::CONTROL)
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

          evt.accept();
        }

        imgui::onEvent(evt);
        engine.onEvent(evt);
      }

      if (engine.beginFrame())
      {
        StandardRenderer& renderer = engine.renderer();

        if (update_fn)
        {
          vm.stackResize(1);
          vm.stackLoadHandle(0, update_fn);

          if (vm.stackGetType(0) == BIFROST_VM_FUNCTION)
          {
            vm.call(0, delta_time);
          }
        }

        imgui::beginFrame(
         renderer.surface(),
         float(window_width),
         float(window_height),
         new_time);

        while (time_accumulator >= time_step_ms)
        {
          engine.fixedUpdate(time_step_ms);
          time_accumulator -= time_step_ms;
        }

        engine.update(delta_time);

        const float render_alpha = time_accumulator / time_step_ms;  // current_state * alpha + previous_state * (1.0f - alpha)

        engine.drawBegin(render_alpha);
        imgui::endFrame(renderer.mainCommandList());
        engine.drawEnd();
      }
    }

    vm.stackDestroyHandle(update_fn);
    imgui::shutdown();
    engine.deinit();
    window.close();
  }

  delete[] main_memory;

  shutdownGLFW();

  return 0;
}

#if 0
  window("Game State Machine", [&engine]() {
    ImGui::Text("Sprite ID: %i", SpriteComponent::s_ComponentID);
    ImGui::Text("Mesh ID: %i", MeshComponent::s_ComponentID);

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
#endif

GLFWwindow* g_Window;
