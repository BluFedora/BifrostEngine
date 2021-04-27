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

   private:
    Array<ArrayItem> m_Table;  //!< Sorted on the key from least to greatest.

   public:
    SortedArrayTable(IMemoryManager& memory) :
      m_Table{memory}
    {
    }

    void insert(Key key, const T& value)
    {
      const auto it = search(key);

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
      const auto it = search(key);

      if (it == m_Table.end() || it->key != key)
      {
        return T{};
      }

      return it->value;
    }

    bool remove(Key key)
    {
      const auto it             = search(key);
      const bool has_found_item = it != m_Table.end();

      if (has_found_item)
      {
        m_Table.removeAt(m_Table.indexOf(it));
      }

      return has_found_item;
    }

   private:
    typename Array<ArrayItem>::const_iterator search(Key key) const
    {
      return std::lower_bound(m_Table.begin(), m_Table.end(), key, [](const ArrayItem& item, const Key& key) { return item.key < key; });
    }

    typename Array<ArrayItem>::iterator search(Key key)
    {
      return std::lower_bound(m_Table.begin(), m_Table.end(), key, [](const ArrayItem& item, const Key& key) { return item.key < key; });
    }
  };

  struct UIContext
  {
    static constexpr int k_WidgetMemorySize = bfMegabytes(10);

    using WidgetMemoryBacking = std::array<char, k_WidgetMemorySize>;
    using WidgetTable         = SortedArrayTable<UIElementID, Widget*>;

    // Input State

    Vector2f      mouse_pos       = {0.0f, 0.0f};
    bfButtonFlags old_mouse_state = 0x0;
    bfButtonFlags new_mouse_state = 0x0;
    float         delta_time      = 0.0f;

    // Widget Memory

    WidgetMemoryBacking widget_freelist_backing = {'\0'};
    FreeListAllocator   widget_freelist         = {widget_freelist_backing.data(), widget_freelist_backing.size()};
    WidgetTable         widgets                 = {widget_freelist};

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

    //

    float display_scale = 1.0f;
  };

}  // namespace bf

namespace bf::UI
{
  static UIContext g_UI = {};
}

// THIS IS A MEM LEAK AND A HACK
bf::PainterFont* TEST_FONT = new bf::PainterFont(bf::UI::g_UI.widget_freelist, "assets/fonts/Montserrat/Montserrat-Medium.ttf", -20.0f);

namespace bf::UI
{
  static constexpr float k_Float32Max = FLT_MAX;

  static IMemoryManager& CurrentAllocator()
  {
    return g_UI.widget_freelist;
  }

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

  static float WidgetParam(const Widget* widget, WidgetParams param)
  {
    return widget->params[int(param)];
  }

