//
// Shareef Abdoul-Raheem
//
#ifndef BF_PAINTER_HPP
#define BF_PAINTER_HPP

#include "bf/gfx/bf_render_queue.hpp"                 // RenderQueue
#include "bf/graphics/bifrost_standard_renderer.hpp"  // Math and Graphics

namespace bf
{
  //
  // Forward Declarations
  //

  struct Font;

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
  // Uniform Data
  //

  struct Gfx2DUniformData
  {
    Mat4x4 ortho_matrix;
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
    bfGfxContextHandle            ctx;
    bfGfxDeviceHandle             device;
    bfVertexLayoutSetHandle       vertex_layouts[2];
    bfShaderModuleHandle          vertex_shader;
    bfShaderModuleHandle          fragment_shader;
    bfShaderProgramHandle         shader_program;
    bfShaderModuleHandle          shadow_modules[3];
    bfShaderProgramHandle         rect_shadow_program;
    bfShaderProgramHandle         rounded_rect_shadow_program;
    bfTextureHandle               white_texture;
    int                           num_frame_datas;
    Gfx2DPerFrameRenderData       frame_datas[k_bfGfxMaxFramesDelay];
    MultiBuffer<Gfx2DUniformData> uniform;

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

  struct Gfx2DDrawCommand
  {
    bfTextureHandle texture;
    UIIndexType     first_index;
    UIIndexType     num_indices;

    explicit Gfx2DDrawCommand(bfTextureHandle texture) :
      texture{texture},
      first_index{0u},
      num_indices{0u}
    {
    }
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

  //
  // 2D Render Commands
  //
  //   * The section of filed marked 'User Parameters' can be changed after adding the command to the buffer
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

  struct BaseRender2DCommand_
  {
    // Internal Command State

    Render2DCommandType type;
    std::uint32_t       size;

    // User Parameters

    const Brush* brush;

    BaseRender2DCommand_(Render2DCommandType type, std::uint32_t size) :
      type{type},
      size{size},
      brush{nullptr}
    {
    }

    bool isBlurred() const
    {
      return type == Render2DCommandType::BlurredRect;
    }
  };

  template<Render2DCommandType k_Type, typename TDerived>
  struct BaseRender2DCommand : public BaseRender2DCommand_
  {
    BaseRender2DCommand() :
      BaseRender2DCommand_{k_Type, std::uint32_t(sizeof(TDerived))}
    {
    }
  };

#define DECLARE_COMMAND(name) struct Render2D##name : public BaseRender2DCommand<Render2DCommandType::name, Render2D##name>

  DECLARE_COMMAND(FillRect)
  {
    // User Parameters

    AxisQuad rect;
  };

  DECLARE_COMMAND(FillRoundedRect)
  {
    // User Parameters

    AxisQuad rect;
    float    border_radius;  //!< Invariant: must be <= min(rect.width, rect.height).
  };

  DECLARE_COMMAND(BlurredRect)
  {
    // User Parameters

    Rect2f rect;
    float  border_radii[4]; /*!< top-left, top-right, bottom-right, bottom-left */
    float  shadow_sigma;
  };

  DECLARE_COMMAND(NineSliceRect)
  {
    // User Parameters

    AxisQuad rect;
    float    border_area[4]; /*!< top, bottom, left, right */
  };

  DECLARE_COMMAND(FillArc)
  {
    // User Parameters

    Vector2f position;
    float    radius;  // Invariant: Must be greater than zero.
    float    start_angle;
    float    arc_angle;  // Invariant: Must be greater than zero.
  };

  DECLARE_COMMAND(Polyline)
  {
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
    // Internal Command State

    Vector2f*    points;
    UIIndexType* indices;
    UIIndexType  num_points;  // Invariant: Must be at least 1.
    UIIndexType  num_indices;
  };

