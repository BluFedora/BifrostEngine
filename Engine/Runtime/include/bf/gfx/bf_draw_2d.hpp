/******************************************************************************/
/*!
 * @file   bf_draw_2d.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   API for efficient drawing of `fancy` vector 2D graphics.
 *
 * @version 0.0.1
 * @date    2021-03-03
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_DRAW_2D_HPP
#define BF_DRAW_2D_HPP

#include "bf/LinearAllocator.hpp"                     // FixedLinearAllocator
#include "bf/MemoryUtils.h"                           // bfKilobytes, bfMegabytes
#include "bf/graphics/bifrost_standard_renderer.hpp"  // Math and Graphics (namely the GlslCompiler)

namespace bf
{
  //
  // Forward Declarations
  //

  struct Font;
  struct RenderQueue;
  struct DescSetBind;

  //
  // Type Aliases
  //

  using UIIndexType = std::uint32_t;

  //
  // Vertex Definitions
  //

  struct UIVertex2D
  {
    Vector2f  pos;
    Vector2f  uv;
    bfColor4u color;
  };

  struct DropShadowVertex
  {
    Vector2f  pos;
    float     shadow_sigma;
    float     corner_radius;
    Rect2f    box;
    bfColor4u color;
  };

  //
  // Low Level Graphics Definitions
  //

  struct Gfx2DPerFrameRenderData
  {
    bfBufferHandle vertex_buffer        = nullptr;
    bfBufferHandle index_buffer         = nullptr;
    bfBufferHandle vertex_shadow_buffer = nullptr;
    bfBufferHandle index_shadow_buffer  = nullptr;

    // Size measured in number of bytes
    void reserve(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size);
    void reserveShadow(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size);
  };

  struct Gfx2DRenderData : private NonCopyMoveable<Gfx2DRenderData>
  {
    bfGfxContextHandle      ctx;
    bfGfxDeviceHandle       device;
    bfVertexLayoutSetHandle vertex_layouts[2];
    bfShaderModuleHandle    vertex_shader;
    bfShaderModuleHandle    fragment_shader;
    bfShaderProgramHandle   shader_program;
    bfShaderModuleHandle    shadow_modules[3];
    bfShaderProgramHandle   rect_shadow_program;
    bfShaderProgramHandle   rounded_rect_shadow_program;
    bfTextureHandle         white_texture;
    int                     num_frame_datas;
    Gfx2DPerFrameRenderData frame_datas[k_bfGfxMaxFramesDelay];

    explicit Gfx2DRenderData(GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics);

    // Size measured in number of items.
    void reserve(int index, size_t vertex_size, size_t indices_size);
    void reserveShadow(int index, size_t vertex_size, size_t indices_size);

    ~Gfx2DRenderData();

   private:
    template<typename F>
    void forEachBuffer(F&& fn) const
    {
      for (int i = 0; i < num_frame_datas; ++i)
      {
        fn(frame_datas[i]);
      }
    }
  };

  //
  // High Level Graphics Definitions
  //

  enum class PolylineJoinStyle : std::uint8_t
  {
    MITER,
    BEVEL,
    ROUND,
  };

  enum class PolylineEndStyle : std::uint8_t
  {
    FLAT,
    SQUARE,
    ROUND,
    CONNECTED,
  };

  struct DynamicAtlas
  {
    bfTextureHandle handle;
    bool            needs_upload;
    bool            needs_resize;
  };

  struct PainterFont : NonCopyMoveable<PainterFont>
  {
    bfGfxDeviceHandle device;
    Font*             font;
    DynamicAtlas      gpu_atlas[k_bfGfxMaxFramesDelay];

    PainterFont(IMemoryManager& memory, const char* filename, float pixel_height);
    ~PainterFont();
  };

  // A rotated quad (arbitrary axes, aka not necessarily orthogonal).
  struct AxisQuad
  {
    static AxisQuad make(Vector2f pos = {0.0f, 0.0f}, Vector2f x_axis = {1.0f, 0.0f}, Vector2f y_axis = {0.0f, 1.0f})
    {
      AxisQuad result;

      result.position = pos;
      result.x_axis   = x_axis;
      result.y_axis   = y_axis;

      return result;
    }

    static AxisQuad make(Rect2f rect)
    {
      AxisQuad result;

      result.position = rect.topLeft();
      result.x_axis   = {rect.width(), 0.0f};
      result.y_axis   = {0.0f, rect.height()};

      return result;
    }

    Vector2f position;
    Vector2f x_axis;
    Vector2f y_axis;

    Vector2f v0() const { return position; }
    Vector2f v1() const { return position + x_axis; }
    Vector2f v2() const { return position + x_axis + y_axis; }
    Vector2f v3() const { return position + y_axis; }
    float    width() const { return vec::length(x_axis); }
    float    height() const { return vec::length(y_axis); }

    // offset moves along the axes.
    AxisQuad mutated(Vector2f offset, float new_width, float new_height) const
    {
      AxisQuad result;

      const Vector2f normalized_x_axis = vec::normalized(x_axis);
      const Vector2f normalized_y_axis = vec::normalized(y_axis);

      result.position = position + normalized_x_axis * offset.x + normalized_y_axis * offset.y;
      result.x_axis   = normalized_x_axis * new_width;
      result.y_axis   = normalized_y_axis * new_height;

      return result;
    }

    Vector2f offsetAlongAxes(Vector2f offset) const
    {
      const Vector2f normalized_x_axis = vec::normalized(x_axis);
      const Vector2f normalized_y_axis = vec::normalized(y_axis);

      return position + normalized_x_axis * offset.x + normalized_y_axis * offset.y;
    }

    Rect2f bounds() const
    {
      const Vector2f v0  = this->v0();
      const Vector2f v1  = this->v1();
      const Vector2f v2  = this->v2();
      const Vector2f v3  = this->v3();
      const Vector2f min = vec::min(vec::min(vec::min(v0, v1), v2), v3);
      const Vector2f max = vec::max(vec::max(vec::max(v0, v1), v2), v3);

      return {min, max};
    }
  };

  // A color placed along a linear gradient.
  struct GradientStop
  {
    float     percent;
    bfColor4f value;
  };

  struct BrushSampleResult
  {
    Vector2f  remapped_uv;
    bfColor4f color;
  };

  struct Brush
  {
    enum /* Type */
    {
      Colored,
      LinearGradient,
      NaryLinearGradient,
      Textured,
      Font,

    } type;

    struct ColorBrushData
    {
      bfColor4f value;
    };

    struct LinearGradientBrushData
    {
      bfColor4f colors[2];
      AxisQuad  uv_remap;
    };

    struct NaryLinearGradientBrushData
    {
      GradientStop* colors;      // Invariant: Sorted on GradientStop::percent.
      std::size_t   num_colors;  // Invariant: There are at least 2 colors.
      AxisQuad      uv_remap;
    };

    struct TextureBrushData
    {
      bfTextureHandle texture;  // Invariant: Is not null.
      bfColor4f       tint;
      AxisQuad        uv_remap;
    };

    struct FontBrushData
    {
      PainterFont* font;  // Invariant: Is not null.
      bfColor4f    tint;
    };

    union
    {
      ColorBrushData              colored_data;
      LinearGradientBrushData     linear_gradient_data;
      NaryLinearGradientBrushData nary_linear_gradient_data;
      TextureBrushData            textured_data;
      FontBrushData               font_data;
    };

    ///
    /// <summary>
    /// Returns you a color and the remapped UVs for a certain
    /// normalized coordinate.
    /// </summary>
    ///
    /// <param name="uv">uv is a normalized point you want ot sample that brush at.</param>
    /// <param name="vertex_index">The index of the vertex you are sampling for.</param>
    ///
    /// <code>
    /// (0, 1)                (1, 1)
    ///    +-------------------+
    ///    |                   |
    ///    y                   |
    ///    .                   |
    ///    b                   |
    ///    a    *uv            |
    ///    s                   |
    ///    i                   |
    ///    s                   |
    ///    |                   |
    ///    +-----x.basis-------+
    /// (0, 0)                (1, 0)
    /// </code>
    ///
    BrushSampleResult sample(Vector2f uv, UIIndexType vertex_index) const;

    //
    // The property of being compatible with a batch is transitive.
    // EX: if
    //       BrushA.canBeBatchedWith(BrushB) && BrushB.canBeBatchedWith(BrushC)
    //     => BrushA.canBeBatchedWith(BrushC)
    //
    bool canBeBatchedWith(const Brush& rhs) const
    {
      if (isVertexColorBased() && rhs.isVertexColorBased())
      {
        return true;
      }

      if (type == rhs.type)
      {
        if (type == Textured) { return textured_data.texture == rhs.textured_data.texture; }
        if (type == Font) { return font_data.font == rhs.font_data.font; }
      }

      return false;
    }

    bool isVertexColorBased() const
    {
      return type == Colored ||
             type == LinearGradient ||
             type == NaryLinearGradient;
    }
  };

  struct ClipRect
  {
    Rect2i    rect;
    ClipRect* prev;  //!< The clip rect to set when popping.
  };

  //
  // 2D Render Commands
  //
  //   * The section of fields marked 'User Parameters' can be changed after adding the command to the buffer
  //     while the 'Internal Command State' section should not be written to (although reading is ok).
  //

  enum class Render2DCommandType : std::uint32_t
  {
    FillRect,
    FillRoundedRect,
    BlurredRect,
    NineSliceRect,
    FillArc,
    Polyline,
    FillTriangles,
    Text,
  };

  struct BaseRender2DCommand
  {
    // Internal Command State

    Render2DCommandType type;
    std::uint32_t       size;
    const ClipRect*     clip_rect;

    // User Parameters

    const Brush* brush;

    BaseRender2DCommand(Render2DCommandType type, std::uint32_t size, const ClipRect* clip_rect, const Brush* brush) :
      type{type},
      size{size},
      clip_rect{clip_rect},
      brush{brush}
    {
    }

    bool isBlurred() const { return type == Render2DCommandType::BlurredRect; }

    bool canBeBatchedWith(const BaseRender2DCommand& rhs) const
    {
      return isBlurred() == rhs.isBlurred() &&
             brush->canBeBatchedWith(*rhs.brush) &&
             clip_rect == rhs.clip_rect;
    }
  };

  template<Render2DCommandType k_Type, typename TDerived>
  struct BaseRender2DCommandT : public BaseRender2DCommand
  {
    using Base = BaseRender2DCommandT<k_Type, TDerived>;

    BaseRender2DCommandT(const ClipRect* clip_rect_, const Brush* brush_) :
      BaseRender2DCommand{k_Type, std::uint32_t(sizeof(TDerived)), clip_rect_, brush_}
    {
    }
  };

