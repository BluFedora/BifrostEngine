#include "demo/game_state_layers/main_demo.hpp"

#include "bifrost/bifrost.hpp"
#include "bifrost/event/bifrost_window_event.hpp"

#include "glfw/glfw3.h"

extern GLFWwindow* g_Window;

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

void MainDemoLayer::onEvent(Engine&, Event& event)
{
  const auto is_key_down = event.type == bifrost::EventType::ON_KEY_DOWN;

  if (is_key_down && event.keyboard.key == 'P' && event.keyboard.modifiers & bifrost::KeyboardEvent::CONTROL)
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

    event.accept();
  }
}

void MainDemoLayer::onUpdate(Engine& engine, float delta_time)
{
}
