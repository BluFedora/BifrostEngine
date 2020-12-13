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

#include "bf/Gfx2DPainter.hpp"
#include "bf/LinearAllocator.hpp"
#include "bf/Math.hpp"
#include "bf/MemoryUtils.h"
#include "bf/PlatformFwd.h"

namespace bf
{
  //
  // Forward Declarations
  //

  struct Gfx2DPainter;
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

      if (!first_child)
      {
        first_child = child_casted;
      }

      if (last_child)
      {
        last_child->next_sibling = child_casted;
      }

      child->parent       = DownCast();
      child->prev_sibling = last_child;
      child->next_sibling = NULL;

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
    Flex,      //!< The ratio of how much free space to take up raltive to other flex children.
  };

  struct SizeUnit
  {
    SizeUnitType type  = SizeUnitType::Absolute;
    float        value = 100.0f;

    //
    // TODO(SR): The `value` can be compressed by
    //   using Relative as a uint16 remap(0.0f, 1.0f => 0u => 0xFF)
    //   And the Other values as normal.
    //
    //   But how will it works with animations???
    //
  };

  struct Size
  {
    SizeUnit width  = {};
    SizeUnit height = {};
  };

  struct LayoutConstraints
  {
    Vector2f min_size = {0.0f, 0.0f};
    Vector2f max_size = {0.0f, 0.0f};
  };

  struct LayoutOutput
  {
    Vector2f desired_size = {0.0f, 0.0f};
  };

  using WidgetLayoutFn      = LayoutOutput (*)(Widget* self, const LayoutConstraints& constraints);
  using WidgetPositioningFn = void (*)(Widget* self);  // Positioning of Children
  using WidgetRenderFn      = void (*)(Widget* self, Gfx2DPainter& painter);

  enum class LayoutType
  {
    // Single Child Layouts
    Default,
    Padding,
    Fixed,

    // Multi-Child Layouts
    Row,
    Column,
    Grid,
    Stack,

    Custom,
  };

  struct WidgetLayout
  {
    LayoutType type;

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

    WidgetLayout() :
      type{LayoutType::Default}
    {
    }
  };

  enum class WidgetParams
  {
    HoverTime,
    ActiveTime,

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
    };

    WidgetLayout   layout               = {};
    char*          name                 = nullptr;
    std::size_t    name_len             = 0u;
    ParamList      params               = {0.0f};
    Size           desired_size         = {};
    Vector2f       position_from_parent = {200.0f, 200.0f};
    Vector2f       realized_size        = {0.0f, 0.0f};
    WidgetRenderFn render               = nullptr;
    std::uint64_t  flags                = IsExpanded;
    UIElementID    hash                 = 0x0;
    std::uint32_t  zindex               = 0;
    Widget*        hit_test_list        = nullptr;

    // TODO(SR): WidgetNavigationFn do_nav;
  };

  namespace UI
  {
    // State Manipulation

    UIElementID PushID(UIElementID local_id);
    UIElementID PushID(StringRange string_value);
    void        PopID();

    // Intertactable Widgets

    bool BeginWindow(const char* title);
    void EndWindow();
    bool Button(const char* name);

    // Layout Widgets

    void PushColumn();
    void PushRow();
    void PushFixedSize(SizeUnit width, SizeUnit height);
    void PushPadding(float value);
    void PopWidget();

    // System API

    void Init();
    void PumpEvents(bfEvent* event);
    void BeginFrame();
    void Update(float delta_time);
    void Render(Gfx2DPainter& painter);
  }  // namespace UI
}  // namespace bf

#endif /* BF_UI_HPP */
