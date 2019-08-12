#include "bifrost/meta/bifrost_meta_member.hpp"
#include <bifrost/bifrost_vm.hpp>
#include <bifrost/graphics/bifrost_gfx_api.h>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream> /*  */

//MSVC: You can use the /permissive- compiler option to specify standards-conforming compiler behavior. This option disables permissive behaviors, and sets the /Zc compiler options for strict conformance.

// TODO(SR):
//   This is a feature list that will be tackled
//   on a need-be basis for development of an indie engine / game.
//   [4273 lines of vm + ds]
//   * Error handling in scripts.
//   * Error throwing in native functions.
//   * Do while loop.
//   * Standard Lib
//   * Class inheritance.
//   * Unary Not
//   * Unary Minus
//   * +=, -=, *=, /=
//   * Modulo
//   * ++x
//   * x++
//   * Switch Statements (Maybe add some pattern matching?)
//   * Preprocessor for some extra fun.
//   * References to C/++ owned Objects (references / lightuserdata)
//   * References to C/++ owned Objects + Class information? (userdata)
//   [X] * User defined callable objects. This would make "Closure" better. define a 'call' function?
//   * Module variables apparently. (Statics also solve the problem)
//   * Integer Div
//   * Bin / Oct / Hex Numbers
//   * Unicode chars?
//   * foreach (with user defined iterators)
//   * Ternary branch
//   * For more efficient execution all string functions must be a lib rather than on the object itself.

// TODO(SR): Bifrost DS
//   * Array needs a 'shrink to fit' function to make it use less memory.

