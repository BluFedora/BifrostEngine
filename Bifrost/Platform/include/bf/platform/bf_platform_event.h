/*!
 * @file   bf_platform_event.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
#ifndef BF_PLATFORM_EVENT_H
#define BF_PLATFORM_EVENT_H

#include "bf_platform_export.h"

#include <stdint.h> /* uint8_t */

// clang-format off

// TODO: The rest of the keys...
#define BIFROST_KEY_ESCAPE    256
#define BIFROST_KEY_ENTER     257
#define BIFROST_KEY_TAB       258
#define BIFROST_KEY_LEFT      259
#define BIFROST_KEY_RIGHT     260
#define BIFROST_KEY_UP        261
#define BIFROST_KEY_DOWN      262
#define BIFROST_KEY_PAGE_UP   263
#define BIFROST_KEY_PAGE_DOWN 264
#define BIFROST_KEY_HOME      265
#define BIFROST_KEY_END       266
#define BIFROST_KEY_INSERT    267
#define BIFROST_KEY_DELETE    268
#define BIFROST_KEY_BACKSPACE 269
#define BIFROST_KEY_PAD_ENTER 270

#define BIFROST_KEY_SPACE ' '

#define BIFROST_KEY_A     'A'
#define BIFROST_KEY_B     'B'
#define BIFROST_KEY_C     'C'
#define BIFROST_KEY_D     'D'
#define BIFROST_KEY_E     'E'
#define BIFROST_KEY_F     'F'
#define BIFROST_KEY_G     'G'
#define BIFROST_KEY_H     'H'
#define BIFROST_KEY_I     'I'
#define BIFROST_KEY_J     'J'
#define BIFROST_KEY_K     'K'
#define BIFROST_KEY_L     'L'
#define BIFROST_KEY_M     'M'
#define BIFROST_KEY_N     'N'
#define BIFROST_KEY_O     'O'
#define BIFROST_KEY_P     'P'
#define BIFROST_KEY_Q     'Q'
#define BIFROST_KEY_R     'R'
#define BIFROST_KEY_S     'S'
#define BIFROST_KEY_T     'T'
#define BIFROST_KEY_U     'U'
#define BIFROST_KEY_V     'V'
#define BIFROST_KEY_W     'W'
#define BIFROST_KEY_X     'X'
#define BIFROST_KEY_Y     'Y'
#define BIFROST_KEY_Z     'Z'

#define k_KeyCodeMax      BIFROST_KEY_PAD_ENTER

// clang-format on

