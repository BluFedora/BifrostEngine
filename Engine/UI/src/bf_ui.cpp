#include "bf/bf_ui.hpp"

#include "bf/FreeListAllocator.hpp"
#include "bf/Platform.h"
#include "bf/bf_hash.hpp"

#include <algorithm>
#include <array>

namespace bf
{
  template<typename Key, typename T>
  struct SortedArrayTable
  {
    struct ArrayItem
    {
      Key key;
      T   value;
    };

    Array<ArrayItem> table;

    SortedArrayTable(IMemoryManager& memory) :
      table{memory}
    {
    }

    void insert(Key key, const T& value)
    {
      const auto it = std::lower_bound(
       table.begin(), table.end(), key, [](const ArrayItem& b, const Key& key) {
         return b.key < key;
       });

      if (it == table.end() || it->key != key)
      {
        table.insert(it, ArrayItem{key, value});
      }
      else
      {
        it->value = value;
      }
    }

    T find(Key key) const
    {
      const auto it = std::lower_bound(
       table.begin(), table.end(), key, [](const ArrayItem& b, const Key& key) {
         return b.key < key;
       });

      if (it == table.end() || it->key != key)
      {
        return nullptr;
      }

      return it->value;
    }
  };

  struct UIContext
  {
    static constexpr int k_WidgetMemorySize = bfMegabytes(10);

    char            memory_backing[k_WidgetMemorySize * 2] = {'\0'};
    LinearAllocator widget_memory[2] /* Double Buffer */   = {
     {memory_backing, k_WidgetMemorySize},
     {memory_backing + k_WidgetMemorySize, k_WidgetMemorySize},
    };
    int current_memory_buffer = 0;

    // Input State

    Vector2f      mouse_pos       = {0.0f, 0.0f};
    bfButtonFlags old_mouse_state = 0x0;
    bfButtonFlags new_mouse_state = 0x0;

    // Widget Memory

    std::array<char, k_WidgetMemorySize>   widget_freelist_backing = {'\0'};
    FreeListAllocator                      widget_freelist         = {widget_freelist_backing.data(), widget_freelist_backing.size()};
    SortedArrayTable<UIElementID, Widget*> widgets                 = {widget_freelist};

    Array<UIElementID> id_stack = Array<UIElementID>{widget_freelist};  // TODO(SR): Use a different allcaotor.

    Array<Widget*> root_widgets   = Array<Widget*>{widget_freelist};
    Widget*        current_widget = nullptr;
    Widget*        current_window = nullptr;

    // Interaction

    const Widget* hot_widget    = nullptr;
    const Widget* active_widget = nullptr;
    Vector2f      drag_offset   = {0.0f, 0.0f};  // MousePos - WidgetPos
  };

}  // namespace bf

namespace bf::UI
{
  static constexpr float k_Inf = FLT_MAX;

  static UIContext g_UI = {};

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

  static LayoutOutput WidgetDoLayout(Widget* widget, const LayoutConstraints& constraints)
  {
    const auto result = widget->vtable->do_layout(widget, constraints);

    widget->realized_size = widget->vtable->do_layout(widget, constraints).desired_size;

    return result;
  }

  static float RealizeSizeUnit(const SizeUnit& su, float parent_size, float flex_size)
  {
    switch (su.type)
    {
      // clang-format off
      case SizeUnitType::Absolute: return su.value;
      case SizeUnitType::Relative: return parent_size * su.value;
      case SizeUnitType::Flex:     return flex_size;

      bfInvalidDefaultCase();

        // clang-format on
    }

    return {0.0f};
  }

  static Vector2f RealizeSize(const Widget* widget, const LayoutConstraints& constraints)
  {
    const Vector2f parent_size = widget->parent ? widget->parent->realized_size : Vector2f{0.0f};
    const Size&    size        = widget->desired_size;

    return {
     RealizeSizeUnit(size.width, parent_size.x, constraints.max_size.x),
     RealizeSizeUnit(size.height, parent_size.y, constraints.max_size.y),
    };
  }

  static const WidgetVTable k_DefaultVTable = {
   [](Widget* self, const LayoutConstraints& constraints) -> LayoutOutput {
     LayoutOutput result = {};

     self->ForEachChild([&constraints](Widget* child) {
       WidgetDoLayout(child, constraints);
     });

     result.desired_size = RealizeSize(self, constraints);

     return result;
   },
  };

