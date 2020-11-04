//
// main.cpp
//

#include "bf/MemoryUtils.h"                /* bfMegabytes */
#include "bf/Platform.h"                   /* Platform API */
#include <bifrost/core/bifrost_engine.hpp> /* Runtime API  */

#include "bf/Font.hpp"          // Font
#include "bf/Gfx2DPainter.hpp"  // Gfx2DPainter

#include <cstdio> /* printf     */
#include <memory> /* unique_ptr */

// Gameplay Layer of Utilties
namespace bf
{
  std::pair<char*, std::size_t> makeTempString(Engine& engine, const char* fmt, ...)
  {
    std::size_t string_size;

    std::va_list args;

    va_start(args, fmt);
    char* string_buffer = string_utils::fmtAllocV(engine.tempMemory(), &string_size, fmt, args);
    va_end(args);

    return {string_buffer, string_size};
  }

  /// OOP Wrapper aroung the C-API Window.
  class Window
  {
   private:
    bfWindow* m_Handle;

   public:
    Window(bfWindow* handle) :
      m_Handle{handle}
    {
    }

    operator bfWindow*() const
    {
      return m_Handle;
    }

    Vector2i size() const
    {
      Vector2i ret_value;

      bfWindow_getSize(m_Handle, &ret_value.x, &ret_value.y);

      return ret_value;
    }
  };
}  // namespace bf

class Game : public bf::IGameStateLayer
{
  virtual const char* name() override
  {
    return "Dungeon Swiper";
  }

 private:
  bf::PainterFont* m_AbelFont;
  Window           m_MainWindow;
  float            button_shadow = 2.0f;
  float            dt            = 0.0f;

 public:
  Game(bfWindow* main_window) :
    m_AbelFont{nullptr},
    m_MainWindow{main_window}
  {
  }

 private:
  void onCreate(Engine& engine) override
  {
    m_AbelFont = engine.mainMemory().allocateT<PainterFont>(engine.mainMemory(), "assets/fonts/Abel.ttf", 16.0f * bfPlatformGetDPIScale());
  }

  void onUpdate(Engine& engine, float delta_time)
  {
    dt = delta_time;
  }

  void onDraw2D(Engine& engine, Gfx2DPainter& painter) override
  {
    const float dpi_scale = bfPlatformGetDPIScale();

    const auto& input         = engine.input();
    const auto  mouse_pos     = input.mousePos();
    const char* mouse_pos_txt = bf::makeTempString(engine, "FPS: %i\nMouse = {%ipx / %ipx}", int(1.0f / dt), mouse_pos.x, mouse_pos.y).first;

    painter.pushText(
     {5.0f, bf::fontNewlineHeight(m_AbelFont->font) + 5.0f},
     mouse_pos_txt,
     m_AbelFont);

    const auto window_size = m_MainWindow.size();

    const Rect2f button_rect   = {10.0f, 200.0f * dpi_scale, (window_size.x - 20.0f), 45.0f * dpi_scale};
    const bool   over_button   = button_rect.intersects(mouse_pos);
    const bool   mouse_is_down = input.mouseState().button_state & BIFROST_BUTTON_LEFT;

    if (mouse_is_down && over_button)
    {
      button_shadow = bf::math::clamp(0.2f, button_shadow - 20.0f * dt, button_shadow);
    }
    else
    {
      button_shadow = bf::math::clamp((!over_button ? 1.0f : 0.0f), button_shadow + (over_button ? 1.0f : -1.0f) * 20.0f * dt, 6.0f);
    }

    painter.pushRectShadow(
     button_shadow * dpi_scale,
     button_rect.topLeft() + bf::Vector2f{0.0f, 2.0f},
     button_rect.width(),
     button_rect.height(),
     6.0f,
     0xAA000000);

    painter.pushFillRoundedRect(
     button_rect.topLeft(),
     button_rect.width(),
     button_rect.height(),
     6.0f,
     0xFFDEDEDE);
  }

  void onDestroy(Engine& engine)
  {
    engine.mainMemory().deallocateT(m_AbelFont);
  }
};

int main(int argc, char* argv[])
{
  using namespace bf;

  static const char* k_AppName = "Dungeon Swiper";

  bfPlatformAllocator allocator       = nullptr;
  void*               global_userdata = nullptr;

  if (!bfPlatformInit({argc, argv, allocator, global_userdata}))
  {
    std::printf("Failed to initialize the platform.\n");
    return 1;
  }

  const auto      window_flags = BIFROST_WINDOW_FLAG_IS_RESIZABLE | BIFROST_WINDOW_FLAG_IS_VISIBLE | BIFROST_WINDOW_FLAG_IS_DECORATED | BIFROST_WINDOW_FLAG_IS_FOCUSED_ON_SHOW;
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

  const EngineCreateParams engine_init_params = {k_AppName, bfGfxMakeVersion(1, 0, 0), 60};

  engine->init(engine_init_params, main_window);

  engine->stateMachine().push<Game>(main_window);

  main_window->event_fn = [](bfWindow* window, bfEvent* evt) { static_cast<Engine*>(window->user_data)->onEvent(window, *evt); };
  main_window->frame_fn = [](bfWindow* window) { static_cast<Engine*>(window->user_data)->tick(); };

  bfPlatformDoMainLoop(main_window);

  engine->deinit();
  bfPlatformDestroyWindow(main_window);
  bfPlatformQuit();

  return 0;
}