#define DECLARE_COMMAND(name) struct Render2D##name : public BaseRender2DCommandT<Render2DCommandType::name, Render2D##name>

  DECLARE_COMMAND(FillRect)
  {
    using Base::Base;

    // User Parameters

    AxisQuad rect;
  };

  DECLARE_COMMAND(FillRoundedRect)
  {
    using Base::Base;

    // User Parameters

    AxisQuad rect;
    float    border_radius;  //!< Invariant: must be <= min(rect.width, rect.height).
  };

  DECLARE_COMMAND(BlurredRect)
  {
    using Base::Base;

    // User Parameters

    Rect2f rect;
    float  border_radii[4];  //!< top-left, top-right, bottom-right, bottom-left
    float  shadow_sigma;
  };

  DECLARE_COMMAND(NineSliceRect)
  {
    using Base::Base;

    // User Parameters

    AxisQuad rect;
    float    border_area[4]; /*!< top, bottom, left, right */
  };

  DECLARE_COMMAND(FillArc)
  {
    using Base::Base;

    // User Parameters

    Vector2f position;
    float    radius;  // Invariant: Must be greater than zero.
    float    start_angle;
    float    arc_angle;  // Invariant: Must be greater than zero.
  };

  DECLARE_COMMAND(Polyline)
  {
    using Base::Base;

    // Internal Command State

    Vector2f*   points;
    UIIndexType num_points;  // Invariant: Must be at least 2.

    // User Parameters

    float             thickness;
    PolylineJoinStyle join_style;
    PolylineEndStyle  end_style;
    bool              is_overlap_allowed;
  };

  DECLARE_COMMAND(FillTriangles)
  {
    using Base::Base;

    // Internal Command State

    Vector2f*    points;
    UIIndexType* indices;
    UIIndexType  num_points;  // Invariant: Must be at least 1.
    UIIndexType  num_indices;
  };

  DECLARE_COMMAND(Text)  // Invariant: The brush type must be Brush::Font
  {
    using Base::Base;

    // Internal Command State

    Vector2f    bounds_size;
    StringRange utf8_text;
    UIIndexType num_codepoints;

    // User Parameters

    Vector2f position;
  };