#if __cplusplus
extern "C" {
#endif

enum
{
  BIFROST_KEY_FLAG_CONTROL        = (1 << 0),
  BIFROST_KEY_FLAG_SHIFT          = (1 << 1),
  BIFROST_KEY_FLAG_ALT            = (1 << 2),
  BIFROST_KEY_FLAG_SUPER          = (1 << 3),
  BIFROST_KEY_FLAG_IS_NUM_LOCKED  = (1 << 4),
  BIFROST_KEY_FLAG_IS_CAPS_LOCKED = (1 << 5),

  BIFROST_BUTTON_LEFT   = (1 << 0),
  BIFROST_BUTTON_RIGHT  = (1 << 1),
  BIFROST_BUTTON_MIDDLE = (1 << 2),
  BIFROST_BUTTON_EXTRA0 = (1 << 3),
  BIFROST_BUTTON_EXTRA1 = (1 << 4),
  BIFROST_BUTTON_EXTRA2 = (1 << 5),
  BIFROST_BUTTON_EXTRA3 = (1 << 6),
  BIFROST_BUTTON_EXTRA4 = (1 << 7),
  BIFROST_BUTTON_NONE   = -1,

  BIFROST_WINDOW_IS_NONE      = 0x0,
  BIFROST_WINDOW_IS_MINIMIZED = (1 << 0),
  BIFROST_WINDOW_IS_FOCUSED   = (1 << 1),

  BIFROST_EVT_FLAGS_DEFAULT      = 0x0,
  BIFROST_EVT_FLAGS_IS_ACCEPTED  = (1 << 0),
  BIFROST_EVT_FLAGS_IS_FALSIFIED = (1 << 1),
};

typedef uint8_t bfKeyModifiers;
typedef uint8_t bfButtonFlags;
typedef uint8_t bfWindowFlags;

typedef enum
{
  // Button Events
  BIFROST_EVT_ON_BUTTON_PRESSED,
  BIFROST_EVT_ON_BUTTON_DOWN,
  BIFROST_EVT_ON_BUTTON_RELEASED,

  // Axes Events
  BIFROST_EVT_ON_AXES_STATIC,
  BIFROST_EVT_ON_AXES_MOVED,

  // Key Events
  BIFROST_EVT_ON_KEY_DOWN,
  BIFROST_EVT_ON_KEY_HELD,
  BIFROST_EVT_ON_KEY_UP,
  BIFROST_EVT_ON_KEY_INPUT,
  // NOTE(Shareef): If you add any more Mouse Events then update the Event::isKeyEvent function.

  // Mouse Events
  BIFROST_EVT_ON_MOUSE_DOWN,
  BIFROST_EVT_ON_MOUSE_MOVE,
  BIFROST_EVT_ON_MOUSE_UP,
  // NOTE(Shareef): If you add any more Mouse Events then update the Event::isMouseEvent function.

  // Scroll Events
  BIFROST_EVT_ON_SCROLL_WHEEL,

  // Window Events
  BIFROST_EVT_ON_WINDOW_RESIZE,
  BIFROST_EVT_ON_WINDOW_CLOSE,
  BIFROST_EVT_ON_WINDOW_MINIMIZE,
  BIFROST_EVT_ON_WINDOW_FOCUS_CHANGED,

} bfEventType;

typedef struct bfKeyboardEvent_t
{
  union
  {
    int      key;
    unsigned codepoint;
  };
  bfKeyModifiers modifiers;

} bfKeyboardEvent;

typedef struct bfMouseEvent_t
{
  int           x;
  int           y;
  uint8_t       target_button;
  bfButtonFlags button_state;

} bfMouseEvent;

typedef struct bfScrollWheelEvent_t
{
  double x;
  double y;

} bfScrollWheelEvent;

typedef struct bfWindowEvent_t
{
  int           width;
  int           height;
  bfWindowFlags state;

} bfWindowEvent;

#if 0
  enum class ControllerButton : unsigned char
  {
    A_BUTTON,
    B_BUTTON,
    X_BUTTON,
    Y_BUTTON,

    LEFT_BUMPER_BUTTON,
    RB_BUMPER_BUTTON,

    SELECT_BUTTON,
    START_BUTTON,

    LEFT_STICK_BUTTON,
    RIGHT_STICK_BUTTON,

    DPAD_UP_BUTTON,
    DPAD_RIGHT_BUTTON,
    DPAD_DOWN_BUTTON,
    DPAD_LEFT_BUTTON
  };

  enum class ControllerAxis : unsigned char
  {
    LEFT_X_STICK,
    LEFT_Y_STICK,
    RIGHT_X_STICK,
    RIGHT_Y_STICK,
    LEFT_TRIGGER,
    RIGHT_TRIGGER
  };
#endif

struct bfEvent_t
{
  bfEventType type;
  uint8_t     flags;

  union
  {
    bfKeyboardEvent    keyboard;
    bfMouseEvent       mouse;
    bfScrollWheelEvent scroll_wheel;
    bfWindowEvent      window;
    // bfControllerButton button;
    // bfControllerAxis   axis;
  };

#if __cplusplus
  bool isAccepted() const
  {
    return flags & BIFROST_EVT_FLAGS_IS_ACCEPTED;
  }

  bool isFalsified() const
  {
    return flags & BIFROST_EVT_FLAGS_IS_FALSIFIED;
  }

  bool isType(bfEventType evt_type) const
  {
    return type == evt_type;
  }

  bool isKeyEvent() const
  {
    return isType(BIFROST_EVT_ON_KEY_DOWN) || isType(BIFROST_EVT_ON_KEY_HELD) || isType(BIFROST_EVT_ON_KEY_UP) || isType(BIFROST_EVT_ON_KEY_INPUT);
  }

  bool isMouseEvent() const
  {
    return isType(BIFROST_EVT_ON_MOUSE_DOWN) || isType(BIFROST_EVT_ON_MOUSE_MOVE) || isType(BIFROST_EVT_ON_MOUSE_UP);
  }

  void accept()
  {
    flags |= BIFROST_EVT_FLAGS_IS_ACCEPTED;
  }

  bfEvent_t(bfEventType type, uint8_t flags, bfKeyboardEvent key) :
    bfEvent_t(type, flags)
  {
    this->keyboard = key;
  }

  bfEvent_t(bfEventType type, uint8_t flags, bfMouseEvent mouse) :
    bfEvent_t(type, flags)
  {
    this->mouse = mouse;
  }

  bfEvent_t(bfEventType type, uint8_t flags, bfScrollWheelEvent scroll_wheel) :
    bfEvent_t(type, flags)
  {
    this->scroll_wheel = scroll_wheel;
  }

  bfEvent_t(bfEventType type, uint8_t flags, bfWindowEvent window) :
    bfEvent_t(type, flags)
  {
    this->window = window;
  }

#if 0
    Event(bfEventType type, uint8_t flags, bfControllerButton button) :
      Event(type, target, flags)
    {
      this->button = button;
    }

    Event(bfEventType type, uint8_t flags, bfControllerAxis axis) :
      Event(type, target, flags)
    {
      this->axis = axis;
    }
#endif

  // clang-format off
  // ReSharper disable once CppPossiblyUninitializedMember
  bfEvent_t(bfEventType type, uint8_t flags) :  // NOLINT
    type{type},
    flags{flags}
  {
  }
// clang-format on
#endif
};
#if 0
  struct ControllerEvent final
  {
    enum Type
    {
      ON_CONTROLLER_CONNECTED,
      ON_CONTROLLER_DISCONNECTED,
    };

    Type         type;
    const char*  name;
    unsigned int id;

    ControllerEvent(Type type, const char* name, unsigned int id) :
      type{type},
      name{name},
      id{id}
    {
    }
  };

  struct FileEvent final
  {
    IBaseWindow& window;
    const char** files;
    int          num_files;

    FileEvent(IBaseWindow& window, const char** files, int num_files) :
      window{window},
      files{files},
      num_files{num_files}
    {
    }
  };
#endif

typedef struct bfEvent_t bfEvent;
#if __cplusplus
}
#endif

BIFROST_PLATFORM_API bfKeyboardEvent    bfKeyboardEvent_makeKeyMod(int key, uint8_t modifiers);
BIFROST_PLATFORM_API bfKeyboardEvent    bfKeyboardEvent_makeCodepoint(unsigned codepoint);
BIFROST_PLATFORM_API bfMouseEvent       bfMouseEvent_make(int x, int y, uint8_t target_button, bfButtonFlags button_state);
BIFROST_PLATFORM_API bfScrollWheelEvent bfScrollWheelEvent_make(double x, double y);
BIFROST_PLATFORM_API bfWindowEvent      bfWindowEvent_make(int width, int height, bfWindowFlags state);
BIFROST_PLATFORM_API struct bfEvent_t   bfEvent_makeImpl(bfEventType type, uint8_t flags, const void* data, size_t data_size);

#define bfEvent_make(type, flags, data) \
  bfEvent_makeImpl((type), (flags), &data, sizeof(data))  // NOLINT(bugprone-macro-parentheses)

#endif /* BF_PLATFORM_EVENT_H */