// Tide Engine Notes:
//   Singletons / Globals:
//     -> Debug Logging
//     -> GUI system
//     -> Renderer Backend
//     -> Engine (lmao)
//        * By extension a lot of the engine systems but if an engine was not global this would help?
//     -> Imgui Backend
//     -> Debug Drawer
//     -> Input System
//     -> Camera FBO
//     -> A lot of Game-play garbage
//
//  Bad Ideas That happened because of Time:
//    * The Engine has GLFW in it.
//    * The Engine relies on the editor.
/*
Critique Per Subsystem:
  -> AI:
    * Great except it used new / delete.
    * Also was put directly into Behaviors so added cost to literally every object.
    * Maybe the actions should be std::functions??Rather than inheritance.
  -> ANIMATION (Frame/SpriteSheet):
    * Seemed fine for the most part. (SpriteSheet was good)
    * Make more data private in the component (People poked around)
    * There are 3 booleans in the component. Bitfield time.
    * Maybe rather than using a 'HashTable' an array with a linear find is better? (O(N) but N is very low?)
      -> This can be a be a compiler define based off of game type.
  -> ANIMATION (Ease):
    * Badly designed API.
    * Needed a more automatic API.
    * Needed a better way to reset / play backwards.
    * Needed a better pause.
  -> ANIMATION (Parallax)
    * Teriible to interface with.
    * Very confusing variable names and editing.
    * Bounds idea was very odd. The Math used was pretty awkward.
    * I set up the bg then it got copied and pasted that means it was too hard to use.
  -> ANIMATION (Timeline)
    * One of the latest system written over the summer.
    * Overall pretty nice and modular. + Lifetime management is interesting.
    * Needs less jank serialization of tracks.
    * Needs to decide on how to manage complex animations in editor.
    * How can we use multiple objects?
    * How would the object IDs work?
    * Uses the default heap with std::make_shared<Timeline>.
    * Boolean support is very bad.
    * Bitflags for the Booleans.
    * Memory Layout is bad and the classes seem heavy.
  -> ASSET_IO (Area)
    * Fine, the way the engine used them was bad though.
    * Lifetime management was bad.
    * Bounds editor could have been better.
    * Terrible for a multi-doc setup. (Practically only 3-5 Areas max used for Project Gemini)
    * Should the Camera be per area? (Yes for multi-doc setup?)
      -> Need to decide on sematics for the 'editor' Camera.
    * Useless Path Member. Maybe to use it effectively the editor should wrap Area in a doc?
    * Assets somewhat own areas but it was kinda cumbersome.
  -> ASSET_IO (Assets + AssetHandle)
    * Works out well enough. Ref counting is a very easy paradigm.
    * The main issue comes with the inability to stream content
      since each component / entity / behavior keeps the asset alive thus loaded in memory.
  -> ASSET_IO (File / FileSystem)
    * File was fine.
    * FileSystem should have an engine abstraction.
  -> ASSET_IO (Json)
    * Good format very flexible and most bugs have been fixed.
    * Pretty Inconsistent API / Naming.
    * Need more use of a config file.
  -> ASSET_IO (Prefab)
    * V2 was better but still very raw.
    * Need to be able to override properties.
    * Editing a Prefab should be more live.
  -> ASSET_IO (Serialization)
    * Good except there should have been an abstract interface for (de/)serialization
      as a lot of the code for each path is pretty much the same.
  -> AUDIO
    * Needs to be redone completely.
    * Positional audio would have been nice.
    * Good that the component lets the behaviors declare what they want.
    * No way to transition easily.
  -> COLLISION (General + Raycast)
    * It's good, need to review the polygon code for more bugs.
    * Raycast could have a more consistent API.
  -> COMBAT
    * Good, the healthbar rendering could have been better...
  -> COMPUTE
    * TBD
  -> CORE (Engine + Gamestate)
    * Gamestate system was much to fickle and awkward.
    * The Engine maybe did too much?
    * Gamestate's need the engine in the event handle.
  -> DATA_STRUCTURES (All)
    * Very good and nice to use.
  -> DEBUG (Drawer)
    * Replace the global state with a system on the engine.
    * This was overall small, minimal and good.
  -> DEBUG (Logger)
    * Pretty good, maybe have the logger not go to screen by default.
    * The editor should have had a console.
    * Replace the global state with a system on the engine.
  -> DEBUG (Profiler)
    * NOT THREAD SAFE.
    * Better editor visualization?
  -> DEBUG (ImGui)
    * A memory leaking mess.
    * Maybe it should be an engine plugin?
    * Bad big state save and restore.
  -> ECS
    * Each system should have been the each specific folder
  -> EVENT (Event)
    * Otherwise pretty good. API for Event::data could be nicer.
    * DelegateList was a poor abstarction with pointer invalidation.
  -> EVENT (Input)
    * Replace the global state with a system on the engine.
    * Multibind would have been nice.
    * Config file based binding would be nice aswell.
  -> GRAPHICS (All?)
    * Paricles       - Try out scripted particles.
    * Lighting       -  Would be done differnetly.
    * Color          - Color8u + Color4f
    * RenderMaterial - A fairly weak abstraction could be better but how?
    * TextComponent  - Heavier Caching + Semi-awkward editing of box.
    * Texture        - Turned into a cluster f**k w/ Gifs.
    * Transform      - Could be more data oriented  + quaternions are better.
    *                - Bad global Node storage.
    * Vertex         - There should be more types of verts.
    * VertexArray    - Bad abstacrion. handled too much.
    * VertexBuffer   - Decent abstarction.
  -> GUI (Font)
    * Static strings / more state cahcing needed.
  -> GUI (UI Layout)
    * Fine could be more advance for a full editor.
    * More layout engine's (Flutter Style)
  -> MATH (All)
    * Camera issues are from other systems.
    * Leaks memory with the FBO.
    * Better 3D culling.
    * More native postprocessing.
  -> MEMORY (All)
    * Great, just needs a more cosistent API cuz it's super easy to use wrong.
  -> PHYSICS (RigidBody)
    * Incosistent API between namespace and object.
  -> PHYSICS (Collider)
    * Weird Setup API.
  -> RTTI (All)
    * Add support for functions.
  -> SCRIPTING (Lua)
    * Error handling needed to be better. Also compile times.
    * Hope Bifrost Script can be the only scripting solution we need.
  -> THREADING (All)
    * Good lib.
  -> TIDE (Config / Init)
    * This stuff should maybe be loaded from a config file.
    * Window Initilization should be out of the engine.
*/

/* GOOD:
 *  Engine Run very liked feature by designers.
 *
 */

class TestClass
{
 public:
  int var;

  TestClass(int f) :
    var(f) {}

  [[nodiscard]] const char*
  printf(float h) const
  {
    std::cout << "h = " << h << ", my var = " << var << "\n";
    return "__ Return from printf __";
  }

