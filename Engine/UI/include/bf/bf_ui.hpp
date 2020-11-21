//
// Shareef Abdoul-Raheem
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
  enum class SizeUnitType
  {
    Absolute,
    Relative,
    Flex,
  };

  struct SizeUnit
  {
    SizeUnitType type  = SizeUnitType::Absolute;
    float        value = 100.0f;
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

  struct Widget;

  using LayoutFunction = LayoutOutput (*)(Widget* self, const LayoutConstraints& constraints);

  struct WidgetVTable
  {
    LayoutFunction do_layout;
    // LayoutFunction do_render;
    // LayoutFunction do_nav;
  };

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

  enum class WidgetParams
  {
    HoverTime,
    ActiveTime,
    Padding,

    WidgetParams_Max,
  };

  struct Widget : public Hierarchy<Widget>
  {
    enum /* WidgetFlags */
    {
      Clickable          = (1 << 0),
      Disabled           = (1 << 1),
      HoverCurser_Normal = (1 << 2),
      HoverCurser_Hand   = (1 << 3),
      IsLayout           = (1 << 4),
      IsExpanded         = (1 << 5),
      // IsBeingDragged     = (1 << 6),
    };

    using ParamList = float[int(WidgetParams::WidgetParams_Max)];

    const WidgetVTable* vtable               = nullptr;
    const char*         name                 = nullptr;
    std::size_t         name_len             = 0u;
    ParamList           params               = {0.0f};
    Size                desired_size         = {};
    Vector2f            position_from_parent = {200.0f, 200.0f};
    Vector2f            realized_size        = {0.0f, 0.0f};
    std::uint64_t       flags                = Clickable | IsExpanded;
    std::uint64_t       hash                 = 0x0;
  };

  using UIElementID = std::uint64_t;

  namespace UI
  {
    // State Manipulation

    UIElementID PushID(UIElementID local_id);
    UIElementID PushID(StringRange string_value);
    void        PopID();

    // Widgets

    bool BeginWindow(const char* title);
    void EndWindow();
    bool Button(const char* name);

    void PushColumn();
    void PushRow();
    void PushFixedSize(SizeUnit width, SizeUnit height);
    void PushPadding(float value);
    void PopWidget();

    // System API

    void Init();
    void PumpEvents(bfEvent* event);
    void Render(Gfx2DPainter& painter);
  }  // namespace UI
}  // namespace bf

#endif /* BF_UI_HPP */
