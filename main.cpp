#include "bifrost/bifrost_version.h"

#include "bifrost/asset_io/bifrost_asset_handle.hpp"
#include "bifrost/core/bifrost_game_state_machine.hpp"
#include "bifrost/data_structures/bifrost_any.hpp"
#include "bifrost/data_structures/bifrost_string.hpp"
#include "bifrost/editor/bifrost_editor_overlay.hpp"
#include "bifrost/memory/bifrost_freelist_allocator.hpp"
#include "bifrost/platform/bifrost_window_glfw.hpp"
#include "demo/game_state_layers/main_demo.hpp"

#include <bifrost/bifrost.hpp>
#include <bifrost_editor/bifrost_imgui_glfw.hpp>

#include "imgui/imgui.h"

#include <chrono>
#include <functional>
#include <future>
#include <glfw/glfw3.h>
#include <iostream>
#include <utility>

#define GLFW_EXPOSE_NATIVE_WIN32
#include "bifrost/core/bifrost_engine.hpp"
#include <glfw/glfw3native.h>
#undef GLFW_EXPOSE_NATIVE_WIN32

namespace bifrost
{
  class SpriteComponent : public Component<SpriteComponent>
  {
   public:
    SpriteComponent(Entity& owner) :
      Component<SpriteComponent>(owner)
    {
      std::printf("Created a SpriteComponent\n");
    }

    void spriteMethod()
    {
      std::printf("Calling spriteMethod\n");
    }
  };

  class MeshComponent : public Component<MeshComponent>
  {
   public:
    MeshComponent(Entity& owner) :
      Component<MeshComponent>(owner)
    {
      std::printf("Created a MeshComponent\n");
    }
  };

  namespace meta
  {
    template<>
    const auto& Meta::registerMembers<BifrostTransform>()
    {
      static auto member_ptrs = members(
       class_info<BifrostTransform>("Transform"),
       ctor<>(),
       field("origin", &BifrostTransform::origin),
       field("local_position", &BifrostTransform::local_position),
       field("local_rotation", &BifrostTransform::local_rotation),
       field("local_scale", &BifrostTransform::local_scale),
       field("world_position", &BifrostTransform::world_position),
       field("world_rotation", &BifrostTransform::world_rotation),
       field("world_scale", &BifrostTransform::world_scale),
       field("local_transform", &BifrostTransform::local_transform),
       field("world_transform", &BifrostTransform::world_transform));

      return member_ptrs;
    }

    template<>
    const auto& Meta::registerMembers<SpriteComponent>()
    {
      static auto member_ptrs = members(
       class_info<SpriteComponent, Component<SpriteComponent>>("SpriteComponent"));

      return member_ptrs;
    }

    template<>
    const auto& Meta::registerMembers<MeshComponent>()
    {
      static auto member_ptrs = members(
       class_info<MeshComponent, Component<MeshComponent>>("MeshComponent"));

      return member_ptrs;
    }
  }  // namespace meta
}  // namespace bifrost

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

