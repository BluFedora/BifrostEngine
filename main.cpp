// TODO(SR): Update glfw to version 3.3

#include "bifrost/bifrost_version.h"
#include <bifrost/bifrost.hpp>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream> /*  */
#include <utility>

// [https://www.glfw.org/docs/latest/group__native.html]
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#undef GLFW_EXPOSE_NATIVE_WIN32

class TestClass
{
 public:
  int         x;
  std::string y;

  TestClass(int f, std::string msg = "") :
    x(f),
    y{std::move(msg)}
  {
  }

  [[nodiscard]] const char* printf(float h) const
  {
    std::cout << "h = " << h << ", my var = " << x << "\n";
    return "__ Return from printf __";
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
    printf("%s", message);
  }
}
void testFN()
{
  std::cout << "does this work?" << std::endl;
}

static const char source[] = R"(
  import "main" for TestClass, cppFn;

  func hello()
  {
    print "Hello from another module.";
  }

  var t = new TestClass.ctor(29);
  print "ret from t = " +  t:printf(54);

  hello();

  class GameState
  {
    static var i = 2;
  };

  func update()
  {
    // print "I am updating!" + GameState.i;
    // GameState.i = GameState.i + 1;
  }

  func callMeFromCpp(arg0, arg1, arg2)
  {
    print "You passed in: " + arg0 + ", " + arg1 + ", "  + arg2;
    cppFn();
  }
)";

struct BifrostEngineCreateParams
{
  const char*   app_name;
  std::uint32_t app_version;
  std::uint32_t width;
  std::uint32_t height;
  void*         render_module;
  void*         render_window;
};

class BifrostEngine : private bifrost::bfNonCopyMoveable<BifrostEngine>
{
 private:
  std::pair<int, const char**> m_CmdlineArgs;
  bfGfxContextHandle           m_GfxBackend;

 public:
  BifrostEngine(int argc, const char* argv[]) :
    m_CmdlineArgs{argc, argv},
    m_GfxBackend{nullptr}
  {
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

    const bfGfxContextCreateParams gfx_create_params = {
     params.app_name,
     params.app_version,
     params.render_module,
     params.render_window,
    };

    m_GfxBackend = bfGfxContext_new(&gfx_create_params);
    // bfGfxContext_onResize(m_GfxBackend, params.width, params.height);

    bfLogPop();
  }

  [[nodiscard]] bool beginFrame() const
  {
    return false;  //bfGfxContext_beginFrame(m_GfxBackend);
  }

  void endFrame() const
  {
    // bfGfxContext_endFrame(m_GfxBackend);
  }

  void deinit()
  {
    // bfGfxDevice_flush(bfGfxContext_device(m_GfxBackend));
    // bfGfxContext_delete(m_GfxBackend);
    m_GfxBackend = nullptr;
    bfLogger_deinit();
  }
};

namespace ErrorCodes
{
  static constexpr int GLFW_FAILED_TO_INIT = -1;
  // static constexpr int GLAD_FAILED_TO_INIT = -2;
}  // namespace ErrorCodes

namespace bifrost::meta
{
    template<>
    const auto& Meta::registerMembers<TestClass>()
    {
      static auto member_ptrs = members(
        field("x", &TestClass::x),
        property("y", &TestClass::getY, &TestClass::setY),
        function("myRandomFn", &TestClass::myRandomFn),
        function("myRandomFn2", &TestClass::myRandomFn2));

      return member_ptrs;
    }
} // namespace bifrost::meta

int main(int argc, const char* argv[])
{
  static constexpr int INITIAL_WINDOW_SIZE[] = {1280, 720};

  std::printf("Bifrost Engine v%s\n", BIFROST_VERSION_STR);

  namespace bfmeta = bifrost::meta;
  using namespace bifrost;

  TestClass my_obj = {74, "This message will be in Y"};

  try
  {
    std::cout << "Meta Testing Bed: \n";

    for_each(meta::membersOf<TestClass>(), [&my_obj](const auto& member) {
      std::cout << member.name() << " : ";

      if constexpr (bfmeta::is_function_v<decltype(member)>)
      {
        member.call(my_obj, 6);
      }
      else
      {
        std::cout << member.get(my_obj) << std::endl;
      }
    });

    std::cout << "x = " << my_obj.x << "\n";

    std::cout << "Meta Testing End: \n\n";
  }
  catch (...)
  {
  }

  int error_code = 0;

  if (!glfwInit())
  {
    error_code = ErrorCodes::GLFW_FAILED_TO_INIT;
    return error_code;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  const auto main_window = glfwCreateWindow(INITIAL_WINDOW_SIZE[0], INITIAL_WINDOW_SIZE[1], "Bifrost Engine", nullptr, nullptr);
  glfwSetWindowSizeLimits(main_window, 400, 400, GLFW_DONT_CARE, GLFW_DONT_CARE);
  /*
  glfwMakeContextCurrent(main_window);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    error_code = ErrorCodes::GLAD_FAILED_TO_INIT;
    goto shutdown_glfw;  // NOLINT(hicpp-avoid-goto)
  }
  */
  namespace bf = bifrost;

  VMParams vm_params;
  vm_params.error_fn      = &userErrorFn;
  vm_params.print_fn      = [](BifrostVM*, const char* message) { std::cout << message << "\n"; };
  vm_params.min_heap_size = 50;
  vm_params.heap_size     = 100;
  VM vm{vm_params};

  const BifrostVMClassBind clz_bind = bf::vmMakeClassBinding<TestClass>(
   "TestClass",
   bifrost::vmMakeCtorBinding<TestClass, int>("ctor"),
   bfVM_makeMemberBinding("printf", &TestClass::printf));

  vm.stackResize(1);
  vm.moduleMake(0, "main");
  vm.moduleBind(0, clz_bind);
  vm.moduleBind<decltype(testFN), testFN>(0, "cppFn");

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

  {
    BifrostEngine engine{argc, argv};

    const BifrostEngineCreateParams params =
     {
      argv[0],
      0,
      INITIAL_WINDOW_SIZE[0],
      INITIAL_WINDOW_SIZE[1],
      GetModuleHandle(nullptr),
      glfwGetWin32Window(main_window),
     };

    engine.init(params);

    while (!glfwWindowShouldClose(main_window))
    {
      int window_width, window_height;
      glfwGetWindowSize(main_window, &window_width, &window_height);

      glfwPollEvents();

      if (engine.beginFrame())
      {
        if (update_fn)
        {
          vm.stackResize(1);
          vm.stackLoadHandle(0, update_fn);

          if (vm.stackGetType(0) == BIFROST_VM_FUNCTION)
          {
            vm.call(0);
          }
        }

        engine.endFrame();
      }

      glfwSwapBuffers(main_window);
    }

    engine.deinit();
  }

  vm.stackDestroyHandle(update_fn);

  // shutdown_glfw:
  glfwDestroyWindow(main_window);
  glfwTerminate();

  // shutdown_exit:
  return error_code;
}
