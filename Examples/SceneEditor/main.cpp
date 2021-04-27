//
// Shareef Abdoul-Raheem
// main file for the editor.
//

#include "bf/bifrost.hpp"
#include "bf/editor/bifrost_editor_overlay.hpp"

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

extern void XXX_RunRandomTests();

int main(int argc, char* argv[])
{
  XXX_RunRandomTests();

  using namespace bf;

  static_assert(std::numeric_limits<double>::is_iec559, "Use IEEE754, you weirdo.");

  ReturnCode err_code = ReturnCode::SUCCESS;

  if (bfPlatformInit({argc, argv, nullptr, nullptr}))
  {
    bfWindow* const main_window = bfPlatformCreateWindow("Mjolnir Editor 2021", 1280, 720, k_bfWindowFlagsDefault);

    if (main_window)
    {
      try
      {
        const std::size_t             engine_memory_size = bfMegabytes(200);
        const std::unique_ptr<char[]> engine_memory      = std::make_unique<char[]>(engine_memory_size);
        const std::unique_ptr<Engine> engine             = std::make_unique<Engine>(engine_memory.get(), engine_memory_size, argc, argv);
        const EngineCreateParams      params             = {{argv[0], 0}, 60};

        main_window->user_data = engine.get();
        main_window->event_fn  = [](bfWindow* window, bfEvent* event) { static_cast<Engine*>(window->user_data)->onEvent(window, *event); };
        main_window->frame_fn  = [](bfWindow* window) { static_cast<Engine*>(window->user_data)->tick(); };

        engine->init(params, main_window);
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
