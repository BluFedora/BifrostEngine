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

void MainDemoLayer::onEvent(BifrostEngine&, bifrost::Event& event)
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

  if (is_key_down || event.type == bifrost::EventType::ON_KEY_UP)
  {
    m_IsKeyDown[event.keyboard.key] = is_key_down;
    m_IsShiftDown                   = event.keyboard.modifiers & KeyboardEvent::SHIFT;
  }
}

void MainDemoLayer::onUpdate(BifrostEngine& engine, float delta_time)
{
  const auto camera_move_speed = (m_IsShiftDown ? 2.2f : 1.0f) * delta_time;

  const std::tuple<int, void (*)(::BifrostCamera*, float), float> camera_controls[] =
   {
    {KeyCode::W, &Camera_moveForward, camera_move_speed},
    {KeyCode::A, &Camera_moveLeft, camera_move_speed},
    {KeyCode::S, &Camera_moveBackward, camera_move_speed},
    {KeyCode::D, &Camera_moveRight, camera_move_speed},
    {KeyCode::Q, &Camera_moveUp, camera_move_speed},
    {KeyCode::E, &Camera_moveDown, camera_move_speed},
    {KeyCode::R, &Camera_addPitch, -0.01f},
    {KeyCode::F, &Camera_addPitch, 0.01f},
    {KeyCode::H, &Camera_addYaw, 0.01f},
    {KeyCode::G, &Camera_addYaw, -0.01f},
   };

  engine.forEachCamera([this, &camera_controls](CameraRender* camera) {
    for (const auto& control : camera_controls)
    {
      if (m_IsKeyDown[std::get<0>(control)])
      {
        std::get<1>(control)(&camera->cpu_camera, std::get<2>(control));
      }
    }

    Camera_update(&camera->cpu_camera);
  });
}
