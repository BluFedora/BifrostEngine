
#define NOMINMAX

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
      // std::printf("Camera::update(@ %f) with %f as dt.\n", m_ElapsedTime, dt);
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

int main(int argc, const char* argv[])  // NOLINT(bugprone-exception-escape)
{
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
    BifrostEngine engine{main_memory, main_memory_size, argc, argv};

    if (!window.open("Mjolnir Editor"))
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
       GetModuleHandle(nullptr),
       glfwGetWin32Window(window.handle()),
      },
      INITIAL_WINDOW_SIZE[0],
      INITIAL_WINDOW_SIZE[1],
     };

    engine.init(params);

    imgui::startup(engine.renderer().context(), window);

    engine.stateMachine().push<MainDemoLayer>();
    engine.stateMachine().addOverlay<editor::EditorOverlay>();

    VM& vm = engine.scripting();

    const BifrostVMClassBind camera_clz_bindings = bf::vmMakeClassBinding<bf::Camera>("Camera", bf::vmMakeCtorBinding<bf::Camera>());

    vm.stackResize(5);

    vm.moduleLoad(0, BIFROST_VM_STD_MODULE_ALL);
    vm.moduleMake(0, "bifrost");
    vm.stackStore(0, camera_clz_bindings);
    vm.stackLoadVariable(1, 0, "Camera");
    vm.stackStore(1, "update", &bf::Camera::update);

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
         engine.renderer().surface(),
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
        imgui::endFrame(engine.renderer().mainCommandList());
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
