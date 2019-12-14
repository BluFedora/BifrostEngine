#include "bifrost/platform/bifrost_window_glfw.hpp"

#include "bifrost/bifrost_std.h"  // bfInvalidDefaultCase

namespace bifrost
{
  static glfw::ControllerEventCallback* s_onControllerEvent;
  static glfw::ErrorCallback*           s_onGLFWError;

  void GLFW_onJoystickStateChanged(int joy, int event)
  {
    if (s_onControllerEvent)
    {
      const char* name = glfwGetJoystickName(joy);

      ControllerEvent::Type evt_type = ControllerEvent::ON_CONTROLLER_CONNECTED;

      if (event == GLFW_CONNECTED)
      {
        evt_type = ControllerEvent::ON_CONTROLLER_CONNECTED;
      }
      else if (event == GLFW_DISCONNECTED)
      {
        evt_type = ControllerEvent::ON_CONTROLLER_DISCONNECTED;
      }

      s_onControllerEvent->safeCall(ControllerEvent(evt_type, name, joy));
    }
  }

  static void GLFW_errorCallback(int errorCode, const char* message)
  {
    if (s_onGLFWError)
    {
      s_onGLFWError->safeCall(errorCode, message);
    }
  }

  WindowGLFW& getWindow(GLFWwindow* window)
  {
    return *static_cast<WindowGLFW*>(glfwGetWindowUserPointer(window));
  }

  static std::uint8_t keyModifiers(int mods)
  {
    std::uint8_t result = 0x0;

    if (mods & GLFW_MOD_SHIFT)
    {
      result |= KeyboardEvent::SHIFT;
    }

    if (mods & GLFW_MOD_CONTROL)
    {
      result |= KeyboardEvent::CONTROL;
    }

    if (mods & GLFW_MOD_ALT)
    {
      result |= KeyboardEvent::ALT;
    }

    if (mods & GLFW_MOD_SUPER)
    {
      result |= KeyboardEvent::SUPER;
    }

    if (mods & GLFW_MOD_CAPS_LOCK)
    {
      result |= KeyboardEvent::IS_CAPS_LOCKED;
    }

    if (mods & GLFW_MOD_NUM_LOCK)
    {
      result |= KeyboardEvent::IS_NUM_LOCKED;
    }

    return result;
  }

  static std::uint8_t buttonState(GLFWwindow* window)
  {
    std::uint8_t result = 0x0;

    for (int i = GLFW_MOUSE_BUTTON_1; i <= GLFW_MOUSE_BUTTON_LAST; ++i)
    {
      if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
      {
        result |= (1 << i);
      }
    }

    return result;
  }

  static void GLFW_onKeyChanged(GLFWwindow* window, int key, int scan_code, int action, int mods)
  {
    auto& w = getWindow(window);

    EventType evt_type = EventType::ON_KEY_DOWN;

    switch (action)
    {
      case GLFW_PRESS:
      {
        evt_type = EventType::ON_KEY_DOWN;
        break;
      }
      case GLFW_REPEAT:
      {
        evt_type = EventType::ON_KEY_HELD;
        break;
      }
      case GLFW_RELEASE:
      {
        evt_type = EventType::ON_KEY_UP;
        break;
      }
        bfInvalidDefaultCase();
    }

    w.pushEvent(evt_type, KeyboardEvent(key, keyModifiers(mods)));
  }

  static void GLFW_onMousePosChanged(GLFWwindow* window, double x_pos, double y_pos)
  {
    auto& w = getWindow(window);

    w.pushEvent(EventType::ON_MOUSE_MOVE, MouseEvent(static_cast<int>(x_pos), static_cast<int>(y_pos), MouseEvent::BUTTON_NONE, buttonState(window)));
  }

  static void GLFW_onMouseButtonChanged(GLFWwindow* window, int button, int action, int mods)
  {
    auto& w = getWindow(window);

    double x_pos, y_pos;
    glfwGetCursorPos(window, &x_pos, &y_pos);

    EventType evt_type = EventType::ON_MOUSE_DOWN;

    switch (action)
    {
      case GLFW_PRESS:
      {
        evt_type = EventType::ON_MOUSE_DOWN;
        break;
      }
      case GLFW_RELEASE:
      {
        evt_type = EventType::ON_MOUSE_UP;
        break;
      }
        bfInvalidDefaultCase();
    }

    std::uint8_t target_button;

    switch (button)
    {
      case GLFW_MOUSE_BUTTON_LEFT:
      {
        target_button = MouseEvent::BUTTON_LEFT;
        break;
      }
      case GLFW_MOUSE_BUTTON_MIDDLE:
      {
        target_button = MouseEvent::BUTTON_MIDDLE;
        break;
      }
      case GLFW_MOUSE_BUTTON_RIGHT:
      {
        target_button = MouseEvent::BUTTON_RIGHT;
        break;
      }
      case GLFW_MOUSE_BUTTON_4:
      {
        target_button = MouseEvent::BUTTON_EXTRA0;
        break;
      }
      case GLFW_MOUSE_BUTTON_5:
      {
        target_button = MouseEvent::BUTTON_EXTRA1;
        break;
      }
      case GLFW_MOUSE_BUTTON_6:
      {
        target_button = MouseEvent::BUTTON_EXTRA2;
        break;
      }
      case GLFW_MOUSE_BUTTON_7:
      {
        target_button = MouseEvent::BUTTON_EXTRA3;
        break;
      }
      case GLFW_MOUSE_BUTTON_8:
      {
        target_button = MouseEvent::BUTTON_EXTRA4;
        break;
      }
      default:
      {
        target_button = MouseEvent::BUTTON_NONE;
        break;
      }
    }

    w.pushEvent(evt_type, MouseEvent(static_cast<int>(x_pos), static_cast<int>(y_pos), target_button, buttonState(window)));
  }