  DECLARE_COMMAND(Text)  // Invariant: The brush type must be Brush::Font
  {
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
  //     with batch compatible `Brush` and not overlapping
  //     non compatible Brushes helps.
  //
  // - Rendering Detail.
  //
  //   * All triangles are emitted in a counter clockwise order.
  //
  struct CommandBuffer2D
  {
    static const UIIndexType k_MemorySize                 = bfKilobytes(400);
    static const UIIndexType k_TempVertexStreamMemorySize = bfMegabytes(5);
    static const UIIndexType k_TempIndexStreamMemorySize  = bfMegabytes(2);

    using AuxMem           = FixedLinearAllocator<k_MemorySize>;
    using CommandStreamMem = FixedLinearAllocator<k_MemorySize>;
    using VertexStreamMem  = FixedLinearAllocator<k_TempVertexStreamMemorySize>;
    using IndexStreamMem   = FixedLinearAllocator<k_TempIndexStreamMemorySize>;

    Gfx2DRenderData  render_data;     //!< Stores GPU Resources.
    AuxMem           aux_memory;      //!< For any intermediate calculations.
    CommandStreamMem command_stream;  //!< Dense stream of `BaseRender2DCommand`s.
    VertexStreamMem  vertex_stream;   //!< For commands that need to pre-calculate their vertices.
    IndexStreamMem   index_stream;    //!< For commands that need to pre-calculate their vertices.
    std::size_t      num_commands;    //!< The number of commands we have.

    CommandBuffer2D(GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics);

    //
    // User API
    //

    // Brush Making //

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

    //
    // Command Buffer Owner API
    //

    void clear()
    {
      aux_memory.clear();
      command_stream.clear();
      vertex_stream.clear();
      index_stream.clear();
      num_commands = 0u;
    }

    struct VertIdxCountResult
    {
      UIVertex2D*  precalculated_vertices = nullptr;
      UIIndexType* precalculated_indices  = nullptr;
      UIIndexType  num_vertices           = 0u;
      UIIndexType  num_indices            = 0u;

      std::pair<UIIndexType, UIVertex2D*> requestVertices(VertexStreamMem& vertex_memory, UIIndexType count)
      {
        const UIIndexType result_offset   = num_vertices;
        UIVertex2D* const result_vertices = static_cast<UIVertex2D*>(vertex_memory.allocate(sizeof(UIVertex2D) * count));

        if (!precalculated_vertices)
        {
          precalculated_vertices = result_vertices;
        }

        num_vertices += count;

        return {
         result_offset,
         result_vertices,
        };
      }

      void pushTriIndex(UIIndexType global_index_offset, IndexStreamMem& index_memory, UIIndexType index0, UIIndexType index1, UIIndexType index2)
      {
        UIIndexType* const indices = static_cast<UIIndexType*>(index_memory.allocate(sizeof(UIIndexType) * 3));

        if (!precalculated_indices)
        {
          precalculated_indices = indices;
        }

        num_indices += 3;

        indices[0] = index0 + global_index_offset;
        indices[1] = index1 + global_index_offset;
        indices[2] = index2 + global_index_offset;
      }
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

    void TEST(RenderQueue& render_queue, const DescSetBind& object_binding = {});

   private:
    static Rect2f      calcCommandBounds(const BaseRender2DCommand_* command);
    VertIdxCountResult calcVertexCount(UIIndexType global_index_offset, const BaseRender2DCommand_* command);
    void               writeVertices(const DestVerts& dest, const BaseRender2DCommand_* command, VertIdxCountResult& counts);
  };

  //
  //
  //

  Vector2f calculateTextSize(StringRange utf8_string, PainterFont* font, UIIndexType& num_codepoints);
  Vector2f calculateTextSize(const char* utf8_text, PainterFont* font);

  struct Gfx2DPainter : private NonCopyMoveable<Gfx2DPainter>
  {
    static const UIIndexType k_TempMemorySize = bfMegabytes(2);

   private:
    Gfx2DRenderData                        render_data;
    Array<UIVertex2D>                      vertices;
    Array<UIIndexType>                     indices;
    Array<DropShadowVertex>                shadow_vertices;
    Array<UIIndexType>                     shadow_indices;
    FixedLinearAllocator<k_TempMemorySize> tmp_memory;
    Array<Gfx2DDrawCommand>                draw_commands;

   public:
    explicit Gfx2DPainter(IMemoryManager& memory, GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics);

    void reset();

    Gfx2DDrawCommand& currentDrawCommand() { return draw_commands.back(); }

    void bindTexture(bfTextureHandle texture);

    void pushRectShadow(float shadow_sigma, const Vector2f& pos, float width, float height, float border_radius, bfColor32u color);
    void pushRect(const Vector2f& pos, float width, float height, bfColor4u color);
    void pushRect(const Vector2f& pos, float width, float height, bfColor32u color = BIFROST_COLOR_PINK);
    void pushFillRoundedRect(const Vector2f& pos, float width, float height, float border_radius, bfColor32u color);
    void pushFilledArc(const Vector2f& pos, float radius, float start_angle, float arc_angle, bfColor32u color);
    void pushFilledCircle(const Vector2f& pos, float radius, bfColor32u color);
    void pushLinedArc(const Vector2f& pos, float radius, float start_angle, float arc_angle, bfColor32u color);
    void pushPolyline(const Vector2f* points, UIIndexType num_points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bfColor32u color, bool is_overlap_allowed = false);
    void pushPolyline(ArrayView<const Vector2f> points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bfColor32u color);
    void pushText(const Vector2f& pos, const char* utf8_text, PainterFont* font);

    void render(bfGfxCommandListHandle command_list, int fb_width, int fb_height);

   private:
    // TODO(SR): Delete Me
    template<typename T>
    struct SafeVertexIndexer
    {
      UIIndexType num_verts;
      T*          verts;
      T&          operator[](UIIndexType index) const noexcept { return verts[index]; }
    };

    template<typename VertexType>
    using RequestVerticesResult = std::pair<UIIndexType, SafeVertexIndexer<VertexType>>;

    RequestVerticesResult<UIVertex2D>       requestVertices(UIIndexType num_verts);
    void                                    pushTriIndex(UIIndexType index0, UIIndexType index1, UIIndexType index2);
    RequestVerticesResult<DropShadowVertex> requestVertices2(UIIndexType num_verts);
    void                                    pushTriIndex2(UIIndexType index0, UIIndexType index1, UIIndexType index2);
  };
}  // namespace bf

#endif  // BF_PAINTER_HPP
