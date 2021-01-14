//
// Shareef Abdoul-Raheem
//
// References:
//   [https://www.youtube.com/watch?v=Z1qyvQsjK5Y]
//   [https://www.youtube.com/watch?v=UUfXWzp0-DU]
//   [https://mortoray.com/topics/writing-a-ui-engine/]
//
#include "bf/bf_ui.hpp"

#include "bf/FreeListAllocator.hpp"
#include "bf/Platform.h"
#include "bf/Text.hpp"
#include "bf/bf_hash.hpp"

#include <algorithm>
#include <array>

namespace bf
{
  //
  // `T` must be default constructable.
  //
  template<typename Key, typename T>
  struct SortedArrayTable
  {
   private:
    struct ArrayItem
    {
      Key key;
      T   value;
    };

    struct Comparator
    {
      bool operator()(const ArrayItem& item, const Key& key) const
      {
        return item.key < key;
      }
    };

   private:
    Array<ArrayItem> m_Table;

   public:
    SortedArrayTable(IMemoryManager& memory) :
      m_Table{memory}
    {
    }

    void insert(Key key, const T& value)
    {
      const auto it = std::lower_bound(m_Table.begin(), m_Table.end(), key, Comparator{});

      if (it == m_Table.end() || it->key != key)
      {
        m_Table.insert(it, ArrayItem{key, value});
      }
      else
      {
        it->value = value;
      }
    }

    T find(Key key) const
    {
      const auto it = std::lower_bound(m_Table.begin(), m_Table.end(), key, Comparator{});

      if (it == m_Table.end() || it->key != key)
      {
        return T{};
      }

      return it->value;
    }

    // TODO(SR): Add `remove` function when need be.
  };

  struct UIContext
  {
    static constexpr int k_WidgetMemorySize = bfMegabytes(10);

    // Input State

    Vector2f      mouse_pos       = {0.0f, 0.0f};
    bfButtonFlags old_mouse_state = 0x0;
    bfButtonFlags new_mouse_state = 0x0;
    float         delta_time      = 0.0f;

    // Widget Memory

    std::array<char, k_WidgetMemorySize>   widget_freelist_backing = {'\0'};
    FreeListAllocator                      widget_freelist         = {widget_freelist_backing.data(), widget_freelist_backing.size()};
    SortedArrayTable<UIElementID, Widget*> widgets                 = {widget_freelist};

    // State Tracking

    Array<UIElementID> id_stack         = Array<UIElementID>{widget_freelist};
    Array<Widget*>     root_widgets     = Array<Widget*>{widget_freelist};
    Array<Widget*>     root_widgets_old = Array<Widget*>{widget_freelist};
    Widget*            current_widget   = nullptr;

    // Interaction

    std::uint32_t next_zindex     = 1u;
    Widget*       next_hover_root = nullptr;
    Widget*       hovered_widgets = nullptr;
    const Widget* hot_widget      = nullptr;
    const Widget* active_widget   = nullptr;
    Vector2f      drag_offset     = {0.0f, 0.0f};  // MousePos - WidgetPos
  };

}  // namespace bf

namespace bf::UI
{
  static constexpr float k_Inf = FLT_MAX;

  static UIContext g_UI = {};

  static IMemoryManager& CurrentAllocator()
  {
    return g_UI.widget_freelist;
  }

  static PainterFont* const TEST_FONT = new PainterFont(CurrentAllocator(), "assets/fonts/Montserrat/Montserrat-Medium.ttf", -20.0f);

  static void BringToFront(Widget* widget)
  {
    widget->zindex = ++g_UI.next_zindex;
  }

  static bool IsFocusedWindow(const Widget* widget)
  {
    return !g_UI.root_widgets.isEmpty() && g_UI.root_widgets.back() == widget;
  }

  static Rect2f WidgetBounds(const Widget* widget)
  {
    return Rect2f(
     widget->position_from_parent,
     widget->position_from_parent + widget->realized_size);
  }

  static void SetZIndexContainer(Widget* widget)
  {
    if (WidgetBounds(widget).intersects(g_UI.mouse_pos) &&
        (!g_UI.next_hover_root || g_UI.next_hover_root->zindex <= widget->zindex))
    {
      g_UI.next_hover_root = widget;
    }
  }

