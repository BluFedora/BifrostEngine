#define NOMINMAX

#include <bf/bifrost.hpp>
#include <bf/editor/bifrost_editor_overlay.hpp>

#include <functional>
#include <iostream>
#include <utility>

using namespace bf;

struct TestClass
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
#if defined(_MSC_VER)
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

  ClassID::Type classID() const override { return ClassID::BASE_OBJECT; }

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

static void TestScriptingStuff()
{
  TestClass my_obj    = {74, "This message will be in Y"};
  VMParams  vm_params = VMParams{};
  vm_params.print_fn  = [](BifrostVM*, const char* message) {
    bfLogSetColor(BIFROST_LOGGER_COLOR_BLACK, BIFROST_LOGGER_COLOR_YELLOW, 0x0);
    std::printf("(BTS) %s", message);
    bfLogSetColor(BIFROST_LOGGER_COLOR_CYAN, BIFROST_LOGGER_COLOR_GREEN, BIFROST_LOGGER_COLOR_FG_BOLD);
  };

  VM vm = VM{vm_params};

  const BifrostVMClassBind camera_clz_bindings = vmMakeClassBinding<Camera_>(
   "Camera",
   vmMakeCtorBinding<Camera_>());

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

  vm.stackDestroyHandle(update_fn);
}

static constexpr TestCaseFn s_Test[] =
 {
  // &Test2DTransform,
  // &TestQuaternions,
  // &TestApplyReturningVoid,
  // &TestMetaSystem,
  &TestScriptingStuff,
};

enum class ReturnCode
{
  SUCCESS,
  FAILED_TO_INITIALIZE_PLATFORM,
  FAILED_TO_CREATE_MAIN_WINDOW,
  FAILED_TO_ALLOCATE_ENGINE_MEMORY,
};

#define MainQuit(code, label) \
  err_code = (code);          \
  goto label

extern "C" void xxx_TestLexer(void);

int main(int argc, char* argv[])
{
  xxx_TestLexer();

  for (const auto& test_fn : s_Test)
  {
    test_fn();
  }

  static_assert(std::numeric_limits<double>::is_iec559, "Use IEEE754, you weirdo.");

  ReturnCode err_code = ReturnCode::SUCCESS;

  if (bfPlatformInit({argc, argv, nullptr, nullptr}))
  {
    bfWindow* const main_window = bfPlatformCreateWindow("Mjolnir Editor 2021", 1280, 720, k_bfWindowFlagsDefault);

    if (main_window)
    {
      try
      {
        const std::size_t             engine_memory_size = bfMegabytes(300);
        const std::unique_ptr<char[]> engine_memory      = std::make_unique<char[]>(engine_memory_size);
        std::unique_ptr<Engine>       engine             = std::make_unique<Engine>(engine_memory.get(), engine_memory_size, argc, argv);
        const EngineCreateParams      params             = {{argv[0], 0}, 60};

        main_window->user_data = engine.get();
        main_window->event_fn  = [](bfWindow* window, bfEvent* event) { static_cast<Engine*>(window->user_data)->onEvent(window, *event); };
        main_window->frame_fn  = [](bfWindow* window) { static_cast<Engine*>(window->user_data)->tick(); };

        engine->init(params, main_window);
        // engine->stateMachine().push<MainDemoLayer>();
        engine->stateMachine().addOverlay<editor::EditorOverlay>(main_window);
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