static char source[4096] = R"(
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

  func update()
  {
    cam:update(0.4);

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

using namespace bifrost;

namespace ErrorCodes
{
  static constexpr int GLFW_FAILED_TO_INIT = -1;
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
}  // namespace bifrost::meta

namespace bifrost
{
  class Camera
  {
   private:
    float m_ElapsedTime = 0.0f;

   public:
    void update(float dt)
    {
      std::printf("Camera::update(@ %f) with %f as dt.\n", m_ElapsedTime, dt);

      m_ElapsedTime += dt;
    }
  };
}  // namespace bifrost

#ifdef _WIN32
extern "C" {
__declspec(dllexport) DWORD NvOptimusEnablement                = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif  //  _WIN32

static GLFWmonitor* get_current_monitor(GLFWwindow* window)
{
  int                nmonitors, i;
  int                wx, wy, ww, wh;
  int                mx, my, mw, mh;
  int                overlap, bestoverlap;
  GLFWmonitor*       bestmonitor;
  const GLFWvidmode* mode;

  bestoverlap = 0;
  bestmonitor = nullptr;

  glfwGetWindowPos(window, &wx, &wy);
  glfwGetWindowSize(window, &ww, &wh);
  GLFWmonitor** monitors = glfwGetMonitors(&nmonitors);

  for (i = 0; i < nmonitors; i++)
  {
    mode = glfwGetVideoMode(monitors[i]);
    glfwGetMonitorPos(monitors[i], &mx, &my);
    mw = mode->width;
    mh = mode->height;

    overlap = max(0, min(wx + ww, mx + mw) - max(wx, mx)) * max(0, min(wy + wh, my + mh) - max(wy, my));

    if (bestoverlap < overlap)
    {
      bestoverlap = overlap;
      bestmonitor = monitors[i];
    }
  }

  return bestmonitor;
}

static constexpr int INITIAL_WINDOW_SIZE[] = {1280, 720};

int main(int argc, const char* argv[])  // NOLINT(bugprone-exception-escape)
{
  namespace bfmeta = meta;
  namespace bf     = bifrost;

  bfLogSetColor(BIFROST_LOGGER_COLOR_CYAN, BIFROST_LOGGER_COLOR_GREEN, BIFROST_LOGGER_COLOR_FG_BOLD);
  std::printf("\n\n                 Bifrost Engine v%s\n\n\n", BIFROST_VERSION_STR);
  bfLogSetColor(BIFROST_LOGGER_COLOR_BLUE, BIFROST_LOGGER_COLOR_WHITE, 0x0);

  TestClass my_obj = {74, "This message will be in Y"};

  try
  {
    std::cout << "Meta Testing Bed: \n";

    for_each(meta::membersOf<TestClass>(), [&my_obj](const auto& member) {
      std::cout << member.name() << " : ";

      if constexpr (bfmeta::is_function_v<decltype(member)>)
      {
        member.call(&my_obj, 6);
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

  {
    WindowGLFW    window{};
    BifrostEngine engine{argc, argv};

    if (!window.open("Bifrost Engine"))
    {
      return -1;
    }

    g_Engine = &engine;
    g_Window = window.handle();

    glfwSetWindowSizeLimits(g_Window, 300, 70, GLFW_DONT_CARE, GLFW_DONT_CARE);

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

    editor::imgui::startup(engine.renderer().context(), window);

    engine.stateMachine().push<MainDemoLayer>();
    engine.stateMachine().addOverlay<editor::EditorOverlay>();

    std::cout << "\n\nScripting Language Test Begin\n";

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

    std::cout << "\n\nScripting Language Test End\n";

    while (!window.wantsToClose())
    {
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
        }

        editor::imgui::onEvent(evt);
        engine.onEvent(evt);
      }

      if (engine.beginFrame())
      {
        editor::imgui::beginFrame(
         engine.renderer().surface(),
         float(window_width),
         float(window_height),
         float(glfwGetTime()));
        engine.update();

        if (update_fn)
        {
          // vm.stackResize(1);
          // vm.stackLoadHandle(0, update_fn);

          if (vm.stackGetType(0) == BIFROST_VM_FUNCTION)
          {
            // vm.call(0);
          }
        }

        editor::imgui::endFrame(engine.renderer().mainCommandList());
        engine.endFrame();
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(15 * 1));
    }

    vm.stackDestroyHandle(update_fn);
    editor::imgui::shutdown();
    engine.deinit();
    window.close();
  }

  shutdownGLFW();

  return 0;
}

void ImGUIOverlay::onUpdate(BifrostEngine& engine, float delta_time)
{
  ImGuiIO& io = ImGui::GetIO();

  const auto height = io.DisplaySize.y - 10.0f;

  ImGui::SetNextWindowPos(ImVec2(5.0f, 5.0f));
  ImGui::SetNextWindowSize(ImVec2(350.0f, height), ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f, height), ImVec2(600.0f, height));
  if (ImGui::Begin("Hierarchy", nullptr, ImGuiWindowFlags_NoMove))
  {
    auto* const scene = engine.currentScene();

    if (scene)
    {
      if (ImGui::Selectable("Create"))
      {
        static int        id   = 0;
        const std::string name = "New Entity" + std::to_string(id++);
        scene->addEntity(name);
      }

      const auto inspectEntityArray = [this](const EntityList& entities, auto& rec) -> void {
        Entity* add_child_to = nullptr;

        for (auto& entity : entities)
        {
          ImGui::PushID(&entity);
          const bool open = ImGui::TreeNode(entity.name().c_str());
          if (ImGui::BeginPopupContextItem())
          {
            if (ImGui::Selectable("Add Child"))
            {
              add_child_to = &entity;
            }

            ImGui::EndPopup();
          }
          if (open)
          {
            if (ImGui::Button("Select"))
            {
              m_SelectedEntity = &entity;
            }

            if (ImGui::Button("Add Sprite Component"))
            {
              entity.add<SpriteComponent>();
            }

            rec(entity.children(), rec);
            ImGui::TreePop();
          }
          ImGui::PopID();
          // inspect(entity->name().c_str(), entity);
        }

        if (add_child_to)
          for (int i = 0; i < 10; ++i)
            add_child_to->addChild("My Child");
      };

      // inspectEntityArray(engine.currentScene()->rootEntities(), inspectEntityArray);
    }
    else
    {
      ImGui::Text("NO SCENE OPEN");
    }
    // inspect("Scene", engine.currentScene());
  }
  ImGui::End();

  ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 5.0f, 5.0f), 0, ImVec2(1.0f, 0.0f));
  ImGui::SetNextWindowSize(ImVec2(350.0f, height), ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints(ImVec2(250.0f, height), ImVec2(600.0f, height));
  window("RTTI", [this]() {
    if (m_SelectedEntity)
    {
      Any v = &m_SelectedEntity->transform();
      inspect("Transform", v, meta::TypeInfo<BifrostTransform>::get());

      m_SelectedEntity->forEachComp([this](BaseObjectT* comp) {
        auto* t = comp->type();

        inspect(t->name().data(), comp);
      });
    }

    auto& memory = bifrost::meta::gRttiMemoryBacking();
    ImGui::Text("Memory Usage %i / %i", int(memory.usedMemory()), int(memory.size()));

    for (auto& type : bifrost::meta::gRegistry())
    {
      const auto& name = type.key();

      if (ImGui::TreeNode(type.value(), "%.*s", name.length(), name.data()))
      {
        auto* const type_info = type.value();

        ImGui::Text("Size: %i, Alignment %i", int(type_info->size()), int(type_info->alignment()));

        for (auto& field : type_info->members())
        {
          ImGui::Text("member: %.*s", field->name().length(), field->name().data());
        }

        for (auto& prop : type_info->properties())
        {
          ImGui::Text("prop: %.*s", prop->name().length(), prop->name().data());
        }

        for (auto& method : type_info->methods())
        {
          ImGui::Text("method: %.*s", method->name().length(), method->name().data());
        }

        ImGui::Separator();
        ImGui::TreePop();
      }
    }
  });
#if 0
  window("Scripting", [&engine]() {
    static std::future<BifrostVMError> s_WaitForCompile;

    ImGui::PushItemWidth(-1.0f);
    ImGui::InputTextMultiline("", source, sizeof(source), ImVec2(), ImGuiInputTextFlags_AllowTabInput);

    if (!s_WaitForCompile.valid() || s_WaitForCompile.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
    {
      if (s_WaitForCompile._Is_ready())
      {
        s_WaitForCompile.get();
      }

      if (ImGui::Button("Run"))
      {
        auto& vm = engine.scripting();

        // s_WaitForCompile = std::async(&bifrost::VM::execInModule, &vm, nullptr, source, std::strlen(source));

        vm.execInModule(nullptr, source, std::strlen(source));
      }
    }

    ImGui::PopItemWidth();
  });

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
}

void ImGUIOverlay::inspect(const char* label, bifrost::BaseObjectT* object) const
{
  Any a = object;
  inspect(label, a, object->type());
}

template<typename T>
static bool InspectInt(const char* label, bifrost::Any& object, bool* did_change = nullptr)
{
  if (object.is<T>())
  {
    int value = int(object.as<T>());

    if (ImGui::DragInt(label, &value))
    {
      if constexpr (std::is_unsigned_v<T>)
      {
        value = value < 0 ? 0 : value;
      }

      object.assign<T>(value);

      if (did_change)
      {
        *did_change = true;
      }
    }
    else
    {
      if (did_change)
      {
        *did_change = false;
      }
    }

    return true;
  }

  return false;
}

template<typename T>
static bool InspectFloat(const char* label, bifrost::Any& object, bool* did_change = nullptr)
{
  if (object.is<T>())
  {
    float value = float(object.as<T>());

    if (ImGui::DragFloat(label, &value))
    {
      object.assign<T>(value);

      if (did_change)
      {
        *did_change = true;
      }
    }
    else
    {
      if (did_change)
      {
        *did_change = false;
      }
    }

    return true;
  }

  if (object.is<T*>())
  {
    float value = float(*object.as<T*>());

    if (ImGui::DragFloat(label, &value))
    {
      *object.as<T*>() = T(value);

      if (did_change)
      {
        *did_change = true;
      }
    }
    else
    {
      if (did_change)
      {
        *did_change = false;
      }
    }

    return true;
  }

  return false;
}

static int inspectStringCallback(ImGuiInputTextCallbackData* data)
{
  if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
  {
    auto* str = static_cast<std::string*>(data->UserData);
    IM_ASSERT(data->Buf == str->c_str());
    str->resize(data->BufTextLen);
    data->Buf = const_cast<char*>(str->c_str());
  }

  return 0;
}

bool inspect(const char* label, std::string& string, ImGuiInputTextFlags flags = ImGuiInputTextFlags_None)
{
  flags |= ImGuiInputTextFlags_CallbackResize;
  return ImGui::InputText(label, const_cast<char*>(string.c_str()), string.capacity() + 1, flags, inspectStringCallback, static_cast<void*>(&string));
}

bool ImGUIOverlay::inspect(const char* label, Any& object, meta::BaseClassMetaInfo* info) const
{
  bool did_change = false;

  if (object.isEmpty())
  {
    ImGui::Text("%s : <empty>", label);
  }
  else if (InspectInt<char>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::int8_t>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::uint8_t>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::int16_t>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::uint16_t>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::int32_t>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::uint32_t>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::int64_t>(label, object, &did_change))
  {
  }
  else if (InspectInt<std::uint64_t>(label, object, &did_change))
  {
  }
  else if (InspectFloat<float>(label, object, &did_change))
  {
  }
  else if (InspectFloat<double>(label, object, &did_change))
  {
  }
  else if (InspectFloat<long double>(label, object, &did_change))
  {
  }
  else if (object.is<Vec3f>())
  {
    did_change = ImGui::DragFloat3(label, &object.as<Vec3f>().x);
  }
  else if (object.is<Vec3f*>())
  {
    did_change = ImGui::DragFloat3(label, &object.as<Vec3f*>()->x);
  }
  else if (object.is<std::string>())
  {
    did_change = ::inspect(label, object.as<std::string>());
  }
  else if (object.is<std::string*>())
  {
    did_change = ::inspect(label, *object.as<std::string*>());
  }
  else if (object.is<void*>())
  {
    ImGui::Text("%s : <%p>", label, object.as<void*>());
  }
  else if (object.is<bool>())
  {
    ImGui::Checkbox(label, &object.as<bool>());
  }
  else if (object.is<std::byte>())
  {
    ImGui::Text("%s : <byte>", label);
  }
  else if (info)
  {
    if (object.as<void*>() != nullptr)
    {
      if (ImGui::TreeNode(label))
      {
        for (auto& field : info->members())
        {
          auto field_value = field->get(object);

          inspect(field->name().data(), field_value, field->type());
        }

        for (auto& prop : info->properties())
        {
          auto field_value = prop->get(object);

          if (inspect(prop->name().data(), field_value, prop->type()))
          {
            prop->set(object, field_value);
            did_change = true;
          }
        }

        if (info->isArray())
        {
          const std::size_t size = info->arraySize(object);

          for (std::size_t i = 0; i < size; ++i)
          {
            auto* const idx_label = bifrost::string_utils::alloc_fmt(g_Engine->mainMemory(), nullptr, "%i", int(i));
            auto        element   = info->arrayGetElementAt(object, i);
            if (inspect(idx_label, element, info->containedType()))
            {
              info->arraySetElementAt(object, i, element);
            }
            g_Engine->mainMemory().dealloc(idx_label);
          }
        }

        ImGui::TreePop();
      }
    }
    else
    {
      if (ImGui::Button("Create"))
      {
        object     = info->instantiate(g_Engine->mainMemory());
        did_change = true;
      }
    }
  }
  else
  {
    ImGui::Text("%s : <unknown>", label);
  }

  return did_change;
}

static void cb()
{
  std::printf("I dont return anything\n");
}

void MainDemoLayer::onLoad(BifrostEngine& engine)
{
  FunctionView<void(void)> test_view;

  test_view.bind(cb);

  test_view();
  test_view.safeCall();

  engine.stateMachine().addOverlay<ImGUIOverlay>("ImGui 0");
#if 0
  auto mye              = new bifrost::Entity("Hello I am an entity");
  auto entity_type_info = bifrost::meta::TypeInfoFromName("Entity");
  auto dynamic_entity   = entity_type_info->instantiate(engine.mainMemory());
  auto move_method      = entity_type_info->findMethod("move");

  int                write;
  const bifrost::Any result = move_method->invoke(dynamic_entity, 89, 6.3f, write);

  if (result.is<bifrost::meta::InvalidMethodCall>())
  {
    bfLogPrint("Failed to call move on entity");
  }

  bifrost::Any any1 = 9;
  bifrost::Any any0 = std::move(any1);
  bifrost::Any any2 = mye;
  bifrost::Any any3 = TestClass(42, "HOLD ME TIGHT ANY");

  delete any2.as<bifrost::Entity*>();

  if (any0.is<int>())
  {
    bfLogPrint("My Any contains an int with value = %i\n", any0.as<int>());
  }

  auto sprite_info = bifrost::meta::TypeInfoFromName("SpriteComponent");

  if (sprite_info)
  {
    bifrost::SpriteComponent* sprite = sprite_info->instantiate(engine.mainMemory());

    sprite->spriteMethod();

    engine.mainMemory().dealloc_t(sprite);
  }
#endif
}

#include "src/bifrost/asset_io/test.cpp"
