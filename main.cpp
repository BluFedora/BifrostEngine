#include "bifrost/bifrost_vm.hpp"
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <iostream> /*  */

//MSVC: You can use the /permissive- compiler option to specify standards-conforming compiler behavior. This option disables permissive behaviors, and sets the /Zc compiler options for strict conformance.

// TODO(SR):
//   This is a feature list that will be tackled
//   on a need-be basis for development of an indie engine / game.
//   [3987 lines. (But doesn't include bifrost ds lib.)]
//   * Error handling in scripts.
//   * Error throwing in native functions.
//   * Do while loop.
//   * Standard Lib
//   * Class inhericance.
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
//   * User defined callable objects. This would make "Closure" better. define a 'call' function?

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
//     -> A lot of Gameplay garabage
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
    * There are 3 bools in the component. Bitfield time.
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
    * Overall pretty nice and modular. + Lifetime management is interetsing.
    * Needs less jank serilization of tracks.
    * Needs to decide on how to manage complex animations in editor.
    * How can we use multiple objects?
    * How would the object IDs work?
    * Uses the default heap with std::make_shared<Timeline>.
    * Boolean support is very bad.
    * Bitflags for the Booleans.
    * Memory Layout is bad and the classes seem heavy.
  -> ASSET_IO (Area)
    * Fine, the way the enine used them was bad though.
    * Lifetime management waas bad.
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
  -> COLLLISION (General + Raycast)
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
  -> DATA_STRCUTURES (All)
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
    * Tetxure        - Turned into a cluster f**k w/ Gifs.
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
    * More native postprocesisng.
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
    var(f)
  {
  }

  const char* printf(float h) const
  {
    std::cout << "h = " << h << ", my var = " << var << "\n";
    return "__ Return from printf __";
  }

  ~TestClass() noexcept(true)
  {
    std::cout << "~TestClass(" << var << ")\n";
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

func update()
{
  print "I am updating!";
}

func callMeFromCpp(arg0, arg1, arg2)
{
  print "You passed in: " + arg0 + ", " + arg1 + ", "  + arg2;
  cppFn();
}
)";

namespace ErrorCodes
{
  static constexpr int GLFW_FAILED_TO_INIT = -1;
  static constexpr int GLAD_FAILED_TO_INIT = -2;
}  // namespace ErrorCodes

int main(int argc, const char* argv[])
{
  int error_code = 0;

  if (!glfwInit())
  {
    error_code = ErrorCodes::GLFW_FAILED_TO_INIT;
    goto shutdown_exit;  // NOLINT(hicpp-avoid-goto)
  }

  auto main_window = glfwCreateWindow(1280, 720, "Bifrost Engine", nullptr, nullptr);

  glfwMakeContextCurrent(main_window);

  if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
  {
    error_code = ErrorCodes::GLAD_FAILED_TO_INIT;
    goto shutdown_glfw;  // NOLINT(hicpp-avoid-goto)
  }

  namespace bf = bifrost;

  (void)argc;
  (void)argv;

  BifrostVMParams vm_params;
  bfVMParams_init(&vm_params);
  vm_params.error_fn = &userErrorFn;
  vm_params.print_fn = [](BifrostVM*, const char* message) {
    std::cout << message << "\n";
  };

  BifrostVM* const vm = bfVM_new(&vm_params);

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

  while (!glfwWindowShouldClose(main_window))
  {
    glfwPollEvents();

    glClearColor(0.4f, 0.5f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (update_fn)
    {
      bfVM_stackResize(vm, 1);
      bfVM_stackLoadHandle(vm, 0, update_fn);

      if (bfVM_stackGetType(vm, 0) == BIFROST_VM_FUNCTION)
      {
        bf::vmCall(vm, 0);
      }
    }

    glfwSwapBuffers(main_window);
  }

  // TODO(Shareef): Decide if I want destroying a null handle be ok.
  if (update_fn)
  {
    bfVM_stackDestroyHandle(vm, update_fn);
  }
  bfVM_delete(vm);

shutdown_glfw:
  glfwDestroyWindow(main_window);
  glfwTerminate();
shutdown_exit:
  return error_code;
}
