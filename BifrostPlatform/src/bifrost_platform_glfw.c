#include "bifrost/platform/bifrost_platform.h"

#include "bifrost/platform/bifrost_platform_event.h"
#include "bifrost/platform/bifrost_platform_vulkan.h"

#include <assert.h>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

extern bfPlatformInitParams g_BifrostPlatform;

int bfPlatformInit(bfPlatformInitParams params)
{
  const int was_success = glfwInit() == GLFW_TRUE;

  if (was_success)
  {
    g_BifrostPlatform = params;

    if (!g_BifrostPlatform.allocator)
    {
      g_BifrostPlatform.allocator = &bfPlatformDefaultAllocator;
    }
  }

  return was_success;
}

void bfPlatformPumpEvents(void)
{
  glfwPollEvents();
}

#if 0

  bool startupGLFW(glfw::ControllerEventCallback* onControllerEvent, glfw::ErrorCallback* onGLFWError)
  {
    s_onControllerEvent = onControllerEvent;
    s_onGLFWError       = onGLFWError;

    glfwSetJoystickCallback(GLFW_onJoystickStateChanged);
    glfwSetErrorCallback(GLFW_errorCallback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    return false;
  }

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
#endif

static BifrostWindow* getWindow(GLFWwindow* window)
{
  return (BifrostWindow*)glfwGetWindowUserPointer(window);
}

static void dispatchEvent(BifrostWindow* window, bfEvent event)
{
  if (window->event_fn)
  {
    window->event_fn(window, &event);
  }
}

static bfKeyModifiers convertKeyModifiers(int mods)
{
  bfKeyModifiers result = 0x0;

  if (mods & GLFW_MOD_SHIFT)
  {
    result |= BIFROST_KEY_FLAG_SHIFT;
  }

  if (mods & GLFW_MOD_CONTROL)
  {
    result |= BIFROST_KEY_FLAG_CONTROL;
  }

  if (mods & GLFW_MOD_ALT)
  {
    result |= BIFROST_KEY_FLAG_ALT;
  }

  if (mods & GLFW_MOD_SUPER)
  {
    result |= BIFROST_KEY_FLAG_SUPER;
  }

  if (mods & GLFW_MOD_CAPS_LOCK)
  {
    result |= BIFROST_KEY_FLAG_IS_CAPS_LOCKED;
  }

  if (mods & GLFW_MOD_NUM_LOCK)
  {
    result |= BIFROST_KEY_FLAG_IS_NUM_LOCKED;
  }

  return result;
}

static bfButtonFlags convertButtonState(GLFWwindow* window)
{
  bfButtonFlags result = 0x0;

  for (int i = GLFW_MOUSE_BUTTON_1; i <= GLFW_MOUSE_BUTTON_LAST; ++i)
  {
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
      result |= (1 << i);
    }
  }

  return result;
}

static int convertKey(int key)
{
  switch (key)
  {
    case GLFW_KEY_A: return BIFROST_KEY_A;
    case GLFW_KEY_B: return BIFROST_KEY_B;
    case GLFW_KEY_C: return BIFROST_KEY_C;
    case GLFW_KEY_D: return BIFROST_KEY_D;
    case GLFW_KEY_E: return BIFROST_KEY_E;
    case GLFW_KEY_F: return BIFROST_KEY_F;
    case GLFW_KEY_G: return BIFROST_KEY_G;
    case GLFW_KEY_H: return BIFROST_KEY_H;
    case GLFW_KEY_I: return BIFROST_KEY_I;
    case GLFW_KEY_J: return BIFROST_KEY_J;
    case GLFW_KEY_K: return BIFROST_KEY_K;
    case GLFW_KEY_L: return BIFROST_KEY_L;
    case GLFW_KEY_M: return BIFROST_KEY_M;
    case GLFW_KEY_N: return BIFROST_KEY_N;
    case GLFW_KEY_O: return BIFROST_KEY_O;
    case GLFW_KEY_P: return BIFROST_KEY_P;
    case GLFW_KEY_Q: return BIFROST_KEY_Q;
    case GLFW_KEY_R: return BIFROST_KEY_R;
    case GLFW_KEY_S: return BIFROST_KEY_S;
    case GLFW_KEY_T: return BIFROST_KEY_T;
    case GLFW_KEY_U: return BIFROST_KEY_U;
    case GLFW_KEY_V: return BIFROST_KEY_V;
    case GLFW_KEY_W: return BIFROST_KEY_W;
    case GLFW_KEY_X: return BIFROST_KEY_X;
    case GLFW_KEY_Y: return BIFROST_KEY_Y;
    case GLFW_KEY_Z: return BIFROST_KEY_Z;
    // default: return -1;
    // default: return 0;
    default: return key;
  }
}