  static const WidgetVTable k_PadVTable = {
   [](Widget* self, const LayoutConstraints& constraints) -> LayoutOutput {
     LayoutOutput result = {};

     const float padding = WidgetParam(self, WidgetParams::Padding);

     LayoutConstraints child_constraints = constraints;

     child_constraints.min_size   = child_constraints.min_size;
     child_constraints.max_size.x = MutateFloat(constraints.max_size.x, -padding * 2.0f);
     child_constraints.max_size.y = MutateFloat(constraints.max_size.y, -padding * 2.0f);

     child_constraints.min_size = vec::min(child_constraints.min_size, child_constraints.max_size);

     Vector2f max_child_size = child_constraints.min_size;

     self->ForEachChild([&max_child_size, &result, &child_constraints](Widget* child) {
       const auto child_layout = WidgetDoLayout(child, child_constraints);

       max_child_size = vec::max(max_child_size, child_layout.desired_size);
     });

     self->ForEachChild([self, position_offset = Vector2f{padding}](Widget* child) {
       child->position_from_parent = self->position_from_parent + position_offset;
     });

     result.desired_size = max_child_size + Vector2f{padding * 2.0f};

     return result;
   },
  };

  static const WidgetVTable k_FixedVTable = {
   [](Widget* self, const LayoutConstraints& constraints) -> LayoutOutput {
     LayoutOutput result = {};

     const float padding = WidgetParam(self, WidgetParams::Padding);

     LayoutConstraints child_constraints = constraints;

     child_constraints.min_size   = child_constraints.min_size;
     child_constraints.max_size.x = MutateFloat(constraints.max_size.x, -padding * 2.0f);
     child_constraints.max_size.y = MutateFloat(constraints.max_size.y, -padding * 2.0f);

     self->ForEachChild([&result, &child_constraints](Widget* child) {
       WidgetDoLayout(child, child_constraints);
     });

     self->ForEachChild([self, position_offset = Vector2f{padding}](Widget* child) {
       child->position_from_parent = self->position_from_parent + position_offset;
     });

     result.desired_size = constraints.max_size;

     return result;
   },
  };

  static const WidgetVTable k_RowVTable = {
   [](Widget* self, const LayoutConstraints& constraints) -> LayoutOutput {
     LayoutOutput result = {};

     result.desired_size.x = 0.0f;
     result.desired_size.y = constraints.min_size.y;

     LayoutConstraints my_constraints = {};

     my_constraints.min_size   = constraints.min_size;
     my_constraints.max_size   = constraints.max_size;
     my_constraints.min_size.x = 0.0f;
     my_constraints.max_size.x = constraints.max_size.x;

     float total_flex_factor = 0.0f;

     self->ForEachChild([&result, &total_flex_factor, &my_constraints](Widget* child) {
       if (child->desired_size.width.type != SizeUnitType::Flex)
       {
         LayoutOutput child_size = WidgetDoLayout(child, my_constraints);

         result.desired_size.x += child_size.desired_size.x;
         result.desired_size.y = std::max(result.desired_size.y, child_size.desired_size.y);
       }
       else
       {
         total_flex_factor += child->desired_size.width.value;
       }
     });

     if (total_flex_factor > 0.0f)
     {
       float flex_space_unit = std::max(constraints.max_size.x - result.desired_size.x, 0.0f) / total_flex_factor;

       self->ForEachChild([flex_space_unit, &constraints, &result](Widget* child) {
         if (child->desired_size.width.type == SizeUnitType::Flex)
         {
           float             child_width      = flex_space_unit * child->desired_size.width.value;
           LayoutConstraints flex_constraints = {
            {child_width, 0.0f},
            {child_width, constraints.max_size.y},
           };
           LayoutOutput child_size = WidgetDoLayout(child, flex_constraints);

           result.desired_size.x += child_size.desired_size.x;
           result.desired_size.y = std::max(result.desired_size.y, child_size.desired_size.y);

           if (child_size.desired_size.y == k_Inf)
           {
             child_size.desired_size.y = result.desired_size.y;
           }
           else
           {
             result.desired_size.y = std::max(result.desired_size.y, child_size.desired_size.y);
           }

           child->realized_size = child_size.desired_size;
         }
       });
     }

     float current_x = self->position_from_parent.x;

     self->ForEachChild([&current_x, self](Widget* child) {
       child->position_from_parent.x = current_x;
       child->position_from_parent.y = self->position_from_parent.y;

       current_x += child->realized_size.x;
     });

     self->realized_size = result.desired_size;

     return result;
   },
  };

