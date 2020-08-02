//
// main.cpp
//

#include "bf/Platform.h"                         /* Platform API */
#include <bifrost/core/bifrost_engine.hpp>       /* Runtime API  */
#include <bifrost/memory/bifrost_memory_utils.h> /* bfMegabytes  */

#include <chrono> /* chrono     */
#include <cstdio> /* printf     */
#include <memory> /* unique_ptr */

int main(int argc, char* argv[])
{
  using namespace std::chrono_literals;
  using namespace bf;

  static constexpr float                    k_NanoSecToSec = 1.0f / 1000000000.0f;
  static constexpr std::chrono::nanoseconds k_TimeStep     = 16ms;
  static constexpr float                    k_TimeStepSec  = k_TimeStep.count() * k_NanoSecToSec;
  static const char*                        k_AppName      = "Dungeon Swiper";

  bfPlatformAllocator allocator       = nullptr;
  void*               global_userdata = nullptr;

  if (!bfPlatformInit({argc, argv, allocator, global_userdata}))
  {
    std::printf("Failed to initialize the platform.\n");
    return 1;
  }

  const auto           window_flags = BIFROST_WINDOW_FLAG_IS_VISIBLE | BIFROST_WINDOW_FLAG_IS_DECORATED | BIFROST_WINDOW_FLAG_IS_FOCUSED_ON_SHOW;
  bfWindow* const main_window  = bfPlatformCreateWindow(k_AppName, 320, 568, window_flags);

  if (!main_window)
  {
    std::printf("Failed to create the window.\n");
    bfPlatformQuit();
    return 2;
  }

  const std::size_t             engine_memory_size = bfMegabytes(10);
  const std::unique_ptr<char[]> engine_memory      = std::make_unique<char[]>(engine_memory_size);
  std::unique_ptr<Engine>       engine             = std::make_unique<Engine>(engine_memory.get(), engine_memory_size, argc, argv);

  main_window->user_data     = engine.get();
  main_window->renderer_data = nullptr;

  const BifrostEngineCreateParams engine_init_params = {k_AppName, bfGfxMakeVersion(1, 0, 0)};

  engine->init(engine_init_params, main_window);

  main_window->event_fn = [](bfWindow* window, bfEvent* evt) {
    Engine* const engine = static_cast<Engine*>(window->user_data);

    engine->onEvent(*evt);
  };

  using clock = std::chrono::high_resolution_clock;

  static std::chrono::nanoseconds s_TimeStepLag = 0ns;
  static auto                     s_CurrentTime = clock::now();

  main_window->frame_fn = [](bfWindow* window) {
    Engine* const engine = static_cast<Engine*>(window->user_data);

    auto delta_time = clock::now() - s_CurrentTime;

    s_CurrentTime = clock::now();
    s_TimeStepLag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

    if (engine->beginFrame())
    {
      while (s_TimeStepLag >= k_TimeStep)
      {
        engine->fixedUpdate(k_TimeStepSec);
        s_TimeStepLag -= k_TimeStep;
      }

      const float dt_seconds = std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time).count() * k_NanoSecToSec;

      engine->update(dt_seconds);

      const float render_alpha = (float)s_TimeStepLag.count() / k_TimeStep.count();

      engine->drawBegin(render_alpha);

      // Custom Drawing Here

      engine->drawEnd();

      engine->endFrame();
    }
  };

  bfPlatformDoMainLoop(main_window);

  engine->deinit();
  bfPlatformDestroyWindow(main_window);
  bfPlatformQuit();

  return 0;
}