static void GLFW_onKeyChanged(GLFWwindow* window, int key, int scan_code, int action, int mods)
{
  const int converted_key = convertKey(key);

  if (converted_key != -1)
  {
    bfEventType evt_type;

    switch (action)
    {
      case GLFW_PRESS:
      {
        evt_type = BIFROST_EVT_ON_KEY_DOWN;
        break;
      }
      case GLFW_REPEAT:
      {
        evt_type = BIFROST_EVT_ON_KEY_HELD;
        break;
      }
      case GLFW_RELEASE:
      {
        evt_type = BIFROST_EVT_ON_KEY_UP;
        break;
      }
      default:
        assert(!"bfInvalidDefaultCase()");
        return;
        // bfInvalidDefaultCase();
    }

    bfKeyboardEvent evt_data = bfKeyboardEvent_makeKeyMod(converted_key, convertKeyModifiers(mods));

    dispatchEvent(getWindow(window), bfEvent_make(evt_type, 0x0, evt_data));
  }
}

static void GLFW_onMousePosChanged(GLFWwindow* window, double x_pos, double y_pos)
{
  BifrostWindow* const w        = getWindow(window);
  bfMouseEvent         evt_data = bfMouseEvent_make((int)(x_pos), (int)(y_pos), BIFROST_BUTTON_NONE, convertButtonState(window));

  dispatchEvent(w, bfEvent_make(BIFROST_EVT_ON_MOUSE_MOVE, 0x0, evt_data));
}

static void GLFW_onMouseButtonChanged(GLFWwindow* window, int button, int action, int mods)
{
  BifrostWindow* const w = getWindow(window);

  double x_pos, y_pos;
  glfwGetCursorPos(window, &x_pos, &y_pos);

  bfEventType evt_type;

  switch (action)
  {
    case GLFW_PRESS:
    {
      evt_type = BIFROST_EVT_ON_MOUSE_DOWN;
      break;
    }
    case GLFW_RELEASE:
    {
      evt_type = BIFROST_EVT_ON_MOUSE_UP;
      break;
    }
    default:
      assert(!"bfInvalidDefaultCase()");
      return;
      // bfInvalidDefaultCase();
  }

  uint8_t target_button;

  switch (button)
  {
    case GLFW_MOUSE_BUTTON_LEFT:
    {
      target_button = BIFROST_BUTTON_LEFT;
      break;
    }
    case GLFW_MOUSE_BUTTON_MIDDLE:
    {
      target_button = BIFROST_BUTTON_MIDDLE;
      break;
    }
    case GLFW_MOUSE_BUTTON_RIGHT:
    {
      target_button = BIFROST_BUTTON_RIGHT;
      break;
    }
    case GLFW_MOUSE_BUTTON_4:
    {
      target_button = BIFROST_BUTTON_EXTRA0;
      break;
    }
    case GLFW_MOUSE_BUTTON_5:
    {
      target_button = BIFROST_BUTTON_EXTRA1;
      break;
    }
    case GLFW_MOUSE_BUTTON_6:
    {
      target_button = BIFROST_BUTTON_EXTRA2;
      break;
    }
    case GLFW_MOUSE_BUTTON_7:
    {
      target_button = BIFROST_BUTTON_EXTRA3;
      break;
    }
    case GLFW_MOUSE_BUTTON_8:
    {
      target_button = BIFROST_BUTTON_EXTRA4;
      break;
    }
    default:
    {
      target_button = BIFROST_BUTTON_NONE;
      break;
    }
  }

  bfMouseEvent evt_data = bfMouseEvent_make((int)(x_pos), (int)(y_pos), target_button, convertButtonState(window));

  dispatchEvent(w, bfEvent_make(evt_type, 0x0, evt_data));
}