  static float& WidgetParam(Widget* widget, WidgetParams param)
  {
    return widget->params[int(param)];
  }

  static float MutateFloat(float value, float delta)
  {
    if (value != k_Inf)
    {
      value += delta;
    }

    return value;
  }

  static float RealizeSizeUnit(const SizeUnit& su, float parent_size, float flex_size)
  {
    // clang-format off
    switch (su.type)
    {
      case SizeUnitType::Absolute: return su.value;
      case SizeUnitType::Relative: return parent_size * su.value;
      case SizeUnitType::Flex:     return flex_size;
      bfInvalidDefaultCase();
    }
    // clang-format on

    return {0.0f};
  }

  //
  // Small little article on the words Actualize vs Realize:
  //   [https://cohering.net/blog/2010/09/realization_vs_actualization.html]
  // I could be using the wrong one here but the differnce is subtle and
  // not of much importance since there is no gramatical incorrectness.
  //
  // Honorabled Mentions: "crystalize" and "materialize". :)
  //

  static Vector2f RealizeSize(const Widget* widget, const LayoutConstraints& constraints)
  {
    // assert(widget->parent && "Only windows have no parent and they do not call this function."); @Temp

    const Vector2f parent_size = widget->parent ? widget->parent->realized_size : Vector2f{0.0f, 0.0f};
    const Size&    size        = widget->desired_size;

    return {
     RealizeSizeUnit(size.width, parent_size.x, constraints.max_size.x),
     RealizeSizeUnit(size.height, parent_size.y, constraints.max_size.y),
    };
  }