  ~TestClass() noexcept
  {
    try
    {
      std::cout << "~TestClass(" << var << ")\n";
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
  //print "I am updating!" + GameState.i;
  //GameState.i = GameState.i + 1;
}

func callMeFromCpp(arg0, arg1, arg2)
{
  print "You passed in: " + arg0 + ", " + arg1 + ", "  + arg2;
  cppFn();
}
)";

template<typename T>
class bfNonCopyable  // NOLINT(hicpp-special-member-functions)
{
 public:
  bfNonCopyable(const bfNonCopyable&) = delete;
  bfNonCopyable& operator=(const T&) = delete;

 protected:
  bfNonCopyable()  = default;
  ~bfNonCopyable() = default;
};

template<typename T>
class bfNonMoveable  // NOLINT(hicpp-special-member-functions)
{
 public:
  bfNonMoveable(const bfNonMoveable&) = delete;
  bfNonMoveable& operator=(const T&) = delete;

 protected:
  bfNonMoveable()  = default;
  ~bfNonMoveable() = default;
};

struct BifrostEngineCreateParams
{
  const char*   app_name;
  std::uint32_t app_version;
  std::uint32_t width;
  std::uint32_t height;
};

// clang-format off
class BifrostEngine : private bfNonCopyable<BifrostEngine>, private bfNonMoveable<BifrostEngine>
// clang-format on
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
    const bfGfxContextCreateParams gfx_create_params = {
     params.app_name,
     params.app_version,
    };

    m_GfxBackend = bfGfxContext_new(&gfx_create_params);
    bfGfxContext_onResize(m_GfxBackend, params.width, params.height);
  }

  [[nodiscard]] bool beginFrame() const
  {
    return bfGfxContext_beginFrame(m_GfxBackend);
  }

  void endFrame() const
  {
    bfGfxContext_endFrame(m_GfxBackend);
  }

  void deinit()
  {
    bfGfxDevice_flush(bfGfxContext_device(m_GfxBackend));
    bfGfxContext_delete(m_GfxBackend);
    m_GfxBackend = nullptr;
  }
};

namespace ErrorCodes
{
  static constexpr int GLFW_FAILED_TO_INIT = -1;
  static constexpr int GLAD_FAILED_TO_INIT = -2;
}  // namespace ErrorCodes

class MetaTest
{
 public:
  float x;

 private:
  std::string y;

 public:
  MetaTest(float v, std::string msg) :
    x{v},
    y{std::move(msg)}
  {
  }

  void myRandomFn(int hello) const
  {
    std::cout << "(" << x << ") Hello is " << hello << "\n";
  }

  void myRandomFn2(int hello)
  {
    std::cout << "Changing x to: " << hello << "\n";
    x = float(hello);
  }

  const std::string& getY() const
  {
    return y;
  }

  void setY(const std::string& val)
  {
    y = val;
  }
};

namespace bifrost::meta
{
  template<>
  const auto& Meta::registerMembers<MetaTest>()
  {
    static auto member_ptrs = members(
     field("x", &MetaTest::x),
     property("y", &MetaTest::getY, &MetaTest::setY),
     function("myRandomFn", &MetaTest::myRandomFn),
     function("myRandomFn2", &MetaTest::myRandomFn2));

    return member_ptrs;
  }
}  // namespace bifrost::meta

