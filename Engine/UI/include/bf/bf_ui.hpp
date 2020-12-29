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

  // [https://flutter.dev/docs/development/ui/widgets/layout]

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
    LayoutType type = LayoutType::Default;

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
    std::uint64_t  flags                = 0x0;
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

    // Interactable Widgets

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

  enum class RenderBuffer2DCommandType
  {
    // Basic Drawing
    FillRect,
    FillRoundedRect,
    BlurredRect,
    BlurredRoundedRect,
    FillArc,
    Polyline,
    FillTriangles,
    StrokeRect,
    StrokeRoundedRect,
    StrokeArc,
    Text,

    // Special Drawing
    NineSliceRect,
    BlurRegion,

    // Render State Commands
    PushClipRect,
    PopClipRect,
    PushFramebuffer,
    PopFramebuffer,
    PushTransform,
    PopTransform,
    SetBrush,
  };

  struct RenderBuffer2DCommandHeader
  {
    RenderBuffer2DCommandType type;
  };

  struct RenderBuffer2DCommand_FillRect : public RenderBuffer2DCommandHeader
  {
    Rect2f rect;
  };

  struct RenderBuffer2DCommand_FillRoundedRect : public RenderBuffer2DCommandHeader
  {
    Rect2f rect;
    float  top_border_radius;
    float  bottom_border_radius;
    float  left_border_radius;
    float  right_border_radius;
  };

  struct RenderBuffer2DCommand_BlurredRect : public RenderBuffer2DCommand_FillRect
  {
    float shadow_sigma;
  };

  struct RenderBuffer2DCommand_BlurredRoundedRect : public RenderBuffer2DCommand_FillRoundedRect
  {
    float shadow_sigma;
  };

  struct RenderBuffer2DCommand_FillArc : public RenderBuffer2DCommandHeader
  {
    Vector2f position;
    float    radius;
    float    start_angle;
    float    arc_angle;
  };

  struct RenderBuffer2DCommand_Polyline : public RenderBuffer2DCommandHeader
  {
    const Vector2f*   points;
    UIIndexType       num_points;
    float             thickness;
    PolylineJoinStyle join_style;
    PolylineEndStyle  end_style;
    bool              is_overlap_allowed;
  };

  struct RenderBuffer2DCommand_FillTriangles : public RenderBuffer2DCommandHeader
  {
    const Vector2f*    points;
    const UIIndexType* indices;
    UIIndexType        num_points;
    UIIndexType        num_indices;
  };

  using RenderBuffer2DCommand_StrokeRect        = RenderBuffer2DCommand_FillRect;
  using RenderBuffer2DCommand_StrokeRoundedRect = RenderBuffer2DCommand_FillRoundedRect;
  using RenderBuffer2DCommand_StrokeArc         = RenderBuffer2DCommand_FillArc;

  struct RenderBuffer2DCommand_Text : public RenderBuffer2DCommandHeader
  {
    Vector2f     position;
    const char*  utf8_text;
    PainterFont* font;
  };

  struct RenderBuffer2DCommand_NineSliceRect : public RenderBuffer2DCommandHeader
  {
    Rect2f rect;
    float  top_area;
    float  bottom_area;
    float  left_area;
    float  right_area;
  };

  struct RenderBuffer2DCommand_BlurRegion : public RenderBuffer2DCommandHeader
  {
    Rect2f rect;
  };

  struct RenderBuffer2DCommand_PushClipRect : public RenderBuffer2DCommandHeader
  {
    Rect2f region;
  };

  struct RenderBuffer2DCommand_PopClipRect : public RenderBuffer2DCommandHeader
  {
  };

  struct RenderBuffer2DCommand_PushFramebuffer : public RenderBuffer2DCommandHeader
  {
    std::uint16_t framebuffer_width;
    std::uint16_t framebuffer_height;
  };

  struct RenderBuffer2DCommand_PopFramebuffer : public RenderBuffer2DCommandHeader
  {
  };

  struct RenderBuffer2DCommand_PushTransform : public RenderBuffer2DCommandHeader
  {
    Mat4x4 matrix;
    bool   adopt_parent;
  };

  struct RenderBuffer2DCommand_PopTransform : public RenderBuffer2DCommandHeader
  {
  };

  struct Brush
  {
    enum /* Type */
    {
      Color,
      LinearGradient,
      RadialGradient,
      Texture,
    } type;

    union
    {
      bfColor4f color;

      // TODO(SR): The Rest...
    };
  };

  struct RenderBuffer2DCommand_SetBrush : public RenderBuffer2DCommandHeader
  {
    Brush brush;
  };

  // Helper API for an Array of Bytes
  namespace ByteBuffer
  {
    struct Reader
    {
      Array<char>& bytes;
      char*        current_pos;
    };

    template<typename T>
    T* Push(Array<char>& bytes)
    {
      static_assert(std::is_nothrow_destructible_v<T>, "We only support simple types.");

      char* const bytes = bytes.emplaceN(sizeof(T), ArrayEmplaceUninitializedTag{});

      return static_cast<T*>(bytes.emplaceN(sizeof(T)));
    }

    inline Reader BeginRead(Array<char>& bytes)
    {
      Reader result = {bytes, bytes.data()};

      return result;
    }

    template<typename T>
    T* Peek(Reader& reader)
    {
      return static_cast<T*>(reader.current_pos);
    }

    template<typename T>
    T* Read(Reader& reader)
    {
      T* const result = Peek<T>(reader);

      assert((reader.current_pos + sizeof(T)) <= reader.bytes.end());

      reader.current_pos += sizeof(T);

      return result;
    }

  }  // namespace ByteBuffer
}  // namespace bf

#endif /* BF_UI_HPP */