  static LayoutOutput WidgetDoLayout(Widget* widget, const LayoutConstraints& constraints)
  {
    const auto&  layout = widget->layout;
    LayoutOutput layout_result;

    switch (layout.type)
    {
        // Single Child Layouts

      case LayoutType::Default:
      {
        widget->ForEachChild([&constraints](Widget* child) {
          WidgetDoLayout(child, constraints);
        });

        layout_result.desired_size = RealizeSize(widget, constraints);
        break;
      }
      case LayoutType::Padding:
      {
        const float padding = WidgetParam(widget, WidgetParams::Padding);

        LayoutConstraints child_constraints;

        child_constraints.max_size.x = MutateFloat(constraints.max_size.x, -padding * 2.0f);
        child_constraints.max_size.y = MutateFloat(constraints.max_size.y, -padding * 2.0f);
        child_constraints.min_size   = vec::min(constraints.min_size, child_constraints.max_size);

        Vector2f max_child_size = child_constraints.min_size;

        widget->ForEachChild([&max_child_size, &child_constraints](Widget* child) {
          const auto child_layout = WidgetDoLayout(child, child_constraints);

          max_child_size = vec::max(max_child_size, child_layout.desired_size);
        });

        layout_result.desired_size = max_child_size + Vector2f{padding * 2.0f};
        break;
      }
      case LayoutType::Fixed:
      {
        const float padding = 0.0f;

        LayoutConstraints child_constraints = constraints;

        child_constraints.min_size   = child_constraints.min_size;
        child_constraints.max_size.x = MutateFloat(constraints.max_size.x, -padding * 2.0f);
        child_constraints.max_size.y = MutateFloat(constraints.max_size.y, -padding * 2.0f);

        widget->ForEachChild([&layout_result, &child_constraints](Widget* child) {
          WidgetDoLayout(child, child_constraints);
        });

        layout_result.desired_size = constraints.max_size;
        break;
      }

        // Multi-Child Layouts

      case LayoutType::Row:
      {
        layout_result.desired_size.x = 0.0f;
        layout_result.desired_size.y = constraints.min_size.y;

        LayoutConstraints child_constraints = {};

        child_constraints.min_size   = constraints.min_size;
        child_constraints.max_size   = constraints.max_size;
        child_constraints.min_size.x = 0.0f;
        child_constraints.max_size.x = constraints.max_size.x;

        float total_flex_factor = 0.0f;

        widget->ForEachChild([&layout_result, &total_flex_factor, &child_constraints](Widget* child) {
          if (child->desired_size.width.type != SizeUnitType::Flex)
          {
            LayoutOutput child_size = WidgetDoLayout(child, child_constraints);

            layout_result.desired_size.x += child_size.desired_size.x;
            layout_result.desired_size.y = std::max(layout_result.desired_size.y, child_size.desired_size.y);
          }
          else
          {
            total_flex_factor += child->desired_size.width.value;
          }
        });

        if (total_flex_factor > 0.0f)
        {
          float flex_space_unit = std::max(constraints.max_size.x - layout_result.desired_size.x, 0.0f) / total_flex_factor;

          widget->ForEachChild([flex_space_unit, &constraints, &layout_result](Widget* child) {
            if (child->desired_size.width.type == SizeUnitType::Flex)
            {
              float             child_width      = flex_space_unit * child->desired_size.width.value;
              LayoutConstraints flex_constraints = {
               {child_width, 0.0f},
               {child_width, constraints.max_size.y},
              };
              LayoutOutput child_size = WidgetDoLayout(child, flex_constraints);

              layout_result.desired_size.x += child_size.desired_size.x;
              layout_result.desired_size.y = std::max(layout_result.desired_size.y, child_size.desired_size.y);

              if (child_size.desired_size.y == k_Inf)
              {
                child_size.desired_size.y = layout_result.desired_size.y;
              }
              else
              {
                layout_result.desired_size.y = std::max(layout_result.desired_size.y, child_size.desired_size.y);
              }

              child->realized_size = child_size.desired_size;
            }
          });
        }
        break;
      }
      case LayoutType::Column:
      {
        layout_result.desired_size.x = constraints.min_size.x;
        layout_result.desired_size.y = 0.0f;

        LayoutConstraints my_constraints = {};

        my_constraints.min_size   = constraints.min_size;
        my_constraints.max_size   = constraints.max_size;
        my_constraints.min_size.y = 0.0f;
        my_constraints.max_size.y = constraints.max_size.y;

        float total_flex_factor = 0.0f;

        widget->ForEachChild([&layout_result, &total_flex_factor, &my_constraints](Widget* child) {
          if (child->desired_size.height.type != SizeUnitType::Flex)
          {
            LayoutOutput child_size = WidgetDoLayout(child, my_constraints);

            layout_result.desired_size.x = std::max(layout_result.desired_size.x, child_size.desired_size.x);
            layout_result.desired_size.y += child_size.desired_size.y;
          }
          else
          {
            total_flex_factor += child->desired_size.height.value;
          }
        });

        if (total_flex_factor > 0.0f)
        {
          float flex_space_unit = std::max(constraints.max_size.y - layout_result.desired_size.y, 0.0f) / total_flex_factor;

          widget->ForEachChild([flex_space_unit, &constraints, &layout_result](Widget* child) {
            if (child->desired_size.height.type == SizeUnitType::Flex)
            {
              float             child_height     = flex_space_unit * child->desired_size.height.value;
              LayoutConstraints flex_constraints = {
               {0.0f, child_height},
               {constraints.max_size.x, child_height},
              };
              LayoutOutput child_size = WidgetDoLayout(child, flex_constraints);

              layout_result.desired_size.x = std::max(layout_result.desired_size.x, child_size.desired_size.x);
              layout_result.desired_size.y += child_size.desired_size.y;

              if (child_size.desired_size.x == k_Inf)
              {
                child_size.desired_size.x = layout_result.desired_size.x;
              }
              else
              {
                layout_result.desired_size.x = std::max(layout_result.desired_size.x, child_size.desired_size.x);
              }

              child->realized_size = child_size.desired_size;
            }
          });
        }
        break;
      }
      case LayoutType::Grid:
      {
        assert(false);

        break;
      }
      case LayoutType::Stack:
      {
        assert(false);
        break;
      }
      case LayoutType::Custom:
      {
        layout_result = layout.custom.layout(widget, constraints);
        break;
      }

        bfInvalidDefaultCase();
    }

    // widget->realized_size = vec::min(vec::max(constraints.min_size, layout_result.desired_size), constraints.max_size);
    widget->realized_size = layout_result.desired_size;

    return layout_result;
  }

