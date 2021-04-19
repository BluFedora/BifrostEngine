//
// Shareef Abdoul-Raheem
//
// References:
//   [https://www.youtube.com/watch?v=Z1qyvQsjK5Y]
//   [https://www.youtube.com/watch?v=UUfXWzp0-DU]
//   [https://mortoray.com/topics/writing-a-ui-engine/]
//
#ifndef BF_UI_HPP
#define BF_UI_HPP

#include "bf/LinearAllocator.hpp"
#include "bf/Math.hpp"
#include "bf/MemoryUtils.h"
#include "bf/PlatformFwd.h"
#include "bf/gfx/bf_draw_2d.hpp"

#include "sr_meta_interface.hpp"

namespace bf
{
  //
  // Forward Declarations
  //

  struct Widget;

  template<typename T>
  struct Hierarchy
  {
    T* parent       = nullptr;
    T* first_child  = nullptr;
    T* last_child   = nullptr;
    T* prev_sibling = nullptr;
    T* next_sibling = nullptr;

    void AddChild(Hierarchy<T>* child)
    {
      T* const child_casted = child->DownCast();

      child->parent       = DownCast();
      child->prev_sibling = last_child;
      child->next_sibling = nullptr;

      if (!first_child)
      {
        first_child = child_casted;
      }

      if (last_child)
      {
        last_child->next_sibling = child_casted;
      }

      last_child = child_casted;
    }

    template<typename F>
    void ForEachChild(F&& callback)
    {
      T* child = first_child;

      while (child)
      {
        callback(child);

        child = child->next_sibling;
      }
    }

    void Reset()
    {
      first_child  = nullptr;
      last_child   = nullptr;
      prev_sibling = nullptr;
      next_sibling = nullptr;
    }

    T* DownCast()
    {
      return static_cast<T*>(this);
    }
  };

  enum class SizeUnitType
  {
    Absolute,  //!< Size in points (DPI * pixels)
    Relative,  //!< The float is in a 0.0f => 1.0f range representing the % of the parent size you are.
    Flex,      //!< The ratio of how much free space to take up relative to other flex children.

#if defined(SR_META_COMPILER)
    ExtraSpecialMetaFeild,
#endif

  };

  struct SizeUnit
  {
    sr_meta() SizeUnitType type = SizeUnitType::Absolute;
    sr_meta() float value       = 0.0f;
  };

  struct Size
  {
    sr_meta() SizeUnit width  = {};
    sr_meta() SizeUnit height = {};

    Size() = default;

    Size(SizeUnit width, SizeUnit height) :
      width{width},
      height{height}
    {
    }

    Size(float width, float height) :
      width{SizeUnitType::Absolute, width},
      height{SizeUnitType::Absolute, height}
    {
    }
  };

  struct LayoutConstraints
  {
    sr_meta() Vector2f min_size = {0.0f, 0.0f};
    sr_meta() Vector2f max_size = {0.0f, 0.0f};
  };

  struct LayoutOutput
  {
    sr_meta() Vector2f desired_size = {0.0f, 0.0f};
  };

  using WidgetLayoutFn      = LayoutOutput (*)(Widget* self, const LayoutConstraints& constraints);
  using WidgetPositioningFn = void (*)(Widget* self);  // Positioning of Children
  using WidgetRenderFn      = void (*)(Widget* self, CommandBuffer2D& gfx2D);

  // [https://flutter.dev/docs/development/ui/widgets/layout]

  enum class sr_meta() LayoutType
  {
    // Single Child Layouts
    Stack,
    Padding,  // TODO(SR): Remove me and just give all widgets padding.
    Fixed,

    // Multi-Child Layouts
    Row,
    Column,
    Grid,

    Custom,
  };

  struct WidgetPadding
  {
    SizeUnit top    = {};
    SizeUnit bottom = {};
    SizeUnit left   = {};
    SizeUnit right  = {};
  };

  struct WidgetLayout
  {
    LayoutType type = LayoutType::Stack;

    union
    {
      struct
      {
        SizeUnit top;
        SizeUnit bottom;
        SizeUnit left;
        SizeUnit right;

      } padding;

      struct
      {
        WidgetLayoutFn      layout;
        WidgetPositioningFn position_children;

      } custom;
    };
  };

  enum class WidgetParams
  {
    HoverTime,
    ActiveTime,
    Padding,
    ScrollX,
    ScrollY,

    WidgetParams_Max,
  };

  using UIElementID = std::uint64_t;

  struct Widget : public Hierarchy<Widget>
  {
    using ParamList = float[int(WidgetParams::WidgetParams_Max)];

    enum /* WidgetFlags */
    {
      Clickable      = (1 << 0),
      Disabled       = (1 << 1),
      IsExpanded     = (1 << 2),
      DrawName       = (1 << 3),
      BlocksInput    = (1 << 4),
      DrawBackground = (1 << 5),
      IsWindow       = (1 << 6),
      ClipChildren   = (1 << 7),
      ScrollChildren = (1 << 8),
      NeedsScrollX   = (1 << 9),
      NeedsScrollY   = (1 << 10),

      IsInteractingWithScrollbar = (1 << 11),
    };

    WidgetLayout   layout               = {};
    char*          name                 = nullptr;
    std::size_t    name_len             = 0u;
    BufferRange    name_                = {};
    ParamList      params               = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    Size           desired_size         = {};
    Vector2f       position_from_parent = {5.0f, 5.0f};
    Vector2f       realized_size        = {0.0f, 0.0f};
    WidgetRenderFn render               = nullptr;
    std::uint64_t  flags                = 0x0;
    UIElementID    hash                 = 0x0;
    std::uint32_t  zindex               = 0;
    Widget*        hit_test_list        = nullptr;
    Vector2f       children_size        = {0.0f, 0.0f};

    bool IsFlagSet(std::uint64_t f) const { return (flags & f) != 0u; }
    void SetFlags(std::uint64_t f) { flags |= f; }
    void UnsetFlags(std::uint64_t f) { flags &= ~f; }

    // TODO(SR): WidgetNavigationFn do_nav;
  };

  constexpr int k_WidgetSize = sizeof(Widget);
  static_assert(k_WidgetSize <= 216, "This is just for keeping track of the size TODO(SR): Delete me!");

  struct WindowState
  {
    bool     can_be_dragged = true;
    Vector2i position       = {5, 5};
    Size     size           = {{SizeUnitType::Absolute, 350.0f}, {SizeUnitType::Absolute, 500.0f}};
  };

  namespace UI
  {
    // Hacks

    bf::PainterFont* xxx_Font();

    // State Manipulation

    UIElementID PushID(UIElementID local_id);
    UIElementID PushID(StringRange string_value);
    void        PopID();

    // Interact-able Widgets

    bool BeginWindow(const char* title, WindowState& state);
    void EndWindow();
    bool Button(const char* name);

    // Layout Widgets

    void PushColumn();
    void PushRow();
    void PushFixedSize(SizeUnit width, SizeUnit height);
    void PopWidget();

    // System API

    void Init();
    void PumpEvents(bfEvent* event);
    void BeginFrame();
    void Update(float delta_time);
    void Render(CommandBuffer2D& gfx2D, float screen_width, float screen_height);
    void ShutDown();
  }  // namespace UI
}  // namespace bf

#endif /* BF_UI_HPP */