void GLFW_onWindowFileDropped(GLFWwindow* window, int count, const char** paths)
{
  // auto& w = getWindow(window);

  // w.onFileDrop().safeCall(FileEvent(w, paths, count));
}

static void GLFW_onWindowSizeChanged(GLFWwindow* window, int width, int height)
{
  BifrostWindow* const w = getWindow(window);

  bfWindowEvent evt_data = bfWindowEvent_make(width, height, BIFROST_WINDOW_IS_NONE);

  dispatchEvent(w, bfEvent_make(BIFROST_EVT_ON_WINDOW_RESIZE, 0x0, evt_data));
}

static void GLFW_onWindowRefresh(GLFWwindow* window)
{
  BifrostWindow* const w = getWindow(window);

  if (w->frame_fn)
  {
    w->frame_fn(w);
  }
}

static void GLFW_onWindowCharacterInput(GLFWwindow* window, unsigned int codepoint)
{
  BifrostWindow* const w = getWindow(window);

  bfKeyboardEvent evt_data = bfKeyboardEvent_makeCodepoint(codepoint);

  dispatchEvent(w, bfEvent_make(BIFROST_EVT_ON_KEY_INPUT, 0x0, evt_data));
}

static void GLFW_onScrollWheel(GLFWwindow* window, double x_offset, double y_offset)
{
  BifrostWindow* const w = getWindow(window);

  bfScrollWheelEvent evt_data = bfScrollWheelEvent_make(x_offset, y_offset);

  dispatchEvent(w, bfEvent_make(BIFROST_EVT_ON_SCROLL_WHEEL, 0x0, evt_data));
}

static void GLFW_onWindowIconify(GLFWwindow* window, int iconified)
{
  BifrostWindow* const w = getWindow(window);

  int width, height;
  glfwGetWindowSize(window, &width, &height);

  bfWindowEvent evt_data = bfWindowEvent_make(width, height, iconified == GLFW_TRUE ? BIFROST_WINDOW_IS_MINIMIZED : 0x0);

  dispatchEvent(w, bfEvent_make(BIFROST_EVT_ON_WINDOW_MINIMIZE, 0x0, evt_data));
}

void GLFW_onWindowFocusChanged(GLFWwindow* window, int focused)
{
  BifrostWindow* const w = getWindow(window);

  int width, height;
  glfwGetWindowSize(window, &width, &height);

  bfWindowEvent evt_data = bfWindowEvent_make(width, height, focused == GLFW_TRUE ? BIFROST_WINDOW_IS_FOCUSED : 0x0);

  dispatchEvent(w, bfEvent_make(BIFROST_EVT_ON_WINDOW_FOCUS_CHANGED, 0x0, evt_data));
}

static void GLFW_onWindowClose(GLFWwindow* window)
{
  BifrostWindow* const w = getWindow(window);

  int width, height;
  glfwGetWindowSize(window, &width, &height);

  bfWindowEvent evt_data = bfWindowEvent_make(width, height, BIFROST_WINDOW_FLAGS_DEFAULT);

  dispatchEvent(w, bfEvent_make(BIFROST_EVT_ON_WINDOW_CLOSE, 0x0, evt_data));

  // glfwSetWindowShouldClose(window, GLFW_FALSE);
}