  //
  // Widget Positioning is Separate From the Layout
  // Since Positioning requires knowledge of one own
  // `Widget::position_from_parent` to be relative to.
  //
  // When you do it within the `WidgetDoLayout` function
  // there is a noticable frame delay of the children not
  // keeping up with parents when quick motion happens.
  //

  static void WidgetDoLayoutPositioning(Widget* widget)
  {
    const auto& layout = widget->layout;

    switch (layout.type)
    {
        // Single Child Layouts

      case LayoutType::Default:
      {
        break;
      }
      case LayoutType::Padding:
      {
        const float padding = WidgetParam(widget, WidgetParams::Padding);

        widget->ForEachChild([widget, position_offset = Vector2f{padding}](Widget* child) {
          child->position_from_parent = widget->position_from_parent + position_offset;
          WidgetDoLayoutPositioning(child);
        });
        break;
      }
      case LayoutType::Fixed:
      {
        const float padding = 0.0f;

        widget->ForEachChild([widget, position_offset = Vector2f{padding}](Widget* child) {
          child->position_from_parent = widget->position_from_parent + position_offset;
          WidgetDoLayoutPositioning(child);
        });
        break;
      }

        // Multi-Child Layouts

      case LayoutType::Row:
      {
        float current_x = widget->position_from_parent.x;

        widget->ForEachChild([&current_x, widget](Widget* child) {
          child->position_from_parent.x = current_x;
          child->position_from_parent.y = widget->position_from_parent.y;
          WidgetDoLayoutPositioning(child);

          current_x += child->realized_size.x;
        });
        break;
      }
      case LayoutType::Column:
      {
        float current_y = widget->position_from_parent.y;

        widget->ForEachChild([&current_y, widget](Widget* child) {
          child->position_from_parent.x = widget->position_from_parent.x;
          child->position_from_parent.y = current_y;
          WidgetDoLayoutPositioning(child);

          current_y += child->realized_size.y;
        });
        break;
      }
      case LayoutType::Grid:
      {
        assert(false);

        break;
      }
      case LayoutType::Stack:
      {
        assert(false);
        break;
      }
      case LayoutType::Custom:
      {
        layout.custom.position_children(widget);
        break;
      }

        bfInvalidDefaultCase();
    }
  }

  static void WidgetDoRender(Widget* self, CommandBuffer2D& gfx2D)
  {
    self->render(self, gfx2D);
  }

  static void DefaultRender(Widget* self, CommandBuffer2D& gfx2D)
  {
    Brush* font_brush = gfx2D.makeBrush(TEST_FONT);

    Brush* beige_brush = gfx2D.makeBrush(BIFROST_COLOR_BEIGE);
    Rect2f main_rect   = {self->position_from_parent.x, self->position_from_parent.y, self->realized_size.x, self->realized_size.y};

    if (self->flags & Widget::IsWindow)
    {
      Brush* burlywood_brush = gfx2D.makeBrush(BIFROST_COLOR_BURLYWOOD);
      Brush* brown_brush     = gfx2D.makeBrush(bfColor4f_fromColor4u(bfColor4u_fromUint32(BIFROST_COLOR_BURLYWOOD)),
                                           bfColor4f_fromColor4u(bfColor4u_fromUint32(BIFROST_COLOR_BROWN)));

      bfQuaternionf gradient_rot = bfQuaternionf_fromEulerDeg(0.0f, 45.0f, 0.0f);
      Vec3f         rot_right    = bfQuaternionf_rightVec(&gradient_rot);
      Vec3f         rot_up       = bfQuaternionf_upVec(&gradient_rot);

      brown_brush->linear_gradient_data.uv_remap.position = {0.0f, 1.0f};
      brown_brush->linear_gradient_data.uv_remap.x_axis   = {rot_right.x, rot_right.y};
      brown_brush->linear_gradient_data.uv_remap.y_axis   = {rot_up.x, rot_up.y};

      if (IsFocusedWindow(self))
      {
        gfx2D.blurredRect(
         beige_brush,
         main_rect,
         10.0f,
         4.0f);
      }

      if (self->flags & Widget::DrawBackground)
      {
        gfx2D.fillRoundedRect(
         burlywood_brush,
         AxisQuad::make(main_rect),
         5.0f);

        gfx2D.fillRoundedRect(
         brown_brush,
         AxisQuad::make(main_rect.expandedFromCenter(-1.0f)),
         5.0f);
      }
    }
    else if (self->flags & Widget::DrawBackground)
    {
      Brush* peach_brush = gfx2D.makeBrush(BIFROST_COLOR_PEACHPUFF);

      gfx2D.fillRect(beige_brush, AxisQuad::make(main_rect));
      gfx2D.fillRect(peach_brush, AxisQuad::make(main_rect.expandedFromCenter(-1.0f)));
    }

    if (self->flags & Widget::DrawName)
    {
      auto text_cmd = gfx2D.text(font_brush, self->position_from_parent + Vector2f{1.0f, 16.0f}, {self->name, self->name_len});

      text_cmd->position.x = self->position_from_parent.x + (main_rect.width() - text_cmd->bounds_size.x) * 0.5f;
      text_cmd->position.y = self->position_from_parent.y + text_cmd->bounds_size.y;
    }

    self->ForEachChild([&gfx2D](Widget* const child) {
      WidgetDoRender(child, gfx2D);
    });
  }