  static const WidgetVTable k_ColVTable = {
   [](Widget* self, const LayoutConstraints& constraints) -> LayoutOutput {
     LayoutOutput result = {};

     result.desired_size.x = constraints.min_size.x;
     result.desired_size.y = 0.0f;

     LayoutConstraints my_constraints = {};

     my_constraints.min_size   = constraints.min_size;
     my_constraints.max_size   = constraints.max_size;
     my_constraints.min_size.y = 0.0f;
     my_constraints.max_size.y = constraints.max_size.x;

     float total_flex_factor = 0.0f;

     self->ForEachChild([&result, &total_flex_factor, &my_constraints](Widget* child) {
       if (child->desired_size.height.type != SizeUnitType::Flex)
       {
         LayoutOutput child_size = WidgetDoLayout(child, my_constraints);

         result.desired_size.x = std::max(result.desired_size.x, child_size.desired_size.x);
         result.desired_size.y += child_size.desired_size.y;
       }
       else
       {
         total_flex_factor += child->desired_size.height.value;
       }
     });

     if (total_flex_factor > 0.0f)
     {
       float flex_space_unit = std::max(constraints.max_size.y - result.desired_size.y, 0.0f) / total_flex_factor;

       self->ForEachChild([flex_space_unit, &constraints, &result](Widget* child) {
         if (child->desired_size.height.type == SizeUnitType::Flex)
         {
           float             child_height     = flex_space_unit * child->desired_size.height.value;
           LayoutConstraints flex_constraints = {
            {0.0f, child_height},
            {constraints.max_size.x, child_height},
           };
           LayoutOutput child_size = WidgetDoLayout(child, flex_constraints);

           result.desired_size.x = std::max(result.desired_size.x, child_size.desired_size.x);
           result.desired_size.y += child_size.desired_size.y;

           if (child_size.desired_size.x == k_Inf)
           {
             child_size.desired_size.x = result.desired_size.x;
           }
           else
           {
             result.desired_size.x = std::max(result.desired_size.x, child_size.desired_size.x);
           }

           child->realized_size = child_size.desired_size;
         }
       });
     }

     float current_y = self->position_from_parent.y;

     self->ForEachChild([&current_y, self](Widget* child) {
       child->position_from_parent.x = self->position_from_parent.x;
       child->position_from_parent.y = current_y;

       current_y += child->realized_size.y;
     });

     self->realized_size = result.desired_size;

     return result;
   },
  };

  static IMemoryManager& CurrentAllocator()
  {
    return g_UI.widget_freelist;
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

  static Rect2f WidgetBounds(const Widget* widget)
  {
    return Rect2f(
     widget->position_from_parent,
     widget->position_from_parent + widget->realized_size);
  }

  static void PushWidget(Widget* widget)
  {
    if (g_UI.current_widget)
    {
      g_UI.current_widget->AddChild(widget);
    }

    g_UI.current_widget = widget;
  }

  static Widget* CreateWidget(StringRange name, const WidgetVTable* vtable = &k_DefaultVTable)
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

      widget->vtable   = vtable;
      widget->name     = name_buffer;
      widget->name_len = name_buffer_len;
      widget->hash     = id;

      g_UI.widgets.insert(id, widget);
    }
    else
    {
      assert(widget->hash == id);
    }

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