  void GLFW_onWindowFileDropped(GLFWwindow* window, int count, const char** paths)
  {
    auto& w = getWindow(window);

    w.onFileDrop().safeCall(FileEvent(w, paths, count));
  }

  static void GLFW_onWindowSizeChanged(GLFWwindow* window, int width, int height)
  {
    auto& w = getWindow(window);

    w.pushEvent(EventType::ON_WINDOW_RESIZE, WindowEvent(width, height, WindowEvent::FLAGS_DEFAULT));
  }

  static void GLFW_onWindowCharacterInput(GLFWwindow* window, unsigned int codepoint)
  {
    auto& w = getWindow(window);

    w.pushEvent(EventType::ON_KEY_INPUT, KeyboardEvent(codepoint));
  }

  static void GLFW_onScrollWheel(GLFWwindow* window, double x_offset, double y_offset)
  {
    auto& w = getWindow(window);

    w.pushEvent(EventType::ON_SCROLL_WHEEL, ScrollWheelEvent(x_offset, y_offset));
  }

  static void GLFW_onWindowIconify(GLFWwindow* window, int iconified)
  {
    auto& w = getWindow(window);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    w.pushEvent(EventType::ON_WINDOW_MINIMIZE, WindowEvent(width, height, iconified == GLFW_TRUE ? WindowEvent::IS_MINIMIZED : 0x0));
  }

  void GLFW_onWindowFocusChanged(GLFWwindow* window, int focused)
  {
    auto& w = getWindow(window);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    w.pushEvent(EventType::ON_WINDOW_FOCUS_CHANGED, WindowEvent(width, height, focused == GLFW_TRUE ? WindowEvent::IS_FOCUSED : 0x0));
  }

  static void GLFW_onWindowClose(GLFWwindow* window)
  {
    auto& w = getWindow(window);

    int width, height;
    glfwGetWindowSize(window, &width, &height);
    w.pushEvent(EventType::ON_WINDOW_CLOSE, WindowEvent(width, height, WindowEvent::FLAGS_DEFAULT));
    // glfwSetWindowShouldClose(window, GLFW_FALSE);
  }

  WindowGLFW::WindowGLFW() :
    // IBaseWindow(),
    m_WindowHandle{nullptr}
  {
  }

  bool WindowGLFW::open(const char* title, int width, int height)
  {
    if (!m_WindowHandle)
    {
      m_WindowHandle = glfwCreateWindow(width, height, title, nullptr, nullptr);

      if (m_WindowHandle)
      {
        glfwSetWindowUserPointer(m_WindowHandle, this);
        glfwSetKeyCallback(m_WindowHandle, GLFW_onKeyChanged);
        glfwSetCursorPosCallback(m_WindowHandle, GLFW_onMousePosChanged);
        glfwSetMouseButtonCallback(m_WindowHandle, GLFW_onMouseButtonChanged);
        glfwSetDropCallback(m_WindowHandle, GLFW_onWindowFileDropped);
        glfwSetWindowSizeCallback(m_WindowHandle, GLFW_onWindowSizeChanged);
        glfwSetCharCallback(m_WindowHandle, GLFW_onWindowCharacterInput);
        glfwSetScrollCallback(m_WindowHandle, GLFW_onScrollWheel);
        glfwSetWindowIconifyCallback(m_WindowHandle, GLFW_onWindowIconify);
        glfwSetWindowFocusCallback(m_WindowHandle, GLFW_onWindowFocusChanged);
        glfwSetWindowCloseCallback(m_WindowHandle, GLFW_onWindowClose);
      }
    }

    return m_WindowHandle != nullptr;
  }

  void WindowGLFW::close()
  {
    if (m_WindowHandle)
    {
      glfwDestroyWindow(m_WindowHandle);
      m_WindowHandle = nullptr;
    }
  }

  bool WindowGLFW::wantsToClose()
  {
    if (!m_WindowHandle)
    {
      return true;
    }

    return glfwWindowShouldClose(m_WindowHandle);
  }

  WindowGLFW::~WindowGLFW()
  {
    WindowGLFW::close();
  }

  bool startupGLFW(glfw::ControllerEventCallback* onControllerEvent, glfw::ErrorCallback* onGLFWError)
  {
    const int was_success = glfwInit() == GLFW_TRUE;

    if (was_success)
    {
      s_onControllerEvent = onControllerEvent;
      s_onGLFWError       = onGLFWError;

      glfwSetJoystickCallback(GLFW_onJoystickStateChanged);
      glfwSetErrorCallback(GLFW_errorCallback);
    }

    return was_success;
  }

  void shutdownGLFW()
  {
    glfwTerminate();
  }
}  // namespace bifrost