#undef DECLARE_COMMAND

  //
  // Holds a list of 2D draw commands for later submission into a RenderQueue.
  //
  // - This command buffer does no culling or sorting -
  //
  //   * It is assumed the order of command submission is back-to-front
  //     and that any culling will happen before calling any function on this.
  //
  // - There is an attempt to efficiently batch up draw commands -
  //
  //   * To make the algorithm work better submitting items
  //     with batch compatible `Brush`es and not overlapping
  //     non compatible Brushes helps.
  //
  // - The clip state is a stack and does not check for redundant pushing
  //   of the same rect so it is the caller's responsibility to not
  //   dumbly set redundant state as it will waste memory and break batches.
  //
  // - Rendering Detail.
  //
  //   * All triangles are emitted in a counter clockwise order.
  //
  struct CommandBuffer2D
  {
   private:
    static const UIIndexType k_CommandStreamMemorySize    = bfKilobytes(150);
    static const UIIndexType k_AuxiliaryMemorySize        = bfKilobytes(200);
    static const UIIndexType k_TempVertexStreamMemorySize = bfMegabytes(5);
    static const UIIndexType k_TempIndexStreamMemorySize  = bfMegabytes(2);

    using AuxMem           = FixedLinearAllocator<k_AuxiliaryMemorySize>;
    using CommandStreamMem = FixedLinearAllocator<k_CommandStreamMemorySize>;
    using VertexStreamMem  = FixedLinearAllocator<k_TempVertexStreamMemorySize>;
    using IndexStreamMem   = FixedLinearAllocator<k_TempIndexStreamMemorySize>;

   private:
    Gfx2DRenderData  render_data;        //!< Stores GPU Resources.
    AuxMem           aux_memory;         //!< For any intermediate calculations.
    CommandStreamMem command_stream;     //!< Dense stream of `BaseRender2DCommand`s.
    VertexStreamMem  vertex_stream;      //!< For commands that need to pre-calculate their vertices.
    IndexStreamMem   index_stream;       //!< For commands that need to pre-calculate their vertices.
    std::size_t      num_commands;       //!< The number of commands we have.
    ClipRect*        current_clip_rect;  //!< Clip Rects are allocated with `aux_memory`, it is a stack.

   public:
    CommandBuffer2D(GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics);

    //
    // User API
    //

    // Brush Making //

    Brush* makeBrush(bfColor32u color);
    Brush* makeBrush(bfColor4f color);
    Brush* makeBrush(bfColor4f color_a, bfColor4f color_b);
    Brush* makeBrushGradient(std::size_t num_gradient_stops);
    Brush* makeBrush(bfTextureHandle texture, bfColor4f tint = {1.0, 1.0f, 1.0f, 1.0f});
    Brush* makeBrush(PainterFont* font, bfColor4f tint = {0.0f, 0.0f, 0.0f, 1.0f});

    // Draw Routines //

    Render2DFillRect*        fillRect(const Brush* brush, AxisQuad rect);
    Render2DFillRoundedRect* fillRoundedRect(const Brush* brush, AxisQuad rect, float border_radius);
    Render2DBlurredRect*     blurredRect(const Brush* brush, Rect2f rect, float shadow_sigma, float border_radius);
    Render2DFillArc*         fillArc(const Brush* brush, Vector2f position, float radius, float start_angle = 0.0f, float arc_angle = k_Tau);
    Render2DPolyline*        polyline(const Brush* brush, const Vector2f* points, UIIndexType num_points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bool is_overlap_allowed = false);
    Render2DText*            text(const Brush* brush, Vector2f position, StringRange utf8_text);

    // State Management //

    //
    // The rect is clipped to the bounds of the screen for you.
    //
    ClipRect* pushClipRect(Rect2i rect);
    void      popClipRect();

    //
    // Command Buffer Owner API
    //

    // After a clear there is a default clip rect of the size passed in.

    void clear(Rect2i default_clip_rect);
    void renderToQueue(RenderQueue& render_queue, const DescSetBind& object_binding);
    void renderToQueue(RenderQueue& render_queue);

    // Returns you the screen bounds that the graphic drawn by
    // the command will take up.
    // Some of the calculations may be expensive depending on the type of command.
    static Rect2f calcCommandBounds(const BaseRender2DCommand* command);

   private:
    struct VertIdxCountResult
    {
      UIVertex2D*  precalculated_vertices = nullptr;
      UIIndexType* precalculated_indices  = nullptr;
      UIIndexType  num_vertices           = 0u;
      UIIndexType  num_indices            = 0u;

      std::pair<UIIndexType, UIVertex2D*> requestVertices(VertexStreamMem& vertex_memory, UIIndexType count);
      void                                pushTriIndex(UIIndexType global_index_offset, IndexStreamMem& index_memory, UIIndexType index0, UIIndexType index1, UIIndexType index2);
    };

    struct DestVerts
    {
      UIVertex2D*       vertex_buffer_ptr;
      UIIndexType*      index_buffer_ptr;
      DropShadowVertex* shadow_vertex_buffer_ptr;
      UIIndexType*      shadow_index_buffer_ptr;
      UIIndexType       vertex_offset;
      UIIndexType       shadow_vertex_offset;
    };

    VertIdxCountResult calcVertexCount(UIIndexType global_index_offset, const BaseRender2DCommand* command);
    void               writeVertices(const DestVerts& dest, const BaseRender2DCommand* command, VertIdxCountResult& counts, const Rect2f& bounds);

    template<typename T>
    T* allocCommand(const Brush* brush)
    {
      return command_stream.allocateT<T>(current_clip_rect, brush);
    }
  };

  //
  // Misc Helpers
  //

  Vector2f calculateTextSize(StringRange utf8_string, PainterFont* font, std::uint32_t* num_codepoints = nullptr);
}  // namespace bf

#endif  // BF_DRAW_2D_HPP

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