  static float MutateFloat(float value, float delta)
  {
    if (value != k_Float32Max)
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
      case SizeUnitType::Absolute: return su.value * g_UI.display_scale;
      case SizeUnitType::Relative: return parent_size * su.value;
      case SizeUnitType::Flex:     return flex_size;
      bfInvalidDefaultCase();
    }
    // clang-format on
  }

  //
  // Small little article on the words Actualize vs Realize:
  //   [https://cohering.net/blog/2010/09/realization_vs_actualization.html]
  // I could be using the wrong one here but the difference is subtle and
  // not of much importance since there is no grammatical incorrectness.
  //
  // Honorable Mentions: "crystallize" and "materialize". :)
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

  static constexpr float k_ScrollbarWidth = 20.0f;
  static constexpr float k_ScrollbarPad   = 4.0f;

  static Rect2f WidgetScrollYBounds(const Widget* widget)
  {
    return {widget->position_from_parent.x + widget->realized_size.x - k_ScrollbarWidth, widget->position_from_parent.y, k_ScrollbarWidth, widget->realized_size.y};
  }

  static Rect2f WidgetScrollYDragger(const Widget* widget, const Rect2f& bounds)
  {
    const float scroll_percent   = WidgetParam(widget, WidgetParams::ScrollY);
    const float widget_height    = widget->realized_size.y;
    const float children_height  = widget->children_size.y;
    const float scrollbar_height = bounds.height();
    const float dragger_height   = (widget_height / children_height) * scrollbar_height;
    const float available_area   = scrollbar_height - dragger_height;
    const float dragger_y        = scroll_percent * available_area;

    return {bounds.left() + k_ScrollbarPad, widget->position_from_parent.y + dragger_y, k_ScrollbarWidth - k_ScrollbarPad * 2.0f, dragger_height};
  }

  static float WidgetScrollYOffset(const Widget* widget)
  {
    const float scroll_percent  = WidgetParam(widget, WidgetParams::ScrollY);
    const float widget_height   = widget->realized_size.y;
    const float children_height = widget->children_size.y;

    return (children_height - widget_height) * scroll_percent;
  }

  static void AugmentChildConstraintsForScrollbar(Widget* widget, LayoutConstraints& constraints)
  {
    if (widget->children_size.x > widget->realized_size.x)
    {
      constraints.max_size.y = MutateFloat(constraints.max_size.y, -k_ScrollbarWidth);
      widget->flags |= Widget::NeedsScrollX;
    }
    else
    {
      widget->flags &= ~Widget::NeedsScrollX;
      WidgetParam(widget, WidgetParams::ScrollX) = 0.0f;
    }

    if (widget->children_size.y > widget->realized_size.y)
    {
      constraints.max_size.x = MutateFloat(constraints.max_size.x, -k_ScrollbarWidth);
      widget->flags |= Widget::NeedsScrollY;
    }
    else
    {
      widget->flags &= ~Widget::NeedsScrollY;
      WidgetParam(widget, WidgetParams::ScrollY) = 0.0f;
    }
  }

  static LayoutOutput WidgetDoLayout(Widget* widget, const LayoutConstraints& constraints)
  {
    const auto&  layout = widget->layout;
    LayoutOutput layout_result;
    Vector2f     children_size = {0.0f, 0.0f};

    switch (layout.type)
    {
        // Single Child Layouts

      case LayoutType::Stack:
      {
        LayoutConstraints child_constraints = constraints;

        // child_constraints.min_size = child_constraints.max_size = layout_result.desired_size;

        AugmentChildConstraintsForScrollbar(widget, child_constraints);

        widget->ForEachChild([&child_constraints, &children_size](Widget* child) {
          const LayoutOutput child_layout = WidgetDoLayout(child, child_constraints);

          children_size.x = std::max(children_size.x, child_layout.desired_size.x);
          children_size.y = std::max(children_size.y, child_layout.desired_size.y);
        });

        layout_result.desired_size = RealizeSize(widget, constraints);
        break;
      }
      case LayoutType::Padding:
      {
        const float padding = WidgetParam(widget, WidgetParams::Padding);

        LayoutConstraints child_constraints;

        AugmentChildConstraintsForScrollbar(widget, child_constraints);

        child_constraints.max_size.x = MutateFloat(constraints.max_size.x, -padding * 2.0f);
        child_constraints.max_size.y = MutateFloat(constraints.max_size.y, -padding * 2.0f);
        child_constraints.min_size   = vec::min(constraints.min_size, child_constraints.max_size);

        Vector2f max_child_size = child_constraints.min_size;

        widget->ForEachChild([&max_child_size, &child_constraints](Widget* child) {
          const auto child_layout = WidgetDoLayout(child, child_constraints);

          max_child_size = vec::max(max_child_size, child_layout.desired_size);
        });

        children_size = max_child_size;

        layout_result.desired_size = max_child_size + Vector2f{padding * 2.0f};
        break;
      }
      case LayoutType::Fixed:
      {
        layout_result.desired_size = RealizeSize(widget, constraints);

        LayoutConstraints child_constraints;

        child_constraints.min_size = child_constraints.max_size = layout_result.desired_size;

        AugmentChildConstraintsForScrollbar(widget, child_constraints);

        widget->ForEachChild([&child_constraints, &children_size](Widget* child) {
          const LayoutOutput child_layout = WidgetDoLayout(child, child_constraints);

          children_size = vec::max(children_size, child_layout.desired_size);
        });
        break;
      }

        // Multi-Child Layouts

      case LayoutType::Row:
      {
        layout_result.desired_size.x = 0.0f;
        layout_result.desired_size.y = constraints.min_size.y;

        LayoutConstraints child_constraints;

        child_constraints.min_size   = constraints.min_size;
        child_constraints.max_size   = constraints.max_size;
        child_constraints.min_size.x = 0.0f;
        child_constraints.max_size.x = constraints.max_size.x;

        AugmentChildConstraintsForScrollbar(widget, child_constraints);

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

          widget->ForEachChild([widget, flex_space_unit, &constraints, &layout_result](Widget* child) {
            if (child->desired_size.width.type == SizeUnitType::Flex)
            {
              const float       child_width      = flex_space_unit * child->desired_size.width.value;
              LayoutConstraints flex_constraints = {
               {child_width, 0.0f},
               {child_width, constraints.max_size.y},
              };

              AugmentChildConstraintsForScrollbar(widget, flex_constraints);

              LayoutOutput child_size = WidgetDoLayout(child, flex_constraints);

              layout_result.desired_size.x += child_size.desired_size.x;
              layout_result.desired_size.y = std::max(layout_result.desired_size.y, child_size.desired_size.y);

              if (child_size.desired_size.y == k_Float32Max)
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

          children_size = layout_result.desired_size;
        }
        break;
      }
      case LayoutType::Column:
      {
        layout_result.desired_size.x = constraints.min_size.x;
        layout_result.desired_size.y = 0.0f;

        LayoutConstraints my_constraints;

        my_constraints.min_size   = constraints.min_size;
        my_constraints.max_size   = constraints.max_size;
        my_constraints.min_size.y = 0.0f;
        my_constraints.max_size.y = constraints.max_size.y;

        AugmentChildConstraintsForScrollbar(widget, my_constraints);

        float total_flex_factor = 0.0f;

        widget->ForEachChild([&layout_result, &total_flex_factor, &my_constraints](Widget* child) {
          if (child->desired_size.height.type != SizeUnitType::Flex)
          {
            const LayoutOutput child_size = WidgetDoLayout(child, my_constraints);

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

          widget->ForEachChild([widget, flex_space_unit, &constraints, &layout_result](Widget* child) {
            if (child->desired_size.height.type == SizeUnitType::Flex)
            {
              const float       child_height     = flex_space_unit * child->desired_size.height.value;
              LayoutConstraints flex_constraints = {
               {0.0f, child_height},
               {constraints.max_size.x, child_height},
              };

              AugmentChildConstraintsForScrollbar(widget, flex_constraints);

              LayoutOutput child_size = WidgetDoLayout(child, flex_constraints);

              layout_result.desired_size.x = std::max(layout_result.desired_size.x, child_size.desired_size.x);
              layout_result.desired_size.y += child_size.desired_size.y;

              if (child_size.desired_size.x == k_Float32Max)
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

          children_size = layout_result.desired_size;
        }
        break;
      }
      case LayoutType::Grid:
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

    widget->children_size = children_size;
    widget->realized_size = layout_result.desired_size;

    return layout_result;
  }

  //
  // Final widget positioning is separate from the layout
  // Since Positioning requires knowledge of the parent
  // widget's `Widget::position_from_parent` to be relative to.
  //
  // When you do it within the `WidgetDoLayout` function
  // there is a noticeable frame delay of the children not
  // keeping up with parents when quick motion happens.
  //
  static void WidgetDoLayoutPositioning(Widget* widget)
  {
    const auto& layout   = widget->layout;
    const float offset_y = WidgetScrollYOffset(widget);

    switch (layout.type)
    {
        // Single Child Layouts

      case LayoutType::Stack:
      {
        widget->ForEachChild([pos = widget->position_from_parent, offset_y](Widget* child) {
          child->position_from_parent = pos;
          child->position_from_parent.y -= offset_y;
          WidgetDoLayoutPositioning(child);
        });
        break;
      }
      case LayoutType::Padding:
      {
        const float padding = WidgetParam(widget, WidgetParams::Padding);

        widget->ForEachChild([widget, position_offset = Vector2f{padding}, offset_y](Widget* child) {
          child->position_from_parent = widget->position_from_parent + position_offset;
          child->position_from_parent.y -= offset_y;
          WidgetDoLayoutPositioning(child);
        });
        break;
      }
      case LayoutType::Fixed:
      {
        const float padding = 0.0f;

        widget->ForEachChild([widget, position_offset = Vector2f{padding}, offset_y](Widget* child) {
          child->position_from_parent = widget->position_from_parent + position_offset;
          child->position_from_parent.y -= offset_y;
          WidgetDoLayoutPositioning(child);
        });
        break;
      }

        // Multi-Child Layouts

      case LayoutType::Row:
      {
        float current_x = widget->position_from_parent.x;

        widget->ForEachChild([&current_x, widget, offset_y](Widget* child) {
          child->position_from_parent.x = current_x;
          child->position_from_parent.y = widget->position_from_parent.y;
          child->position_from_parent.y -= offset_y;
          WidgetDoLayoutPositioning(child);

          current_x += child->realized_size.x;
        });
        break;
      }
      case LayoutType::Column:
      {
        float current_y = widget->position_from_parent.y - offset_y;

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

  static Rect2f WidgetRect(const Widget* self)
  {
    return {self->position_from_parent.x, self->position_from_parent.y, self->realized_size.x, self->realized_size.y};
  }

  static void DefaultRender(Widget* self, CommandBuffer2D& gfx2D)
  {
    Brush*       font_brush     = gfx2D.makeBrush(TEST_FONT);
    const Rect2f window_bg_rect = WidgetRect(self);

    Brush* beige_brush = gfx2D.makeBrush(BIFROST_COLOR_BEIGE);
    Rect2f main_rect   = {self->position_from_parent.x, self->position_from_parent.y, self->realized_size.x, self->realized_size.y};

    if (self->flags & Widget::IsWindow)
    {
      Brush* burlywood_brush = gfx2D.makeBrush(BIFROST_COLOR_BURLYWOOD);
      Brush* brown_brush     = gfx2D.makeBrush(bfColor4f_fromColor4u(bfColor4u_fromUint32(BIFROST_COLOR_BURLYWOOD)),
                                           bfColor4f_fromColor4u(bfColor4u_fromUint32(BIFROST_COLOR_BROWN)));

      bfQuaternionf gradient_rot = bfQuaternionf_fromEulerDeg(0.0f, 0.0f, 20.0f);
      Vec3f         rot_right    = bfQuaternionf_right(&gradient_rot);
      Vec3f         rot_up       = bfQuaternionf_up(&gradient_rot);

      brown_brush->linear_gradient_data.uv_remap.position = {0.0f, 1.0f};
      brown_brush->linear_gradient_data.uv_remap.x_axis   = {rot_right.x, rot_right.y};
      brown_brush->linear_gradient_data.uv_remap.y_axis   = {rot_up.x, rot_up.y};

      /*
      if (IsFocusedWindow(self))
      {
        gfx2D.blurredRect(
         beige_brush,
         main_rect,
         4.0f,
         4.0f);
      }
      //*/

      if (self->flags & Widget::DrawBackground)
      {
        Brush* const window_bg_outline  = gfx2D.makeBrush(0xFF2E2529);
        Brush* const window_bg          = gfx2D.makeBrush(0xFF54464C);
        const float  window_border_size = 2.0f;

#if 0
        gfx2D.fillRoundedRect(
         burlywood_brush,
         AxisQuad::make(main_rect),
         5.0f);

        gfx2D.fillRoundedRect(
         brown_brush,
         AxisQuad::make(main_rect.expandedFromCenter(-1.0f)),
         5.0f);
#endif

        gfx2D.fillRect(window_bg_outline, AxisQuad::make(main_rect));
        gfx2D.fillRect(window_bg, AxisQuad::make(main_rect.expandedFromCenter(-window_border_size)));
      }
    }

    if (self->flags & Widget::DrawName)
    {
      auto text_cmd = gfx2D.text(font_brush, {}, {self->name, self->name_len}, g_UI.display_scale);

      text_cmd->position.x = self->position_from_parent.x + (main_rect.width() - text_cmd->bounds_size.x) * 0.5f;
      text_cmd->position.y = self->position_from_parent.y + text_cmd->bounds_size.y + 4.0f;
    }

    if (self->flags & Widget::NeedsScrollY)
    {
      const auto scrollbar_bg_rect  = WidgetScrollYBounds(self);
      const auto scrollbar_dragger  = WidgetScrollYDragger(self, scrollbar_bg_rect);
      const auto scrollbar_bg_brush = gfx2D.makeBrush(0xFF4C4654);
      const auto scrollbar_fg_brush = gfx2D.makeBrush(0xFF707070);

      gfx2D.fillRect(scrollbar_bg_brush, AxisQuad::make(scrollbar_bg_rect));
      gfx2D.fillRect(scrollbar_fg_brush, AxisQuad::make(scrollbar_dragger));
    }

    if (self->flags & Widget::IsWindow)
    {
      gfx2D.pushClipRect({vec::convert<int>(main_rect.min()), vec::convert<int>(main_rect.max())});
    }

    self->ForEachChild([&gfx2D](Widget* const child) {
      WidgetDoRender(child, gfx2D);
    });

    if (self->flags & Widget::IsWindow)
    {
      gfx2D.popClipRect();
    }
  }

  static Widget* CreateWidget(StringRange name, LayoutType layout_type = LayoutType::Stack)
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

      widget->flags |= Widget::IsExpanded;

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
      IsClicked            = (1 << 0),
      IsHovered            = (1 << 1),
      IsActive             = (1 << 2),
      IsPressed            = (1 << 3),
      IsInScrollbarBg      = (1 << 4),
      IsInScrollbarDragger = (1 << 5),
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

      if (widget->flags & Widget::NeedsScrollY)
      {
        const auto scrollbar_bg_rect = WidgetScrollYBounds(widget);
        const auto scrollbar_dragger = WidgetScrollYDragger(widget, scrollbar_bg_rect);

        if (scrollbar_bg_rect.intersects(g_UI.mouse_pos))
        {
          result.flags |= WidgetBehaviorResult::IsInScrollbarBg;
          // g_UI.drag_offset = g_UI.mouse_pos - scrollbar_bg_rect.topLeft();
        }

        if (scrollbar_dragger.intersects(g_UI.mouse_pos))
        {
          result.flags |= WidgetBehaviorResult::IsInScrollbarDragger;

          if (ClickedDownThisFrame(BIFROST_BUTTON_LEFT))
          {
            g_UI.drag_offset = g_UI.mouse_pos - scrollbar_dragger.topLeft();
          }
        }
      }

      if (g_UI.active_widget == widget && button_released)
      {
        g_UI.active_widget = nullptr;
      }

      if (IsActiveWidget(widget))
      {
        result.flags |= WidgetBehaviorResult::IsActive;
      }
    }

    return result;
  }

  bf::PainterFont* xxx_Font()
  {
    return TEST_FONT;
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

  bool BeginWindow(const char* title, WindowState& state)
  {
    Widget* const window = CreateWidget(title, LayoutType::Fixed);

    window->flags |= Widget::BlocksInput | Widget::IsWindow | Widget::DrawBackground | Widget::Clickable;
    window->desired_size                       = state.size;
    WidgetParam(window, WidgetParams::Padding) = 2.0f;

    g_UI.root_widgets.push(window);

    SetZIndexContainer(window);

    PushID(window->hash);

    PushWidget(window);
    PushColumn();

    if (window->flags & Widget::NeedsScrollY)
    {
      const auto window_behavior   = WidgetBehavior(window);
      const auto scrollbar_bg_rect = WidgetScrollYBounds(window);
      const auto scrollbar_dragger = WidgetScrollYDragger(window, scrollbar_bg_rect);

      if (window_behavior.Is(WidgetBehaviorResult::IsInScrollbarBg) && window_behavior.Is(WidgetBehaviorResult::IsClicked))
      {
        //const float current_y = g_UI.mouse_pos.y;

        //WidgetParam(window, WidgetParams::ScrollY) = bfMathRemapf(scrollbar_bg_rect.top(), scrollbar_bg_rect.bottom(), 0.0f, 1.0f, current_y);
      }

      if (window_behavior.Is(WidgetBehaviorResult::IsActive))
      {
        if (window_behavior.Is(WidgetBehaviorResult::IsInScrollbarDragger))
        {
          window->SetFlags(Widget::IsInteractingWithScrollbar);
        }

        if (window->IsFlagSet(Widget::IsInteractingWithScrollbar))
        {
          const float current_y = g_UI.mouse_pos.y;
          const float offset_y  = g_UI.drag_offset.y;
          const float desired_y = current_y - offset_y;

          WidgetParam(window, WidgetParams::ScrollY) = math::clamp(0.0f, bfMathRemapf(scrollbar_bg_rect.top(), scrollbar_bg_rect.bottom() - scrollbar_dragger.height(), 0.0f, 1.0f, desired_y), 1.0f);
        }
      }
      else
      {
        window->UnsetFlags(Widget::IsInteractingWithScrollbar);
      }
    }

    Widget* const titlebar = CreateWidget("__WindowTitleBar__", LayoutType::Row);

    titlebar->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    titlebar->desired_size.height = {SizeUnitType::Absolute, 30.0f};
    titlebar->flags |= Widget::Clickable | Widget::DrawBackground;

    titlebar->render = [](Widget* const titlebar, CommandBuffer2D& gfx2D) {
      const auto rect = WidgetRect(titlebar).expandedFromCenter(-2.0f);

      gfx2D.fillRect(
       gfx2D.makeBrush(0xFFE6E6E6),
       AxisQuad::make(rect));

      //Brush* const        font_brush = gfx2D.makeBrush(TEST_FONT);
      //Render2DText* const text_cmd   = gfx2D.text(font_brush, titlebar->position_from_parent + Vector2f{1.0f, 16.0f}, {titlebar->name, titlebar->name_len});

      //text_cmd->position.x = titlebar->position_from_parent.x + (rect.width() - text_cmd->bounds_size.x) * 0.5f;
      //text_cmd->position.y = titlebar->position_from_parent.y + text_cmd->bounds_size.y;

      titlebar->ForEachChild([&gfx2D](Widget* const child) {
        WidgetDoRender(child, gfx2D);
      });
    };

    if (state.can_be_dragged)
    {
      const auto titlebar_behavior = WidgetBehavior(titlebar);

      if (titlebar_behavior.Is(WidgetBehaviorResult::IsActive))
      {
        const Vector2f titlebar_offset_from_window = titlebar->position_from_parent - vec::convert<float>(state.position);
        const Vector2f new_window_pos              = g_UI.mouse_pos - g_UI.drag_offset - titlebar_offset_from_window;

        state.position = vec::convert<int>(new_window_pos);
      }
    }

    window->position_from_parent = vec::convert<float>(state.position);

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

      AddWidget(x_button);

      x_button->render = [](Widget* const x_button, CommandBuffer2D& gfx2D) {
        const auto rect = WidgetRect(x_button).expandedFromCenter(-2.0f);

        gfx2D.fillRect(
         gfx2D.makeBrush(0xFF4C4CF2),
         AxisQuad::make(rect));

        DefaultRender(x_button, gfx2D);
      };

      const auto x_button_behavior = WidgetBehavior(x_button);

      if (x_button_behavior.flags & WidgetBehaviorResult::IsClicked)
      {
        window->flags ^= Widget::IsExpanded;
      }
    }
    PopWidget();

    const bool is_expanded = window->flags & Widget::IsExpanded;

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
    PopWidget();  // Column
    PopWidget();
    PopID();
  }

  bool Button(const char* name)
  {
    static const float max_hover_time = 0.1f;

    Widget* const button = CreateButton(
     name,
     {{SizeUnitType::Flex, 1.0f}, {SizeUnitType::Absolute, 40.0f}});

    const float hover_lerp_factor = math::clamp(0.0f, WidgetParam(button, WidgetParams::HoverTime) / max_hover_time, 1.0f);

    button->desired_size.height.value += hover_lerp_factor * 6.0f;

    button->flags |= Widget::DrawName | Widget::Clickable;

    const auto behavior = WidgetBehavior(button);

    AddWidget(button);

    const bool  is_hovered = behavior.Is(WidgetBehaviorResult::IsHovered);
    const float hover_time = WidgetParam(button, WidgetParams::HoverTime) +
                             (is_hovered ? +g_UI.delta_time : -g_UI.delta_time);

    WidgetParam(button, WidgetParams::HoverTime) = math::clamp(0.0f, hover_time, max_hover_time);

    button->render = [](Widget* self, CommandBuffer2D& gfx2D) {
      const float hover_lerp_factor = math::clamp(0.0f, WidgetParam(self, WidgetParams::HoverTime) / max_hover_time, 1.0f);

      const bfColor4u  normal_color = bfColor4u_fromUint32(BIFROST_COLOR_AQUAMARINE);
      const bfColor4u  hover_color  = bfColor4u_fromUint32(BIFROST_COLOR_DODGERBLUE);
      const bfColor32u final_color  = bfColor4u_toUint32(bfMathLerpColor4u(normal_color, hover_color, hover_lerp_factor));

      Brush* button_brush       = gfx2D.makeBrush(final_color);
      Brush* button_inner_brush = gfx2D.makeBrush(bfColor4u_toUint32(normal_color));
      Brush* font_brush         = gfx2D.makeBrush(TEST_FONT);

      Rect2f rect = {self->position_from_parent.x, self->position_from_parent.y + 3.0f, self->realized_size.x, self->realized_size.y - 6.0f};

      // gfx2D.blurredRect(button_brush, rect, 3.0f, 3.0f);
      // gfx2D.fillRoundedRect(button_brush, AxisQuad::make(rect), 3.0f);
      gfx2D.fillRect(button_brush, AxisQuad::make(rect));
      gfx2D.fillRect(button_inner_brush, AxisQuad::make(rect.expandedFromCenter(-2.0f)));

      auto text_cmd = gfx2D.text(font_brush, {}, {self->name, self->name_len}, g_UI.display_scale);

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
      default: break;
    }
  }

  void BeginFrame()
  {
    g_UI.hovered_widgets = WidgetsUnderPoint(g_UI.mouse_pos);
    g_UI.display_scale   = bfPlatformGetDPIScale();
  }

  void Update(float delta_time)
  {
    g_UI.delta_time = delta_time;
  }

  void Render(CommandBuffer2D& gfx2D, float screen_width, float screen_height)
  {
#if 1
    // Test Code

    static WindowState s_WinStates[2] = {};

    s_WinStates[0].can_be_dragged = true;

    s_WinStates[1].position.x  = int(screen_width - s_WinStates[1].size.width.value * g_UI.display_scale - 5 * g_UI.display_scale);
    s_WinStates[1].position.y  = int(5 * g_UI.display_scale);
    s_WinStates[1].size.height = {SizeUnitType::Absolute, screen_height - 10.0f * g_UI.display_scale};

    /*
    if (BeginWindow("Buttons Galore", s_WinStates[0]))
    {
      char button_label_buffer[128];

      for (int i = 0; i < 25; ++i)
      {
        string_utils::fmtBuffer(button_label_buffer, sizeof(button_label_buffer), nullptr, "Button(%i)", i);

        if (Button(button_label_buffer))
        {
          std::printf("\nButton %i has been pressed\n", i);
        }
      }

      EndWindow();
    }
    //*/

    //*
    if (BeginWindow("Test Window", s_WinStates[1]))
    {
      g_UI.current_widget->flags |= Widget::DrawBackground;

      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});

      if (Button("Hello"))
      {
        std::printf("\nHello was pressed.\n");
      }

      PopWidget();

      PushColumn();
      if (Button("Button 2"))
      {
        std::printf("\nButton2 was pressed.\n");
      }

      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PopWidget();

      if (Button("Button 3"))
      {
        std::printf("\nButton3 was pressed.\n");
      }

      PopWidget();

      EndWindow();
    }
    //*/

    // Test End
#endif

    assert(g_UI.current_widget == nullptr && "Missing a PopWidget to a corresponding PushWidget.");

    std::stable_sort(
     g_UI.root_widgets.begin(),
     g_UI.root_widgets.end(),
     [](const Widget* a, const Widget* b) {
       return a->zindex < b->zindex;
     });

    // Layout, Position and Render Top Level Widgets

    const LayoutConstraints screen_constraints = {
     {0.0f, 0.0f},
     {screen_width, screen_height},
    };

    for (Widget* const window : g_UI.root_widgets)
    {
      WidgetDoLayout(window, screen_constraints);
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

    // Reset State

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
