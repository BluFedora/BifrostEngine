#define NOMINMAX

#include <bifrost/bifrost.hpp>
#include <bifrost/bifrost_version.h>
#include <bifrost/editor/bifrost_editor_overlay.hpp>
#include <bifrost/memory/bifrost_memory_utils.h>  // bfMegabytes
#include <bifrost_imgui_glfw.hpp>

#include "main_demo.hpp"

#include <glfw/glfw3.h>  // TODO(SR): Remove but Needed for Global Window and glfwSetWindowSizeLimits

#include <chrono>
#include <functional>
#include <iostream>
#include <utility>

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

class Camera_
{
 private:
  float m_ElapsedTime = 0.0f;

 public:
  void update(float dt)
  {
    m_ElapsedTime += dt;
  }
};

#ifdef _WIN32
#include <intsafe.h> /* DWORD */

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
      __debugbreak();
      // DebugBreak();
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
    const auto              enum_str = meta_obj.type_info->enumToString(meta_obj.enum_value);

    std::printf("result1.value = %.*s\n", int(enum_str.length()), enum_str.begin());
  }

  std::printf("test.field = %i\n", test.field);

  test_class_info->findProperty("field")->set(makeVariant(&test), 3.0f);

  std::printf("test.field = %i\n", test.field);
}

static constexpr TestCaseFn s_Test[] = {&Test2DTransform, &TestQuaternions, &TestApplyReturningVoid, &TestMetaSystem};

namespace Error
{
  static constexpr int SUCCESS                          = 0;
  static constexpr int FAILED_TO_INITIALIZE_PLATFORM    = 1;
  static constexpr int FAILED_TO_CREATE_MAIN_WINDOW     = 2;
  static constexpr int FAILED_TO_ALLOCATE_ENGINE_MEMORY = 3;
}  // namespace Error

extern void toggleFs();

GLFWwindow* g_Window;  // TODO(SR): NEEDED BY MainDemoLayer for fullscreening code.

static constexpr float frame_rate       = 60.0f;
static constexpr float min_frame_rate   = 4.0f;
static constexpr float time_step_ms     = 1.0f / frame_rate;
static constexpr float max_time_step_ms = 1.0f / min_frame_rate;

float current_time;
float time_accumulator;

#define MainQuit(code, label) \
  err_code = (code);          \
  goto label