  static UIElementID CalcID(StringRange name)
  {
    const auto hash_seed    = g_UI.id_stack.back();
    const auto current_hash = hash::addBytes(hash_seed, name.begin(), name.length());

    return current_hash;
  }

  static UIElementID CalcID(UIElementID local_id)
  {
    const auto hash_seed    = g_UI.id_stack.back();
    const auto current_hash = hash::addU64(hash_seed, local_id);

    return current_hash;
  }

  static void PushWidget(Widget* widget)
  {
    if (g_UI.current_widget)
    {
      g_UI.current_widget->AddChild(widget);
    }

    g_UI.current_widget = widget;

    PushID(widget->hash);
  }

  static void AddWidget(Widget* widget)
  {
    PushWidget(widget);
    PopWidget();
  }

  static Widget* CreateWidget(StringRange name, LayoutType layout_type = LayoutType::Default)
  {
    const auto id     = CalcID(name);
    Widget*    widget = g_UI.widgets.find(id);

    if (!widget)
    {
      const std::size_t name_buffer_len = name.length();
      char* const       name_buffer     = (char*)CurrentAllocator().allocate(name_buffer_len + 1u);

      std::strncpy(name_buffer, name.begin(), name_buffer_len);
      name_buffer[name_buffer_len] = '\0';

      widget = CurrentAllocator().allocateT<Widget>();

      widget->layout.type = layout_type;
      widget->name        = name_buffer;
      widget->name_len    = name_buffer_len;
      widget->render      = &DefaultRender;
      widget->hash        = id;

      g_UI.widgets.insert(id, widget);
    }

    assert(widget->hash == id);

    widget->Reset();

    return widget;
  }

  struct WidgetBehaviorResult
  {
    enum
    {
      IsClicked = (1 << 0),
      IsHovered = (1 << 1),
      IsActive  = (1 << 2),
      IsPressed = (1 << 3),
    };

    std::uint8_t flags = 0x0;

    bool Is(std::uint8_t f) const
    {
      return flags & f;
    }
  };

  static bool ClickedDownThisFrame(bfButtonFlags buttons)
  {
    const bool is_mouse_down_old = g_UI.old_mouse_state & buttons;
    const bool is_mouse_down_new = g_UI.new_mouse_state & buttons;

    return (is_mouse_down_old != is_mouse_down_new) && is_mouse_down_new;
  }

  static bool IsActiveWidget(const Widget* widget)
  {
    return g_UI.active_widget == widget;
  }

  static void WidgetsUnderPointHelper(Widget* widget, Widget*& result_list, const Vector2f& point)
  {
    const auto bounds = WidgetBounds(widget);

    if (bounds.intersects(point))
    {
      if (widget->flags & (Widget::Clickable | Widget::BlocksInput))
      {
        widget->hit_test_list = result_list;
        result_list           = widget;
      }

      widget->ForEachChild([&](Widget* child) {
        WidgetsUnderPointHelper(child, result_list, point);
      });
    }
  }

