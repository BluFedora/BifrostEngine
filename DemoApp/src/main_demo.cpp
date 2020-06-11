#include "demo/game_state_layers/main_demo.hpp"

#include "bifrost/bifrost.hpp"
#include "bifrost/event/bifrost_window_event.hpp"

#include "glfw/glfw3.h"

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

void MainDemoLayer::onEvent(BifrostEngine&, bifrost::Event& event)
{
  if (event.type == bifrost::EventType::ON_KEY_DOWN && event.keyboard.key == 'P' && event.keyboard.modifiers & bifrost::KeyboardEvent::CONTROL)
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

void MainDemoLayer::onUpdate(BifrostEngine& engine, float delta_time)
{
  const auto camera_move_speed = 1.0f * delta_time;

  const std::tuple<int, void (*)(::BifrostCamera*, float), float> CameraControls[] =
   {
    {GLFW_KEY_W, &Camera_moveForward, camera_move_speed},
    {GLFW_KEY_A, &Camera_moveLeft, camera_move_speed},
    {GLFW_KEY_S, &Camera_moveBackward, camera_move_speed},
    {GLFW_KEY_D, &Camera_moveRight, camera_move_speed},
    {GLFW_KEY_Q, &Camera_moveUp, camera_move_speed},
    {GLFW_KEY_E, &Camera_moveDown, camera_move_speed},
    {GLFW_KEY_R, &Camera_addPitch, -0.01f},
    {GLFW_KEY_F, &Camera_addPitch, 0.01f},
    {GLFW_KEY_H, &Camera_addYaw, 0.01f},
    {GLFW_KEY_G, &Camera_addYaw, -0.01f},
   };

  auto& Camera = engine.Camera;

  for (const auto& control : CameraControls)
  {
    if (glfwGetKey(g_Window, std::get<0>(control)) == GLFW_PRESS)
    {
      std::get<1>(control)(&Camera, std::get<2>(control));
    }
  }

  Camera_update(&Camera);
}