int main(int argc, const char* argv[])
{
  namespace bfmeta = bifrost::meta;
  using namespace bifrost;

  MetaTest my_obj = {74.0f, "This message will be in Y"};

  try
  {
    std::cout << "Meta Testing Bed: \n";

    for_each(meta::membersOf<MetaTest>(), [&my_obj](const auto& member) {
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

  static constexpr int INITIAL_WINDOW_SIZE[] = {1280, 720};

  int error_code = 0;

  if (!glfwInit())
  {
    error_code = ErrorCodes::GLFW_FAILED_TO_INIT;
    goto shutdown_exit;  // NOLINT(hicpp-avoid-goto)
  }

  const auto main_window = glfwCreateWindow(INITIAL_WINDOW_SIZE[0], INITIAL_WINDOW_SIZE[1], "Bifrost Engine", nullptr, nullptr);

  glfwMakeContextCurrent(main_window);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    error_code = ErrorCodes::GLAD_FAILED_TO_INIT;
    goto shutdown_glfw;  // NOLINT(hicpp-avoid-goto)
  }

  namespace bf = bifrost;

  BifrostVMParams vm_params;
  bfVMParams_init(&vm_params);
  vm_params.error_fn = &userErrorFn;
  vm_params.print_fn = [](BifrostVM*, const char* message) {
    std::cout << message << "\n";
  };
  vm_params.min_heap_size = 50;
  vm_params.heap_size     = 100;
  BifrostVM* const vm     = bfVM_new(&vm_params);

  const BifrostVMClassBind clz_bind = bf::vmMakeClassBinding<TestClass>(
   "TestClass",
   bifrost::vmMakeCtorBinding<TestClass, int>("ctor"),
   bfVM_makeMemberBinding("printf", &TestClass::printf));

  bfVM_stackResize(vm, 1);
  bfVM_moduleMake(vm, 0, "main");
  bfVM_moduleBindClass(vm, 0, &clz_bind);
  bf::vmBindNativeFn<decltype(testFN), testFN>(vm, 0, "cppFn");

  const BifrostVMError err = bfVM_execInModule(vm, "main2", source, std::size(source));

  bfValueHandle update_fn = nullptr;

  if (!err)
  {
    bfVM_stackResize(vm, 1);
    bfVM_moduleLoad(vm, 0, "main2");
    bfVM_stackLoadVariable(vm, 0, 0, "callMeFromCpp");

    bf::vmCall(vm, 0, 45, std::string("Hello from cpp") + "!!!", false);

    bfVM_moduleLoad(vm, 0, "main2");
    bfVM_stackLoadVariable(vm, 0, 0, "update");
    update_fn = bfVM_stackMakeHandle(vm, 0);
  }

  {
    BifrostEngine engine{argc, argv};

    const BifrostEngineCreateParams params = {
     argv[0],
     0,
     INITIAL_WINDOW_SIZE[0],
     INITIAL_WINDOW_SIZE[1],
    };

    engine.init(params);

    // clang-format off
    static const float vertices[] = {
      -0.5f, -0.5f, 0.0f,
       0.5f, -0.5f, 0.0f,
       0.0f,  0.5f, 0.0f,
    };
    // clang-format on

    const char* vertexShaderSource = "#version 330 core\n"
                                     "layout (location = 0) in vec3 aPos;\n"
                                     "void main()\n"
                                     "{\n"
                                     "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                     "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
                                       "out vec4 FragColor;\n"
                                       "void main()\n"
                                       "{\n"
                                       "   FragColor = vec4(0.5f, 0.8f, 0.2f, 1.0f);\n"
                                       "}\n\0";

    const auto createShader = [](int type, const char* source) {
      const int vertexShader = glCreateShader(type);
      glShaderSource(vertexShader, 1, &source, nullptr);
      glCompileShader(vertexShader);

      int  success;
      char infoLog[512];
      glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
      if (!success)
      {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                  << infoLog << std::endl;
      }

      return vertexShader;
    };

    const auto vertexShader   = createShader(GL_VERTEX_SHADER, vertexShaderSource);
    const auto fragmentShader = createShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    const int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    int  success;
    char infoLog[512];
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
      glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindVertexArray(VAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(main_window))
    {
      int window_width, window_height;
      glfwGetWindowSize(main_window, &window_width, &window_height);

      glfwPollEvents();

      if (engine.beginFrame())
      {
        glViewport(0, 0, window_width, window_height);
        glClearColor(0.4f, 0.5f, 0.7f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        if (update_fn)
        {
          bfVM_stackResize(vm, 1);
          bfVM_stackLoadHandle(vm, 0, update_fn);

          if (bfVM_stackGetType(vm, 0) == BIFROST_VM_FUNCTION)
          {
            bf::vmCall(vm, 0);
          }
        }

        engine.endFrame();
      }

      glfwSwapBuffers(main_window);
    }

    engine.deinit();

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
  }

  bfVM_stackDestroyHandle(vm, update_fn);
  bfVM_delete(vm);

shutdown_glfw:
  glfwDestroyWindow(main_window);
  glfwTerminate();

shutdown_exit:
  return error_code;
}