  static Widget* WidgetsUnderPoint(const Vector2f& point)
  {
    Widget* result = nullptr;

    for (Widget* window : g_UI.root_widgets_old)
    {
      WidgetsUnderPointHelper(window, result, point);
    }

    return result;
  }

  static WidgetBehaviorResult WidgetBehavior(const Widget* widget)
  {
    WidgetBehaviorResult result          = {};
    const bool           button_released = !(g_UI.new_mouse_state & BIFROST_BUTTON_LEFT) &&
                                 (g_UI.old_mouse_state & BIFROST_BUTTON_LEFT);

    if (g_UI.hovered_widgets == widget)
    {
      result.flags |= WidgetBehaviorResult::IsHovered;

      if (g_UI.new_mouse_state & BIFROST_BUTTON_LEFT)
      {
        result.flags |= WidgetBehaviorResult::IsPressed;
      }
    }

    if (widget->flags & Widget::Clickable)
    {
      if (result.Is(WidgetBehaviorResult::IsHovered))
      {
        if (button_released && IsActiveWidget(widget))
        {
          result.flags |= WidgetBehaviorResult::IsClicked;
        }

        if (ClickedDownThisFrame(BIFROST_BUTTON_LEFT))
        {
          g_UI.active_widget = widget;
          g_UI.drag_offset   = g_UI.mouse_pos - widget->position_from_parent;
        }
      }

      if (g_UI.active_widget == widget && button_released)
      {
        g_UI.active_widget = nullptr;
      }
    }

    if (IsActiveWidget(widget))
    {
      result.flags |= WidgetBehaviorResult::IsActive;
    }

    return result;
  }

  UIElementID PushID(UIElementID local_id)
  {
    const auto current_hash = CalcID(local_id);

    g_UI.id_stack.push(current_hash);

    return current_hash;
  }

  UIElementID PushID(StringRange string_value)
  {
    const auto current_hash = CalcID(string_value);

    g_UI.id_stack.push(current_hash);

    return current_hash;
  }

  void PopID()
  {
    g_UI.id_stack.pop();
  }

  static Widget* CreateButton(const char* name, const Size& size)
  {
    Widget* const button = CreateWidget(name);

    button->desired_size = size;
    button->flags |= Widget::DrawName | Widget::Clickable;

    return button;
  }

  bool BeginWindow(const char* title)
  {
    Widget* const window = CreateWidget(title, LayoutType::Column);

    window->flags |= Widget::BlocksInput | Widget::IsWindow | Widget::DrawBackground;

    g_UI.root_widgets.push(window);

    SetZIndexContainer(window);

    PushID(window->hash);

    PushWidget(window);

    PushPadding(5.0f);
    PushColumn();

    Widget* const titlebar = CreateWidget("__WindowTitlebar__", LayoutType::Row);

    titlebar->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    titlebar->desired_size.height = {SizeUnitType::Absolute, 25.0f};
    titlebar->flags |= Widget::Clickable | Widget::DrawBackground;

    const auto behavior = WidgetBehavior(titlebar);

    if (behavior.Is(WidgetBehaviorResult::IsActive))
    {
      window->position_from_parent = g_UI.mouse_pos - g_UI.drag_offset;
    }

    PushWidget(titlebar);
    {
      Widget* const title_spacing = CreateWidget(title);

      title_spacing->desired_size.width  = {SizeUnitType::Flex, 1.0f};
      title_spacing->desired_size.height = titlebar->desired_size.height;
      title_spacing->flags |= Widget::DrawName;

      AddWidget(title_spacing);

      Widget* const x_button = CreateButton(
       window->flags & Widget::IsExpanded ? "C" : "O",
       {titlebar->desired_size.height, titlebar->desired_size.height});

      x_button->flags |= Widget::DrawBackground;

      AddWidget(x_button);

      const auto behavior = WidgetBehavior(x_button);

      if (behavior.flags & WidgetBehaviorResult::IsClicked)
      {
        window->flags ^= Widget::IsExpanded;  // Toggle

        window->realized_size.y = 0.0f;
      }
    }
    PopWidget();

    const bool is_expanded = window->flags & Widget::IsExpanded;

    PushPadding(is_expanded ? 5.0f : 0.0f);
    PushColumn();

    if (!is_expanded)
    {
      EndWindow();
    }

    return is_expanded;
  }