int main(int argc, char* argv[])
{
  static_assert(std::numeric_limits<double>::is_iec559, "Use IEEE754, you weirdo.");

  int err_code = Error::SUCCESS;

  for (const auto& test_fn : s_Test)
  {
    test_fn();
  }

  namespace bfmeta = meta;

  if (!bfPlatformInit({argc, argv, nullptr, BIFROST_PLATFORM_GFX_VUlKAN, nullptr}))
  {
    MainQuit(Error::FAILED_TO_INITIALIZE_PLATFORM, quit_main);
  }

      {
  BifrostWindow* const main_window = bfPlatformCreateWindow("Mjolnir Editor 2020", 1280, 720, BIFROST_WINDOW_FLAGS_DEFAULT);

  if (!main_window)
  {
    MainQuit(Error::FAILED_TO_CREATE_MAIN_WINDOW, quit_platform);
  }

  bfLogSetColor(BIFROST_LOGGER_COLOR_CYAN, BIFROST_LOGGER_COLOR_GREEN, BIFROST_LOGGER_COLOR_FG_BOLD);
  std::printf("\n\n                 Bifrost Engine v%s\n\n\n", BIFROST_VERSION_STR);
  bfLogSetColor(BIFROST_LOGGER_COLOR_BLUE, BIFROST_LOGGER_COLOR_WHITE, 0x0);

  {
    TestClass my_obj = {74, "This message will be in Y"};

    try
    {
      const std::size_t             engine_memory_size = bfMegabytes(50);
      const std::unique_ptr<char[]> engine_memory      = std::make_unique<char[]>(engine_memory_size);

      Engine engine{engine_memory.get(), engine_memory_size, argc, argv};

      g_Window = (GLFWwindow*)main_window->handle;

      glfwSetWindowSizeLimits(g_Window, 300, 70, GLFW_DONT_CARE, GLFW_DONT_CARE);

      const BifrostEngineCreateParams params = {argv[0], 0};

      engine.init(params, main_window);

      imgui::startup(engine.renderer().context(), main_window);

      engine.stateMachine().push<MainDemoLayer>();
      engine.stateMachine().addOverlay<editor::EditorOverlay>();

      VM& vm = engine.scripting();

      const BifrostVMClassBind camera_clz_bindings = vmMakeClassBinding<Camera_>("Camera", vmMakeCtorBinding<Camera_>());

      vm.stackResize(5);

      vm.moduleLoad(0, BIFROST_VM_STD_MODULE_ALL);
      vm.moduleMake(0, "bifrost");
      vm.stackStore(0, camera_clz_bindings);
      vm.stackLoadVariable(1, 0, "Camera");
      vm.stackStore(1, "update", &Camera_::update);

      const BifrostVMClassBind clz_bind = vmMakeClassBinding<TestClass>(
       "TestClass",
       vmMakeCtorBinding<TestClass, int>(),
       vmMakeMemberBinding<&TestClass::printf>("printf"));

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

      current_time     = float(glfwGetTime());
      time_accumulator = 0.0f;

      main_window->user_data = &engine;

      main_window->event_fn = [](BifrostWindow* window, bfEvent* event) {
        Engine* const engine = (Engine*)window->user_data;

        if (event->isType(BIFROST_EVT_ON_WINDOW_RESIZE))
        {
          bfGfxWindow_markResized(engine->renderer().context(), (bfWindowSurfaceHandle)window->renderer_data);
        }

        imgui::onEvent(window, *event);
        engine->onEvent(*event);
      };

      main_window->frame_fn = [](BifrostWindow* window) {
        Engine* const engine = (Engine*)window->user_data;

        const auto CurrentTimeSeconds = []() -> float {
          return float(glfwGetTime());
        };

        const float new_time   = CurrentTimeSeconds();
        const float delta_time = std::min(new_time - current_time, max_time_step_ms);

        time_accumulator += delta_time;
        current_time = new_time;

        int window_width;
        int window_height;
        bfWindow_getSize(window, &window_width, &window_height);

        if (engine->beginFrame())
        {
          StandardRenderer& renderer = engine->renderer();

#if 0
        if (update_fn)
        {
          vm.stackResize(1);
          vm.stackLoadHandle(0, update_fn);

          if (vm.stackGetType(0) == BIFROST_VM_FUNCTION)
          {
            vm.call(0, delta_time);
          }
        }
#endif

          imgui::beginFrame(
           renderer.surface(),
           float(window_width),
           float(window_height),
           delta_time);

          while (time_accumulator >= time_step_ms)
          {
            engine->fixedUpdate(time_step_ms);
            time_accumulator -= time_step_ms;
          }

          const float render_alpha = time_accumulator / time_step_ms;  // current_state * render_alpha + previous_state * (1.0f - render_alpha)

          engine->update(delta_time);

          engine->drawBegin(render_alpha);
          imgui::endFrame();
          engine->drawEnd();
          engine->endFrame();
        }
      };

      while (!bfWindow_wantsToClose(main_window))
      {
        bfPlatformPumpEvents();
        main_window->frame_fn(main_window);
      }

      vm.stackDestroyHandle(update_fn);
      imgui::shutdown();
      engine.deinit();
    }
    catch (std::bad_alloc&)
    {
      MainQuit(Error::FAILED_TO_ALLOCATE_ENGINE_MEMORY, quit_window);
    }
  }

quit_window:
  bfPlatformDestroyWindow(main_window);
      }
quit_platform:
  bfPlatformQuit();

quit_main:
  return err_code;
}

#undef MainQuit