BifrostWindow* bfPlatformCreateWindow(const char* title, int width, int height, uint32_t flags)
{
  BifrostWindow* const window = bfPlatformAlloc(sizeof(BifrostWindow));

  if (window)
  {
    glfwWindowHint(GLFW_RESIZABLE, flags & BIFROST_WINDOW_FLAG_IS_RESIZABLE ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, flags & BIFROST_WINDOW_FLAG_IS_VISIBLE ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, flags & BIFROST_WINDOW_FLAG_IS_DECORATED ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, flags & BIFROST_WINDOW_FLAG_IS_MAXIMIZED ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, flags & BIFROST_WINDOW_FLAG_IS_FLOATING ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, flags & BIFROST_WINDOW_FLAG_IS_FOCUSED ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, flags & BIFROST_WINDOW_FLAG_IS_FOCUSED_ON_SHOW ? GLFW_TRUE : GLFW_FALSE);

    GLFWwindow* const glfw_handle = glfwCreateWindow(width, height, title, NULL, NULL);

    window->handle        = glfw_handle;
    window->event_fn      = NULL;
    window->frame_fn      = NULL;
    window->user_data     = NULL;
    window->renderer_data = NULL;

    glfwSetWindowUserPointer(glfw_handle, window);
    glfwSetKeyCallback(glfw_handle, GLFW_onKeyChanged);
    glfwSetCursorPosCallback(glfw_handle, GLFW_onMousePosChanged);
    glfwSetMouseButtonCallback(glfw_handle, GLFW_onMouseButtonChanged);
    glfwSetDropCallback(glfw_handle, GLFW_onWindowFileDropped);
    glfwSetWindowSizeCallback(glfw_handle, GLFW_onWindowSizeChanged);
    glfwSetCharCallback(glfw_handle, GLFW_onWindowCharacterInput);
    glfwSetScrollCallback(glfw_handle, GLFW_onScrollWheel);
    glfwSetWindowIconifyCallback(glfw_handle, GLFW_onWindowIconify);
    glfwSetWindowFocusCallback(glfw_handle, GLFW_onWindowFocusChanged);
    glfwSetWindowCloseCallback(glfw_handle, GLFW_onWindowClose);
    glfwSetWindowRefreshCallback(glfw_handle, GLFW_onWindowRefresh);

    // glfwSetWindowOpacity(m_WindowHandle, 0.9f);
  }

  return window;
}

int bfWindow_wantsToClose(BifrostWindow* self)
{
  return glfwWindowShouldClose(self->handle);
}

void bfWindow_show(BifrostWindow* self)
{
  glfwShowWindow(self->handle);
}

void bfWindow_getPos(BifrostWindow* self, int* x, int* y)
{
  glfwGetWindowPos(self->handle, x, y);
}

void bfWindow_setPos(BifrostWindow* self, int x, int y)
{
  glfwSetWindowPos(self->handle, x, y);
}

void bfWindow_getSize(BifrostWindow* self, int* x, int* y)
{
  glfwGetWindowSize(self->handle, x, y);
}

void bfWindow_setSize(BifrostWindow* self, int x, int y)
{
  glfwSetWindowSize(self->handle, x, y);
}

void bfWindow_focus(BifrostWindow* self)
{
  glfwFocusWindow(self->handle);
}

int bfWindow_isFocused(BifrostWindow* self)
{
  return glfwGetWindowAttrib(self->handle, GLFW_FOCUSED) != 0;
}

int bfWindow_isMinimized(BifrostWindow* self)
{
  return glfwGetWindowAttrib(self->handle, GLFW_ICONIFIED) != 0;
}

int bfWindow_isHovered(BifrostWindow* self)
{
  return glfwGetWindowAttrib(self->handle, GLFW_HOVERED) != 0;
}

void bfWindow_setTitle(BifrostWindow* self, const char* title)
{
  glfwSetWindowTitle(self->handle, title);
}

void bfWindow_setAlpha(BifrostWindow* self, float value)
{
  glfwSetWindowOpacity(self->handle, value);
}

void bfPlatformDestroyWindow(BifrostWindow* window)
{
  glfwDestroyWindow(window->handle);
}

void bfPlatformQuit(void)
{
  glfwTerminate();
}

int bfWindow_createVulkanSurface(BifrostWindow* self, VkInstance instance, VkSurfaceKHR* out)
{
  return glfwCreateWindowSurface(instance, self->handle, NULL, out) == VK_SUCCESS;
}
