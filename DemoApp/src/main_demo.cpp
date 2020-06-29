#include "main_demo.hpp"

#include "bifrost/bifrost.hpp"

#include "bifrost/platform/bifrost_platform_event.h"
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

MainDemoLayer::MainDemoLayer() :
  IGameStateLayer()
{
}

static bool do_fs = false;

void toggleFs()
{
  if (!do_fs) return;

  static bool isFullscreen = false;
  static int  old_info[4];

  if (isFullscreen)
  {
    glfwSetWindowMonitor(g_Window, nullptr, old_info[0], old_info[1], old_info[2], old_info[3], 60);
  }
  else
  {
    GLFWmonitor* monitor = get_current_monitor(g_Window);

    if (!monitor)
    {
      return;
    }

    glfwGetWindowPos(g_Window, &old_info[0], &old_info[1]);
    glfwGetWindowSize(g_Window, &old_info[2], &old_info[3]);

    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(g_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  }

  isFullscreen = !isFullscreen;
}

void MainDemoLayer::onEvent(Engine&, Event& event)
{
  const auto is_key_down = event.type == BIFROST_EVT_ON_KEY_DOWN;

  if (is_key_down && event.keyboard.key == 'P' && event.keyboard.modifiers & BIFROST_KEY_FLAG_CONTROL)
  {
    toggleFs();
    do_fs = true;
    event.accept();
  }
}

void MainDemoLayer::onUpdate(Engine& engine, float delta_time)
{
  // toggleFs();
}