  static WidgetBehaviorResult WidgetBehavior(const Widget* widget)
  {
    WidgetBehaviorResult result          = {};
    const auto           bounds          = WidgetBounds(widget);
    const bool           button_released = !(g_UI.new_mouse_state & BIFROST_BUTTON_LEFT) &&
                                 (g_UI.old_mouse_state & BIFROST_BUTTON_LEFT);

    if (bounds.intersects(g_UI.mouse_pos))
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

  bool BeginWindow(const char* title)
  {
    Widget* const window = CreateWidget(title, &k_ColVTable);

    PushID(window->hash);

    g_UI.root_widgets.push(window);

    PushWidget(window);

    Widget* const titlebar = CreateWidget("__WindowTitlebar__");

    titlebar->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    titlebar->desired_size.height = {SizeUnitType::Absolute, 20.0f};

    const auto behavior = WidgetBehavior(titlebar);

    if (behavior.Is(WidgetBehaviorResult::IsActive))
    {
      window->position_from_parent = g_UI.mouse_pos - g_UI.drag_offset;
    }

    PushWidget(titlebar);
    PopWidget();

    return window->flags & Widget::IsExpanded;
  }

  void EndWindow()
  {
    PopWidget();
    PopID();
  }

  bool Button(const char* name)
  {
    Widget* const button = CreateWidget(name);

    button->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    button->desired_size.height = {SizeUnitType::Absolute, 20.0f};

    const auto behavior = WidgetBehavior(button);

    PushWidget(button);
    PopWidget();

    return behavior.flags & WidgetBehaviorResult::IsClicked;
  }

  void PushColumn()
  {
    Widget* const widget = CreateWidget("__PushColumn__", &k_ColVTable);

    widget->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    widget->desired_size.height = {SizeUnitType::Flex, 1.0f};

    PushWidget(widget);
  }

  void PushRow()
  {
    Widget* const widget = CreateWidget("__PushRow__", &k_RowVTable);

    widget->desired_size.width  = {SizeUnitType::Flex, 1.0f};
    widget->desired_size.height = {SizeUnitType::Flex, 1.0f};

    PushWidget(widget);
  }

  void PushFixedSize(SizeUnit width, SizeUnit height)
  {
    Widget* const widget = CreateWidget("__FixedSize__", &k_PadVTable);

    widget->desired_size = {width, height};

    PushWidget(widget);
  }

  void PushPadding(float value)
  {
    Widget* const padding = CreateWidget("__PADDING__", &k_PadVTable);

    WidgetParam(padding, WidgetParams::Padding) = value;

    PushWidget(padding);
  }

  void PopWidget()
  {
    g_UI.current_widget = g_UI.current_widget->parent;
  }

  static void RenderWidget(Gfx2DPainter& painter, Widget* const widget)
  {
    painter.pushRect(
     widget->position_from_parent,
     widget->realized_size.x,
     widget->realized_size.y,
     BIFROST_COLOR_BURLYWOOD);

    painter.pushRect(
     widget->position_from_parent + Vector2f{1.0f},
     widget->realized_size.x - 2.0f,
     widget->realized_size.y - 2.0f,
     BIFROST_COLOR_BROWN);

    widget->ForEachChild([&painter](Widget* const child) {
      RenderWidget(painter, child);
    });
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

  void Render(Gfx2DPainter& painter)
  {
    // Test Code

    if (BeginWindow("Test Window"))
    {
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

      PushID("__FIXME__");
      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PopWidget();
      PopID();

      PushID("__FIXME2__");
      PushPadding(5.0f);
      if (Button("Button 3"))
      {
        std::printf("\nButton3 was pressed.\n");
      }
      PopWidget();
      PopID();

      PopWidget();

      EndWindow();
    }

    if (BeginWindow("Test Window2"))
    {
      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PushPadding(20.0f);
      if (Button("Hello"))
      {
        std::printf("\nHelloffffffff was pressed.\n");
      }
      PopWidget();
      PopWidget();

      PushColumn();
      if (Button("Button 2"))
      {
        std::printf("\nButton2ffffffff was pressed.\n");
      }

      PushID("__FIXME__");
      PushFixedSize({SizeUnitType::Flex, 1.0f}, {SizeUnitType::Flex, 1.0f});
      PopWidget();
      PopID();

      PushID("__FIXME2__");
      PushPadding(5.0f);
      if (Button("Button 3"))
      {
        std::printf("\nButton3fffffffff was pressed.\n");
      }
      PopWidget();
      PopID();

      PopWidget();

      EndWindow();
    }

    // Test End

    // Layout

    LayoutConstraints main_constraints = {
     {0.0f, 0.0f},
     // {std::min(g_UI.mouse_pos.x, 600.0f), std::min(g_UI.mouse_pos.y, 500.0f)},
     {600.0f, 500.0f},
    };

    for (Widget* const window : g_UI.root_widgets)
    {
      WidgetDoLayout(window, main_constraints);
    }

    // Render

    for (Widget* const window : g_UI.root_widgets)
    {
      RenderWidget(painter, window);
    }

    // Reset Some State

    g_UI.current_memory_buffer = (g_UI.current_memory_buffer + 1) & 1;
    g_UI.widget_memory[g_UI.current_memory_buffer].clear();

    g_UI.root_widgets.clear();
    g_UI.old_mouse_state = g_UI.new_mouse_state;
    g_UI.current_widget  = nullptr;
  }
}  // namespace bf::UI
