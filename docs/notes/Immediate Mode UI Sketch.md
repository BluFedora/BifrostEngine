# Immediate Mode UI Sketch

## UI Structures

```
struct Vec2
{
  float x, y;
}

enum SizeUnitType
{
  Absolute,
  Relative,
  Flex,
}

struct SizeUnit
{
  SizeType type;
  float    value;
}

struct Size
{
  SizeUnit width, height;
}

struct Rect
{
  Vec2 min;
  Vec2 max;
}

struct LayoutConstraints
{
  Vec2 min_size, max_size;
}

struct LayoutOutput
{
  Vec2 desired_size;
}

enum Nav
{
  Up,
  Down,
  Left,
  Right,
}

struct NavResult
{
  Widget* new_selection;
}

using LayoutFunction     = LayoutOutput (*)(Widget* self, LayoutConstraints constraints);
using LayoutNavigationFn = NavResult (*)(Widget* self, Nav dir);

struct WidgetVTable
{
  LayoutFunction     do_layout;
  LayoutNavigationFn do_nav;
}

enum WidgetParams
{
  HoverTime,
  ActiveTime,

  WidgetParams_Max,
}

using CustomRender = void (*)(const Widget* self);

struct Widget
{
  const WidgetVTable* vtable;
  Widget*             parent;
  Vec2                position_from_parent;
  Size                size;
  Widget*             first_child;
  Widget*             last_child;
  Widget*             prev_sibling;
  Widget*             next_sibling;
  float               params[WidgetParams_Max];
  uint64_t            flags;
  uint64_t            hash;
  CustomRender        render;
  void*               render_user_data;
}

struct UIContext
{
}

```

## Processing Loop

```
void MainUILoop()
{
  UI_Submission();      // Handle Input Here, uses layout from last frame.
  UI_CalculateLayout(); // Use tree from 'UI_Submission' to coordinate layout.
  UI_Render();          // Draw the calculated layout to the screen.
}
```

## Directional Navigation

```

OnNavPress(Dir direction) {
  if (!g_UI.HasWidgets()) {
    return;
  }

  if (!g_UI.selected_item) {
    g_UI.selected_item = GetFirstItemInCurrentWindow(); // Also if no current window select the left most?
  } else {
    LayoutBase* layout = g_UI.selected_item->parent;
    NavResult nav_res  = layout->vtable->do_nav(layout, direction);
    
    // Keep going up the tree untile we hit null to see who wants to handle navigation...

    if (nav_res.new_selection)
    {
      g_UI.selected_item = nav_res.new_selection;
    }
    else // We need to go to a new layout group.
    {
      Vec2 dir_vector = DirectionToVec2(direction);
      float best_dot  = FLT_MIN;

      for (Widget* const widget : g_UI.AllWidgets())
      {
        Vec2 curr_widget_to_new = widget->position_from_parent -  g_UI.selected_item->position_from_parent;
        float curr_dot = dot(dir_vector, curr_widget_to_new);

        if (curr_dot > best_dot)
        {
          best_dot = curr_dot;
        }
      }

      // ... yada yada ... From all the best DotProd widgets choose the closest.
    }
  }
}

```

Some stuff can be double buffered from last frame.
Widget data is not part of that.