  void EndWindow()
  {
    PopWidget();  // Column
    PopWidget();  // Padding
    PopWidget();  // Column
    PopWidget();  // Padding
    PopWidget();
    PopID();
  }

  bool Button(const char* name)
  {
    Widget* const button = CreateButton(
     name,
     {{SizeUnitType::Flex, 1.0f}, {SizeUnitType::Absolute, 40.0f}});

    button->flags |= Widget::DrawName | Widget::Clickable;

    const auto behavior = WidgetBehavior(button);

    AddWidget(button);

    const bool  is_hovered = behavior.Is(WidgetBehaviorResult::IsHovered);
    const float hover_time = WidgetParam(button, WidgetParams::HoverTime) +
                             (is_hovered ? +g_UI.delta_time : -g_UI.delta_time);

    WidgetParam(button, WidgetParams::HoverTime) = math::clamp(0.0f, hover_time, 1.0f);

    button->render = [](Widget* self, CommandBuffer2D& gfx2D) {
      const float max_hover_time    = 0.3f;
      const float hover_lerp_factor = math::clamp(0.0f, WidgetParam(self, WidgetParams::HoverTime) / max_hover_time, 1.0f);

      const bfColor4u  normal_color = bfColor4u_fromUint32(BIFROST_COLOR_AQUAMARINE);
      const bfColor4u  hover_color  = bfColor4u_fromUint32(BIFROST_COLOR_CYAN);
      const bfColor32u final_color  = bfColor4u_toUint32(bfMathLerpColor4u(normal_color, hover_color, hover_lerp_factor));

      Brush* button_brush = gfx2D.makeBrush(final_color);
      Brush* font_brush   = gfx2D.makeBrush(TEST_FONT);

      Rect2f rect = {self->position_from_parent.x, self->position_from_parent.y + 3.0f, self->realized_size.x, self->realized_size.y - 6.0f};

      gfx2D.blurredRect(button_brush, rect, 3.0f, 3.0f);
      gfx2D.fillRoundedRect(button_brush, AxisQuad::make(rect), 3.0f);

      auto text_cmd = gfx2D.text(font_brush, {}, {self->name, self->name_len});

      text_cmd->position.x = self->position_from_parent.x + (rect.width() - text_cmd->bounds_size.x) * 0.5f;
      text_cmd->position.y = self->position_from_parent.y + text_cmd->bounds_size.y + (rect.height() - text_cmd->bounds_size.y) * 0.5f;
    };

    return behavior.Is(WidgetBehaviorResult::IsClicked);
  }

  void PushColumn()
  {
    Widget* const widget = CreateWidget("__PushColumn__", LayoutType::Column);

    widget->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    widget->desired_size.height = {SizeUnitType::Flex, 1.0f};

    PushWidget(widget);
  }

  void PushRow()
  {
    Widget* const widget = CreateWidget("__PushRow__", LayoutType::Row);

    widget->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    widget->desired_size.height = {SizeUnitType::Flex, 1.0f};

    PushWidget(widget);
  }

  void PushFixedSize(SizeUnit width, SizeUnit height)
  {
    Widget* const widget = CreateWidget("__FixedSize__", LayoutType::Fixed);

    widget->desired_size = {width, height};

    PushWidget(widget);
  }

  void PushPadding(float value)
  {
    Widget* const widget = CreateWidget("__PADDING__", LayoutType::Padding);

    widget->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    widget->desired_size.height = {SizeUnitType::Flex, 1.0f};

    WidgetParam(widget, WidgetParams::Padding) = value;

    PushWidget(widget);
  }

  void PopWidget()
  {
    PopID();

    g_UI.current_widget = g_UI.current_widget->parent;
  }

  void Init()
  {
    g_UI.id_stack.push(0x0);  // Root ID Seed
  }

