/*!
 * @file   bifrost_window_event.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_WINDOW_EVENT_HPP
#define BIFROST_WINDOW_EVENT_HPP

#include <cstdint> /* uint8_t */

namespace bifrost
{
  struct KeyboardEvent final
  {
    enum flags_t
    {
      CONTROL        = (1 << 0),
      SHIFT          = (1 << 1),
      ALT            = (1 << 2),
      SUPER          = (1 << 3),
      IS_NUM_LOCKED  = (1 << 4),
      IS_CAPS_LOCKED = (1 << 5),
    };

    union
    {
      int      key;
      unsigned codepoint;
    };
    std::uint8_t modifiers;

    KeyboardEvent(int key, std::uint8_t modifiers) :
      key{key},
      modifiers{modifiers}
    {
    }

    KeyboardEvent(unsigned codepoint) :
      codepoint{codepoint},
      modifiers{0x0}
    {
    }
  };

  struct MouseEvent final
  {
    enum flags_t
    {
      BUTTON_LEFT   = (1 << 0),
      BUTTON_RIGHT  = (1 << 1),
      BUTTON_MIDDLE = (1 << 2),
      BUTTON_EXTRA0 = (1 << 3),
      BUTTON_EXTRA1 = (1 << 4),
      BUTTON_EXTRA2 = (1 << 5),
      BUTTON_EXTRA3 = (1 << 6),
      BUTTON_EXTRA4 = (1 << 7),
      BUTTON_NONE   = -1,
    };

    int          x;
    int          y;
    std::uint8_t target_button;
    std::uint8_t button_state;  // flags_t

    MouseEvent(int x, int y, std::uint8_t target_button, std::uint8_t button_state) :
      x{x},
      y{y},
      target_button{target_button},
      button_state{button_state}
    {
    }
  };

  struct ScrollWheelEvent final
  {
    double x;
    double y;

    ScrollWheelEvent(double x, double y) :
      x{x},
      y{y}
    {
    }
  };

  struct WindowEvent final
  {
    enum flags_t
    {
      FLAGS_DEFAULT = 0x0,
      IS_MINIMIZED  = (1 << 0),
      IS_FOCUSED    = (1 << 1),
    };

    int          width;
    int          height;
    std::uint8_t state;  // flags_t

    WindowEvent(int width, int height, std::uint8_t state) :
      width{width},
      height{height},
      state{state}
    {
    }
  };

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

  struct ControllerButtonEvent final
  {
    ControllerButton button;

    ControllerButtonEvent(ControllerButton button) :
      button{button}
    {
    }
  };

  struct ControllerAxisEvent final
  {
    ControllerAxis axis;

    ControllerAxisEvent(ControllerAxis axis) :
      axis{axis}
    {
    }
  };

  enum class EventType : unsigned char
  {
    // Button Events
    ON_BUTTON_PRESSED,
    ON_BUTTON_DOWN,
    ON_BUTTON_RELEASED,

    // Axes Events
    ON_AXES_STATIC,
    ON_AXES_MOVED,

    // Key Events
    ON_KEY_DOWN,
    ON_KEY_HELD,
    ON_KEY_UP,
    ON_KEY_INPUT,

    // Mouse Events
    ON_MOUSE_DOWN,
    ON_MOUSE_MOVE,
    ON_MOUSE_UP,

    // Scroll Events
    ON_SCROLL_WHEEL,

    // Window Events
    ON_WINDOW_RESIZE,
    ON_WINDOW_CLOSE,
    ON_WINDOW_MINIMIZE,
    ON_WINDOW_FOCUS_CHANGED,
  };

  class IBaseWindow;

  struct Event final
  {
    enum flags_t
    {
      FLAGS_DEFAULT      = 0x0,
      FLAGS_IS_ACCEPTED  = (1 << 0),
      FLAGS_IS_FALSIFIED = (1 << 1),
    };

    EventType    type;
    IBaseWindow* target;
    std::uint8_t flags;

    bool isAccepted() const
    {
      return flags & FLAGS_IS_ACCEPTED;
    }

    void accept()
    {
      flags |= FLAGS_IS_ACCEPTED;
    }

    union
    {
      KeyboardEvent    keyboard;
      MouseEvent       mouse;
      ScrollWheelEvent scroll_wheel;
      WindowEvent      window;
      ControllerButton button;
      ControllerAxis   axis;
    };

    Event(EventType type, IBaseWindow* target, std::uint8_t flags, KeyboardEvent key) :
      Event(type, target, flags)
    {
      this->keyboard = key;
    }

    Event(EventType type, IBaseWindow* target, std::uint8_t flags, MouseEvent mouse) :
      Event(type, target, flags)
    {
      this->mouse = mouse;
    }

    Event(EventType type, IBaseWindow* target, std::uint8_t flags, ScrollWheelEvent scroll_wheel) :
      Event(type, target, flags)
    {
      this->scroll_wheel = scroll_wheel;
    }

    Event(EventType type, IBaseWindow* target, std::uint8_t flags, WindowEvent window) :
      Event(type, target, flags)
    {
      this->window = window;
    }

    Event(EventType type, IBaseWindow* target, std::uint8_t flags, ControllerButton button) :
      Event(type, target, flags)
    {
      this->button = button;
    }

    Event(EventType type, IBaseWindow* target, std::uint8_t flags, ControllerAxis axis) :
      Event(type, target, flags)
    {
      this->axis = axis;
    }

   private:
    Event(EventType type, IBaseWindow* target, std::uint8_t flags) :
      type{type},
      target{target},
      flags{flags}
    {
    }
  };

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
}  // namespace bifrost

#endif /* BIFROST_WINDOW_EVENT_HPP */