  void PumpEvents(bfEvent* event)
  {
    switch (event->type)
    {
      case BIFROST_EVT_ON_MOUSE_DOWN:
      case BIFROST_EVT_ON_MOUSE_UP:
      {
        g_UI.new_mouse_state = event->mouse.button_state;
        break;
      }
      case BIFROST_EVT_ON_MOUSE_MOVE:
      {
        const auto& mouse = event->mouse;

        g_UI.mouse_pos.x = float(mouse.x);
        g_UI.mouse_pos.y = float(mouse.y);

        break;
      }
    }
  }

  void BeginFrame()
  {
    g_UI.hovered_widgets = WidgetsUnderPoint(g_UI.mouse_pos);
  }

  void Update(float delta_time)
  {
    g_UI.delta_time = delta_time;
  }

  void Render(CommandBuffer2D& gfx2D)
  {
    // Test Code

    if (BeginWindow("Test Window"))
    {
      Button("Button 0");
      Button("Button 1");
      Button("Button 2");
      Button("Button 3");
      Button("Button 4");
      Button("Button 5");
      Button("Button 6");
      Button("Button 7");
      Button("Button 8");
      Button("Button 9");
      Button("Button 10");
      Button("Button 11");
      Button("Button 12");
      Button("Button 13");
      Button("Button 14");
      Button("Button 15");
      Button("Button 16");
      Button("Button 17");
      Button("Button 18");
      Button("Button 19");
      Button("Button 20");
      Button("Button 21");
      Button("Button 22");
      Button("Button 23");
      Button("Button 24");

      EndWindow();
    }

    /*
    if (BeginWindow("Test Window"))
    {
      g_UI.current_widget->flags |= Widget::DrawBackground;

      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PushPadding(20.0f);
      if (Button("Hello"))
      {
        std::printf("\nHello was pressed.\n");
      }
      PopWidget();
      PopWidget();

      PushColumn();
      if (Button("Button 2"))
      {
        std::printf("\nButton2 was pressed.\n");
      }

      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PopWidget();

      PushPadding(5.0f);
      if (Button("Button 3"))
      {
        std::printf("\nButton3 was pressed.\n");
      }
      PopWidget();

      PopWidget();

      EndWindow();
    }

    if (BeginWindow("Test Window2"))
    {
      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PushPadding(20.0f);
      if (Button(u8"Hello22222222 \u0210"))
      {
        std::printf("\nHelloffffffff was pressed.\n");
      }
      PopWidget();
      PopWidget();

      PushColumn();
      if (Button("Button 2222222222"))
      {
        std::printf("\nButton2ffffffff was pressed.\n");
      }

      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PopWidget();

      PushPadding(5.0f);
      if (Button("Button 32222222"))
      {
        std::printf("\nButton3fffffffff was pressed.\n");
      }
      PopWidget();

      PopWidget();

      EndWindow();
    }
    */
    assert(g_UI.current_widget == nullptr && "Missing a PopWidget to a corresponding PushWidget.");

    // Test End

    std::stable_sort(
     g_UI.root_widgets.begin(),
     g_UI.root_widgets.end(),
     [](const Widget* a, const Widget* b) {
       return a->zindex < b->zindex;
     });

    // Layout, Position and Render Top Level Widgets

    for (Widget* const window : g_UI.root_widgets)
    {
      LayoutConstraints main_constraints = {
       {320.0f, 0.0f},
       window->realized_size,
      };

      WidgetDoLayout(window, main_constraints);
      WidgetDoLayoutPositioning(window);
      WidgetDoRender(window, gfx2D);
    }

    if (ClickedDownThisFrame(BIFROST_BUTTON_LEFT) &&
        g_UI.next_hover_root &&
        g_UI.next_hover_root->zindex < g_UI.next_zindex)
    {
      BringToFront(g_UI.next_hover_root);
    }
    g_UI.next_hover_root = nullptr;

    // Reset Some State

    g_UI.root_widgets_old.clear();
    std::swap(g_UI.root_widgets, g_UI.root_widgets_old);

    g_UI.old_mouse_state = g_UI.new_mouse_state;
    g_UI.current_widget  = nullptr;
  }

  void ShutDown()
  {
    delete TEST_FONT;
  }
}  // namespace bf::UI
