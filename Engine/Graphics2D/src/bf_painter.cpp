//
// Shareef Abdoul-Raheem
//
#include "bf/gfx2D/bf_painter.hpp"

#include "bf/NoFreeAllocator.hpp"  // NoFreeAllocator
#include "bf/Platform.h"           // bfPlatformGetGfxAPI
#include "bf/Text.hpp"             // CodePoint, Font

#include <cmath>

namespace bf
{
  static const bfTextureSamplerProperties k_SamplerNearestClampToEdge = bfTextureSamplerProperties_init(BF_SFM_NEAREST, BF_SAM_CLAMP_TO_EDGE);
  static constexpr bfColor4u              k_ColorWhite4u              = {0xFF, 0xFF, 0xFF, 0xFF};
  static constexpr float                  k_ArcSmoothingFactor        = 3.5f; /*!< This is just about the minimum before quality of the curves degrade. */
  static constexpr std::size_t            k_NumVertRect               = 4;
  static constexpr std::size_t            k_NumIdxRect                = 6;

  void Gfx2DPerFrameRenderData::reserve(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
  {
    if (!vertex_buffer || bfBuffer_size(vertex_buffer) < vertex_size)
    {
      bfGfxDevice_release(device, vertex_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = vertex_size;
      buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      vertex_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }

    if (!index_buffer || bfBuffer_size(index_buffer) < indices_size)
    {
      bfGfxDevice_release(device, index_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = indices_size;
      buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

      index_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }
  }

  void Gfx2DPerFrameRenderData::reserveShadow(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
  {
    if (!vertex_shadow_buffer || bfBuffer_size(vertex_shadow_buffer) < vertex_size)
    {
      bfGfxDevice_release(device, vertex_shadow_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = vertex_size;
      buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      vertex_shadow_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }

    if (!index_shadow_buffer || bfBuffer_size(index_shadow_buffer) < indices_size)
    {
      bfGfxDevice_release(device, index_shadow_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = indices_size;
      buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

      index_shadow_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }
  }

  Gfx2DRenderData::Gfx2DRenderData(GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics) :
    ctx{graphics},
    device{bfGfxContext_device(graphics)},
    vertex_layouts{nullptr, nullptr},
    vertex_shader{nullptr},
    fragment_shader{nullptr},
    shader_program{nullptr},
    shadow_modules{nullptr, nullptr, nullptr},
    rect_shadow_program{nullptr},
    rounded_rect_shadow_program{nullptr},
    white_texture{nullptr},
    num_frame_datas{0},
    frame_datas{},
    uniform{}
  {
    // Vertex Layout
    vertex_layouts[0] = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(vertex_layouts[0], 0, sizeof(UIVertex2D));
    bfVertexLayout_addVertexLayout(vertex_layouts[0], 0, BF_VFA_FLOAT32_2, offsetof(UIVertex2D, pos));
    bfVertexLayout_addVertexLayout(vertex_layouts[0], 0, BF_VFA_FLOAT32_2, offsetof(UIVertex2D, uv));
    bfVertexLayout_addVertexLayout(vertex_layouts[0], 0, BF_VFA_UCHAR8_4_UNORM, offsetof(UIVertex2D, color));

    vertex_layouts[1] = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(vertex_layouts[1], 0, sizeof(DropShadowVertex));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BF_VFA_FLOAT32_2, offsetof(DropShadowVertex, pos));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BF_VFA_FLOAT32_1, offsetof(DropShadowVertex, shadow_sigma));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BF_VFA_FLOAT32_1, offsetof(DropShadowVertex, corner_radius));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BF_VFA_FLOAT32_4, offsetof(DropShadowVertex, box));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BF_VFA_UCHAR8_4_UNORM, offsetof(DropShadowVertex, color));

    // Shaders
    vertex_shader   = glsl_compiler.createModule(device, "assets/shaders/gfx2D/textured.vert.glsl");
    fragment_shader = glsl_compiler.createModule(device, "assets/shaders/gfx2D/textured.frag.glsl");
    shader_program  = gfx::createShaderProgram(device, 4, vertex_shader, fragment_shader, "Graphics2D.Painter");

    bfShaderProgram_addUniformBuffer(shader_program, "u_Set0", k_GfxCameraSetIndex, 0, 1, BF_SHADER_STAGE_VERTEX);
    bfShaderProgram_addImageSampler(shader_program, "u_Texture", k_GfxMaterialSetIndex, 0, 1, BF_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_compile(shader_program);

    shadow_modules[0]           = glsl_compiler.createModule(device, "assets/shaders/gfx2D/drop_shadow.vert.glsl");
    shadow_modules[1]           = glsl_compiler.createModule(device, "assets/shaders/gfx2D/drop_shadow_rect.frag.glsl");
    shadow_modules[2]           = glsl_compiler.createModule(device, "assets/shaders/gfx2D/drop_shadow_rounded_rect.frag.glsl");
    rect_shadow_program         = gfx::createShaderProgram(device, 1, shadow_modules[0], shadow_modules[1], "Graphics2D.ShadowRect");
    rounded_rect_shadow_program = gfx::createShaderProgram(device, 1, shadow_modules[0], shadow_modules[2], "Graphics2D.ShadowRoundedRect");

    bfShaderProgram_addUniformBuffer(rect_shadow_program, "u_Set0", 0, 0, 1, BF_SHADER_STAGE_VERTEX);
    bfShaderProgram_addUniformBuffer(rounded_rect_shadow_program, "u_Set0", 0, 0, 1, BF_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(rect_shadow_program);
    bfShaderProgram_compile(rounded_rect_shadow_program);

    // White Texture
    white_texture = gfx::createTexture(device, bfTextureCreateParams_init2D(BF_IMAGE_FORMAT_R8G8B8A8_UNORM, 1, 1), k_SamplerNearestClampToEdge, &k_ColorWhite4u, sizeof(k_ColorWhite4u));

    // Frame Data
    const auto& frame_info = bfGfxContext_getFrameInfo(ctx);

    num_frame_datas = int(frame_info.num_frame_indices);

    for (int i = 0; i < num_frame_datas; ++i)
    {
      frame_datas[i] = Gfx2DPerFrameRenderData();
    }

    // Uniform Buffer
    const auto device_info = bfGfxDevice_limits(device);

    uniform.create(device, BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_UNIFORM_BUFFER, frame_info, device_info.uniform_buffer_offset_alignment);
  }

  void Gfx2DRenderData::reserve(int index, size_t vertex_size, size_t indices_size)
  {
    if (vertex_size == 0 || indices_size == 0)
    {
      return;
    }

    assert(index < num_frame_datas);

    Gfx2DPerFrameRenderData& frame_data = frame_datas[index];

    frame_data.reserve(
     device,
     vertex_size * sizeof(UIVertex2D),
     indices_size * sizeof(UIIndexType));
  }

  void Gfx2DRenderData::reserveShadow(int index, size_t vertex_size, size_t indices_size)
  {
    if (vertex_size == 0 || indices_size == 0)
    {
      return;
    }

    assert(index < num_frame_datas);

    Gfx2DPerFrameRenderData& frame_data = frame_datas[index];

    frame_data.reserveShadow(
     device,
     vertex_size * sizeof(DropShadowVertex),
     indices_size * sizeof(UIIndexType));
  }

  Gfx2DRenderData::~Gfx2DRenderData()
  {
    uniform.destroy(device);

    forEachBuffer([this](const Gfx2DPerFrameRenderData& data) {
      bfGfxDevice_release(device, data.vertex_buffer);
      bfGfxDevice_release(device, data.index_buffer);
      bfGfxDevice_release(device, data.vertex_shadow_buffer);
      bfGfxDevice_release(device, data.index_shadow_buffer);
    });

    bfGfxDevice_release(device, white_texture);

    bfGfxDevice_release(device, rounded_rect_shadow_program);
    bfGfxDevice_release(device, rect_shadow_program);
    bfGfxDevice_release(device, shadow_modules[2]);
    bfGfxDevice_release(device, shadow_modules[1]);
    bfGfxDevice_release(device, shadow_modules[0]);

    bfGfxDevice_release(device, shader_program);
    bfGfxDevice_release(device, fragment_shader);
    bfGfxDevice_release(device, vertex_shader);
    bfVertexLayout_delete(vertex_layouts[1]);
    bfVertexLayout_delete(vertex_layouts[0]);
  }

  PainterFont::PainterFont(IMemoryManager& memory, const char* filename, float pixel_height) :
    device{nullptr},
    font{makeFont(memory, filename, pixel_height)},
    gpu_atlas{}
  {
    for (auto& texture : gpu_atlas)
    {
      texture.handle       = nullptr;
      texture.needs_upload = false;
      texture.needs_resize = false;
    }
  }

  PainterFont::~PainterFont()
  {
    destroyFont(font);

    if (device)
    {
      for (auto& texture : gpu_atlas)
      {
        bfGfxDevice_release(device, texture.handle);
      }
    }
  }

  static Vector2f remapUV(const AxisQuad& uv_remap, Vector2f uv)
  {
    return {
     vec::inverseLerp(uv_remap.position, uv_remap.position + uv_remap.x_axis, uv),
     vec::inverseLerp(uv_remap.position, uv_remap.position + uv_remap.y_axis, uv),
    };
  }

  BrushSampleResult Brush::sample(Vector2f uv, UIIndexType vertex_index) const
  {
    (void)vertex_index;  // For future use.

    BrushSampleResult result;

    switch (type)
    {
      case Colored:
      {
        result.color       = colored_data.value;
        result.remapped_uv = uv;
        break;
      }
      case LinearGradient:
      {
        result.remapped_uv = remapUV(linear_gradient_data.uv_remap, uv);

        result.color = bfMathLerpColor4f(
         linear_gradient_data.colors[0],
         linear_gradient_data.colors[1],
         math::clamp(0.0f, result.remapped_uv.x, 1.0f));
        break;
      }
      case NaryLinearGradient:
      {
        const auto& gradient = nary_linear_gradient_data;

        result.remapped_uv = remapUV(gradient.uv_remap, uv);

        const GradientStop* const stops_bgn = gradient.colors;
        const GradientStop* const stops_end = stops_bgn + gradient.num_colors;
        const GradientStop* const it        = std::lower_bound(
         stops_bgn,
         stops_end,
         result.remapped_uv.x,
         [](const GradientStop& gradient_stop, float sample_location) {
           return gradient_stop.percent < sample_location;
         });

        if (it == stops_bgn)
        {
          result.color = stops_bgn->value;
        }
        else if (it == stops_end)
        {
          result.color = stops_end[-1].value;
        }
        else
        {
          const float         old_min_lerp      = gradient.uv_remap.position.x;
          const float         old_max_lerp      = gradient.uv_remap.position.x + gradient.uv_remap.x_axis.x;
          const GradientStop* stop_a            = (it - 1);
          const GradientStop* stop_b            = (it - 0);
          const float         new_min_lerp      = stop_a->percent;
          const float         new_max_lerp      = stop_b->percent;
          const bfColor4f     color_a           = stop_a->value;
          const bfColor4f     color_b           = stop_b->value;
          const float         local_lerp_factor = bfMathRemapf(old_min_lerp,
                                                       old_max_lerp,
                                                       new_min_lerp,
                                                       new_max_lerp,
                                                       result.remapped_uv.x);

          result.color = bfMathLerpColor4f(
           color_a,
           color_b,
           local_lerp_factor);
        }

        break;
      }
      case Textured:
      {
        result.color       = textured_data.tint;
        result.remapped_uv = remapUV(textured_data.uv_remap, uv);
        break;
      }
      case Font:
      {
        result.color       = font_data.tint;
        result.remapped_uv = uv;
        break;
      }
        bfInvalidDefaultCase();
    }

    return result;
  }

  Gfx2DPainter::Gfx2DPainter(IMemoryManager& memory, GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics) :
    render_data{glsl_compiler, graphics},
    vertices{memory},
    indices{memory},
    shadow_vertices{memory},
    shadow_indices{memory},
    tmp_memory{},
    draw_commands{memory}
  {
  }

  void Gfx2DPainter::reset()
  {
    vertices.clear();
    indices.clear();
    shadow_vertices.clear();
    shadow_indices.clear();
    draw_commands.clear();
    draw_commands.emplace(render_data.white_texture);
  }

  void Gfx2DPainter::bindTexture(bfTextureHandle texture)
  {
    if (!texture)
    {
      texture = render_data.white_texture;
    }

    if (texture != currentDrawCommand().texture)
    {
      draw_commands.emplace(texture).first_index = UIIndexType(indices.size());
    }
  }

  void Gfx2DPainter::pushRectShadow(float shadow_sigma, const Vector2f& pos, float width, float height, float border_radius, bfColor32u color)
  {
    const auto [vertex_id, verts] = requestVertices2(4);

    const float    shadow_border_size  = shadow_sigma * 3.0f;
    const Vector2f shadow_border_size2 = Vector2f{shadow_border_size};
    const Rect2f   box                 = {pos.x, pos.y, width, height};
    const auto     color_4u            = bfColor4u_fromUint32(color);

    verts[0] = {box.topLeft() - shadow_border_size2, shadow_sigma, border_radius, box, color_4u};
    verts[1] = {box.topRight() + Vector2f{shadow_border_size, -shadow_border_size}, shadow_sigma, border_radius, box, color_4u};
    verts[2] = {box.bottomRight() + shadow_border_size2, shadow_sigma, border_radius, box, color_4u};
    verts[3] = {box.bottomLeft() + Vector2f{-shadow_border_size, shadow_border_size}, shadow_sigma, border_radius, box, color_4u};

    pushTriIndex2(vertex_id + 0, vertex_id + 1, vertex_id + 2);
    pushTriIndex2(vertex_id + 0, vertex_id + 2, vertex_id + 3);
  }

  void Gfx2DPainter::pushRect(const Vector2f& pos, float width, float height, bfColor4u color)
  {
    const auto [vertex_id, verts] = requestVertices(4);

    const Vector2f size_x  = {width, 0.0f};
    const Vector2f size_y  = {0.0f, height};
    const Vector2f size_xy = {width, height};

    verts[0] = {pos, {0.0f, 0.0f}, color};
    verts[1] = {pos + size_x, {0.0f, 0.0f}, color};
    verts[2] = {pos + size_xy, {0.0f, 0.0f}, color};
    verts[3] = {pos + size_y, {0.0f, 0.0f}, color};

    pushTriIndex(vertex_id + 0, vertex_id + 1, vertex_id + 2);
    pushTriIndex(vertex_id + 0, vertex_id + 2, vertex_id + 3);
  }

  void Gfx2DPainter::pushRect(const Vector2f& pos, float width, float height, bfColor32u color)
  {
    pushRect(pos, width, height, bfColor4u_fromUint32(color));
  }

  void Gfx2DPainter::pushFillRoundedRect(const Vector2f& pos, float width, float height, float border_radius, bfColor32u color)
  {
    border_radius = std::min({border_radius, width * 0.5f, height * 0.5f});

    const auto     color_4u              = bfColor4u_fromUint32(color);
    const float    two_x_border_radius   = 2.0f * border_radius;
    const Vector2f middle_section_pos    = pos + Vector2f{border_radius, 0.0f};
    const float    middle_section_width  = width - two_x_border_radius;
    const float    middle_section_height = height;
    const Vector2f left_section_pos      = pos + Vector2f{0.0f, border_radius};
    const float    left_section_width    = border_radius;
    const float    left_section_height   = height - two_x_border_radius;
    const Vector2f right_section_pos     = pos + Vector2f{width - border_radius, border_radius};
    const float    right_section_width   = border_radius;
    const float    right_section_height  = left_section_height;
    const Vector2f tl_corner_pos         = pos + Vector2f{border_radius};
    const Vector2f tr_corner_pos         = pos + Vector2f{width - border_radius, border_radius};
    const Vector2f bl_corner_pos         = pos + Vector2f{border_radius, height - border_radius};
    const Vector2f br_corner_pos         = pos + Vector2f{width - border_radius, height - border_radius};

    pushRect(middle_section_pos, middle_section_width, middle_section_height, color_4u);
    pushRect(left_section_pos, left_section_width, left_section_height, color_4u);
    pushRect(right_section_pos, right_section_width, right_section_height, color_4u);
    pushFilledArc(tl_corner_pos, border_radius, k_PI, k_HalfPI, color);
    pushFilledArc(tr_corner_pos, border_radius, -k_HalfPI, k_HalfPI, color);
    pushFilledArc(bl_corner_pos, border_radius, k_HalfPI, k_HalfPI, color);
    pushFilledArc(br_corner_pos, border_radius, 0.0f, k_HalfPI, color);
  }

  // Clockwise Winding.

  static UIIndexType calculateNumSegmentsForArc(float radius)
  {
    return UIIndexType(k_ArcSmoothingFactor * std::sqrt(radius));
  }

  void Gfx2DPainter::pushFilledArc(const Vector2f& pos, float radius, float start_angle, float arc_angle, bfColor32u color)
  {
    if (arc_angle <= 0.0f)
    {
      return;
    }

    if (radius <= 0.0f)
    {
      return;
    }

    assert(radius > 0.0f);
    assert(arc_angle > 0.0f);

    const UIIndexType num_segments      = calculateNumSegmentsForArc(radius);
    const float       theta             = arc_angle / float(num_segments);
    const float       tangential_factor = std::tan(theta);
    const float       radial_factor     = std::cos(theta);
    const auto [vertex_id, verts]       = requestVertices(num_segments * 2 + 1);
    const auto  color_4u                = bfColor4u_fromUint32(color);
    float       x                       = std::cos(start_angle) * radius;
    float       y                       = std::sin(start_angle) * radius;
    UIIndexType current_vertex          = 0;

    verts[current_vertex++] = {pos, {0.0f, 0.0f}, color_4u};

    for (std::size_t i = 0; i < num_segments; ++i)
    {
      const Vector2f    p0       = Vector2f{x + pos.x, y + pos.y};
      const UIIndexType p0_index = current_vertex;

      verts[current_vertex++] = {p0, {0.0f, 0.0f}, color_4u};

      const float tx = -y;
      const float ty = x;

      x += tx * tangential_factor;
      y += ty * tangential_factor;

      x *= radial_factor;
      y *= radial_factor;

      const Vector2f    p1       = Vector2f{x + pos.x, y + pos.y};
      const UIIndexType p1_index = current_vertex;

      verts[current_vertex++] = {p1, {0.0f, 0.0f}, color_4u};

      pushTriIndex(vertex_id, vertex_id + p0_index, vertex_id + p1_index);
    }
  }

  void Gfx2DPainter::pushFilledCircle(const Vector2f& pos, float radius, bfColor32u color)
  {
    pushFilledArc(pos, radius, 0.0f, k_TwoPI, color);
  }

  void Gfx2DPainter::pushLinedArc(const Vector2f& pos, float radius, float start_angle, float arc_angle, bfColor32u color)
  {
    if (arc_angle <= 0.0f)
    {
      return;
    }

    if (radius <= 0.0f)
    {
      return;
    }

    assert(radius > 0.0f);
    assert(arc_angle > 0.0f);

    if (arc_angle > k_TwoPI)
    {
      arc_angle = k_TwoPI;
    }

    const bool           not_full_circle   = arc_angle < k_TwoPI;
    const UIIndexType    num_segments      = calculateNumSegmentsForArc(radius);
    const float          theta             = arc_angle / float(num_segments);
    const float          tangential_factor = std::tan(theta);
    const float          radial_factor     = std::cos(theta);
    float                x                 = std::cos(start_angle) * radius;
    float                y                 = std::sin(start_angle) * radius;
    LinearAllocatorScope mem_scope         = tmp_memory;
    NoFreeAllocator      no_free           = tmp_memory;
    Array<Vector2f>      points            = Array<Vector2f>{no_free};

    points.reserve(num_segments + 2 * not_full_circle);

    if (not_full_circle)
    {
      points.emplace(pos);
    }

    for (std::size_t i = 0; i < num_segments; ++i)
    {
      points.emplace(x + pos.x, y + pos.y);

      const float tx = -y;
      const float ty = x;

      x += tx * tangential_factor;
      y += ty * tangential_factor;

      x *= radial_factor;
      y *= radial_factor;

      points.emplace(x + pos.x, y + pos.y);
    }

    pushPolyline(points.data(), UIIndexType(points.size()), 5.0f, PolylineJoinStyle::ROUND, PolylineEndStyle::CONNECTED, color, true);
  }

  // References:
  //   [https://github.com/CrushedPixel/Polyline2D]
  //   [https://essence.handmade.network/blogs/p/7388-generating_polygon_outlines]
  void Gfx2DPainter::pushPolyline(const Vector2f* points, UIIndexType num_points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bfColor32u color, bool is_overlap_allowed)
  {
    static constexpr float k_TenDegAsRad   = 10.0f * k_DegToRad;
    static constexpr float k_MinAngleMiter = 15.0f * k_DegToRad;

    struct LineSegment final
    {
      Vector2f p0;
      Vector2f p1;

      Vector2f normal() const
      {
        const Vector2f dir = direction();

        return {-dir.y, dir.x};
      }

      Vector2f direction() const
      {
        return vec::normalized(directionUnnormalized());
      }

      Vector2f directionUnnormalized() const
      {
        return p1 - p0;
      }

      LineSegment& operator+=(const Vector2f& offset)
      {
        p0 += offset;
        p1 += offset;

        return *this;
      }

      LineSegment& operator-=(const Vector2f& offset)
      {
        p0 -= offset;
        p1 -= offset;

        return *this;
      }

      bool intersectionWith(const LineSegment& rhs, bool is_infinite, Vector2f& out_result) const
      {
        const auto r      = directionUnnormalized();
        const auto s      = rhs.directionUnnormalized();
        const auto a_to_b = rhs.p0 - p0;
        const auto num    = Vec2f_cross(&a_to_b, &r);
        const auto denom  = Vec2f_cross(&r, &s);

        if (std::abs(denom) < k_Epsilon)
        {
          return false;
        }

        const auto u = num / denom;
        const auto t = Vec2f_cross(&a_to_b, &s) / denom;

        if (!is_infinite && (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f))
        {
          return false;
        }

        out_result = p0 + r * t;

        return true;
      }
    };

    struct PolylineSegment final
    {
      LineSegment      center;
      LineSegment      edges[2];
      PolylineSegment* next;

      PolylineSegment(const LineSegment& center, float half_thickness) :
        center{center},
        edges{center, center},
        next{nullptr}
      {
        const Vector2f thick_normal = center.normal() * half_thickness;

        edges[0] += thick_normal;
        edges[1] -= thick_normal;
      }
    };

    struct LineSegmentList final
    {
      PolylineSegment* head;
      PolylineSegment* tail;

      LineSegmentList() :
        head{nullptr},
        tail{nullptr}
      {
      }

      bool isEmpty() const { return head == nullptr; }

      void add(IMemoryManager& memory, const Vector2f* p0, const Vector2f* p1, float half_thickness)
      {
        if (*p0 != *p1)
        {
          PolylineSegment* new_segment = memory.allocateT<PolylineSegment>(LineSegment{*p0, *p1}, half_thickness);

          // TODO(SR):
          //   Rather than asserting (also this assert should never happen because of an earlier check but...)
          //   we could just truncate the polyline and give off a warning.
          assert(new_segment && "Failed to allocate a new segment.");

          if (tail)
          {
            tail->next = new_segment;
          }
          else
          {
            head = new_segment;
          }

          tail = new_segment;
        }
      }
    };

    static constexpr int k_MaxNumberOfSegments = k_TempMemorySize / sizeof(PolylineSegment);

    // TODO(SR): I should handle this case and just dynamically allocate or something.
    assert(num_points < k_MaxNumberOfSegments && "Polyline has too many points.");

    if (num_points < 2)
    {
      return;
    }

    LinearAllocatorScope mem_scope      = tmp_memory;
    const float          half_thickness = thickness * 0.5f;
    LineSegmentList      segments       = {};
    const auto           color_4u       = bfColor4u_fromUint32(color);

    for (UIIndexType i = 0; (i + 1) < num_points; ++i)
    {
      const Vector2f* p0 = points + i + 0;
      const Vector2f* p1 = points + i + 1;

      segments.add(tmp_memory, p0, p1, half_thickness);
    }

    if (end_style == PolylineEndStyle::CONNECTED)
    {
      const Vector2f* last_point  = points + num_points - 1;
      const Vector2f* first_point = points + 0;

      segments.add(tmp_memory, last_point, first_point, half_thickness);
    }

    if (segments.isEmpty())
    {
      return;
    }

    const auto pushRoundedFan = [this, &color_4u](const Vector2f& center_vertex_pos, const Vector2f& origin, const Vector2f& start, const Vector2f& end) {
      const auto point0 = start - origin;
      const auto point1 = end - origin;
      auto       angle0 = std::atan2(point0.y, point0.x);
      const auto angle1 = std::atan2(point1.y, point1.x);

      if (angle0 > angle1)
      {
        angle0 = angle0 - k_TwoPI;
      }

      const auto [center_vertex_id, center_vertex] = requestVertices(1);
      const float join_angle                       = angle1 - angle0;
      const int   num_tris                         = std::max(1, int(std::floor(std::abs(join_angle) / k_TenDegAsRad)));
      const float tri_angle                        = join_angle / float(num_tris);

      center_vertex[0] = {center_vertex_pos, {0.0f, 0.0f}, color_4u};

      Vector2f start_p = start;

      for (int i = 0; i < num_tris; ++i)
      {
        Vector2f end_p;

        if (i == (num_tris - 1))
        {
          end_p = end;
        }
        else
        {
          const float rotation = float(i + 1) * tri_angle;
          const float cos_rot  = std::cos(rotation);
          const float sin_rot  = std::sin(rotation);

          end_p = Vector2f{
                   cos_rot * point0.x - sin_rot * point0.y,
                   sin_rot * point0.x + cos_rot * point0.y} +
                  origin;
        }

        const auto [vertex_id, verts] = requestVertices(2);

        verts[0] = {start_p, {0.0f, 0.0f}, color_4u};
        verts[1] = {end_p, {0.0f, 0.0f}, color_4u};

        pushTriIndex(vertex_id + 0, vertex_id + 1, center_vertex_id);

        start_p = end_p;
      }
    };

    const auto pushJoint = [this, &color_4u, &pushRoundedFan](
                            const PolylineSegment& segment_one,
                            const PolylineSegment& segment_two,
                            PolylineJoinStyle      style,
                            Vector2f&              out_end0,
                            Vector2f&              out_end1,
                            Vector2f&              out_nxt_start0,
                            Vector2f&              out_nxt_start1,
                            bool                   is_overlap_allowed) {
      const Vector2f dirs[]        = {segment_one.center.direction(), segment_two.center.direction()};
      const float    angle         = vec::angleBetween0ToPI(dirs[0], dirs[1]);
      const float    wrapped_angle = angle > k_HalfPI ? k_PI - angle : angle;

      if (style == PolylineJoinStyle::MITER && wrapped_angle < k_MinAngleMiter)
      {
        style = PolylineJoinStyle::BEVEL;
      }

      switch (style)
      {
        case PolylineJoinStyle::MITER:
        {
          if (!segment_one.edges[0].intersectionWith(segment_two.edges[0], true, out_end0))
          {
            out_end0 = segment_one.edges[0].p1;
          }

          if (!segment_one.edges[1].intersectionWith(segment_two.edges[1], true, out_end1))
          {
            out_end1 = segment_one.edges[1].p1;
          }

          out_nxt_start0 = out_end0;
          out_nxt_start1 = out_end1;
          break;
        }
        case PolylineJoinStyle::BEVEL:
        case PolylineJoinStyle::ROUND:
        {
          const float        x1        = dirs[0].x;
          const float        x2        = dirs[1].x;
          const float        y1        = dirs[0].y;
          const float        y2        = dirs[1].y;
          const bool         clockwise = x1 * y2 - x2 * y1 < 0;
          const LineSegment *inner1, *inner2, *outer1, *outer2;

          if (clockwise)
          {
            outer1 = &segment_one.edges[0];
            outer2 = &segment_two.edges[0];
            inner1 = &segment_one.edges[1];
            inner2 = &segment_two.edges[1];
          }
          else
          {
            outer1 = &segment_one.edges[1];
            outer2 = &segment_two.edges[1];
            inner1 = &segment_one.edges[0];
            inner2 = &segment_two.edges[0];
          }

          Vector2f   inner_intersection;
          const bool inner_intersection_is_valid = inner1->intersectionWith(*inner2, is_overlap_allowed, inner_intersection);

          if (!inner_intersection_is_valid)
          {
            inner_intersection = inner1->p1;
          }

          Vector2f inner_start;

          if (inner_intersection_is_valid)
          {
            inner_start = inner_intersection;
          }
          else
          {
            inner_start = angle > k_TwoPI ? outer1->p1 : inner1->p1;
          }

          if (clockwise)
          {
            out_end0       = outer1->p1;
            out_end1       = inner_intersection;
            out_nxt_start0 = outer2->p0;
            out_nxt_start1 = inner_start;
          }
          else
          {
            out_end0       = inner_intersection;
            out_end1       = outer1->p1;
            out_nxt_start0 = inner_start;
            out_nxt_start1 = outer2->p0;
          }

          if (style == PolylineJoinStyle::BEVEL)
          {
            const auto [vertex_id, verts] = requestVertices(3);

            verts[0] = {outer1->p1, {0.0f, 0.0f}, color_4u};
            verts[1] = {outer2->p0, {0.0f, 0.0f}, color_4u};
            verts[2] = {inner_intersection, {0.0f, 0.0f}, color_4u};

            if (clockwise)
            {
              pushTriIndex(vertex_id + 0, vertex_id + 2, vertex_id + 1);
            }
            else
            {
              pushTriIndex(vertex_id + 0, vertex_id + 1, vertex_id + 2);
            }
          }
          else  // ROUND
          {
            if (clockwise)
            {
              pushRoundedFan(inner_intersection, segment_one.center.p1, outer2->p0, outer1->p1);
            }
            else
            {
              pushRoundedFan(inner_intersection, segment_one.center.p1, outer1->p1, outer2->p0);
            }
          }

          break;
        }
      }
    };

    auto&    first_segment = *segments.head;
    auto&    last_segment  = *segments.tail;
    Vector2f path_starts[] = {first_segment.edges[0].p0, first_segment.edges[1].p0};
    Vector2f path_ends[]   = {last_segment.edges[0].p1, last_segment.edges[1].p1};

    switch (end_style)
    {
      case PolylineEndStyle::FLAT:
      {
        break;
      }
      case PolylineEndStyle::SQUARE:
      {
        const Vector2f first_segment_dir0 = first_segment.edges[0].direction() * half_thickness;
        const Vector2f first_segment_dir1 = first_segment.edges[1].direction() * half_thickness;
        const Vector2f last_segment_dir0  = last_segment.edges[0].direction() * half_thickness;
        const Vector2f last_segment_dir1  = last_segment.edges[1].direction() * half_thickness;

        path_starts[0] -= first_segment_dir0;
        path_starts[1] -= first_segment_dir1;
        path_ends[0] -= last_segment_dir0;
        path_ends[1] -= last_segment_dir1;

        break;
      }
      case PolylineEndStyle::ROUND:
      {
        pushRoundedFan(first_segment.center.p0, first_segment.center.p0, first_segment.edges[0].p0, first_segment.edges[1].p0);
        pushRoundedFan(last_segment.center.p1, last_segment.center.p1, last_segment.edges[1].p1, last_segment.edges[0].p1);
        break;
      }
      case PolylineEndStyle::CONNECTED:
      {
        pushJoint(last_segment, first_segment, join_style, path_ends[0], path_ends[1], path_starts[0], path_starts[1], is_overlap_allowed);
        break;
      }
    }

    PolylineSegment* segment = segments.head;

    Vector2f starts[2];
    Vector2f ends[2];

    while (segment)
    {
      PolylineSegment* const next_segment = segment->next;
      Vector2f               nxt_starts[2];

      if (segment == segments.head)
      {
        starts[0] = path_starts[0];
        starts[1] = path_starts[1];
      }

      if (segment == segments.tail)
      {
        ends[0] = path_ends[0];
        ends[1] = path_ends[1];
      }
      else
      {
        pushJoint(
         *segment,
         *next_segment,
         join_style,
         ends[0],
         ends[1],
         nxt_starts[0],
         nxt_starts[1],
         is_overlap_allowed);
      }

      const auto [vertex_id, verts] = requestVertices(4);

      verts[0] = {starts[0], {0.0f, 0.0f}, color_4u};
      verts[1] = {starts[1], {0.0f, 0.0f}, color_4u};
      verts[2] = {ends[0], {0.0f, 0.0f}, color_4u};
      verts[3] = {ends[1], {0.0f, 0.0f}, color_4u};

      pushTriIndex(vertex_id + 0, vertex_id + 1, vertex_id + 2);
      pushTriIndex(vertex_id + 2, vertex_id + 1, vertex_id + 3);

      segment   = next_segment;
      starts[0] = nxt_starts[0];
      starts[1] = nxt_starts[1];
    }
  }

  void Gfx2DPainter::pushPolyline(ArrayView<const Vector2f> points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bfColor32u color)
  {
    pushPolyline(points.data, UIIndexType(points.data_size), thickness, join_style, end_style, color);
  }

  // TODO(SR): This should be in the data structures lib.
  template<typename T>
  struct TempList
  {
    T* first = nullptr;
    T* last  = nullptr;

    template<typename F>
    void forEach(F&& callback)
    {
      T* it = first;

      while (it)
      {
        T* const next = it->next;

        callback(it);

        it = next;
      }
    }

    void add(T* item)
    {
      if (!first)
      {
        first = item;
      }

      if (last)
      {
        last->next = item;
      }

      last = item;
    }
  };

  CommandBuffer2D::CommandBuffer2D(GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics) :
    render_data{glsl_compiler, graphics},
    aux_memory{},
    command_stream{},
    vertex_stream{},
    index_stream{},
    num_commands{0u}
  {
  }

  Brush* CommandBuffer2D::makeBrush(bfColor4f color)
  {
    Brush* const result = aux_memory.allocateT<Brush>();

    result->type               = Brush::Colored;
    result->colored_data.value = color;

    return result;
  }

  Brush* CommandBuffer2D::makeBrush(bfColor4f color_a, bfColor4f color_b)
  {
    Brush* const result = aux_memory.allocateT<Brush>();

    result->type                           = Brush::LinearGradient;
    result->linear_gradient_data.colors[0] = color_a;
    result->linear_gradient_data.colors[1] = color_b;
    result->linear_gradient_data.uv_remap  = AxisQuad::make();

    return result;
  }

  Brush* CommandBuffer2D::makeBrushGradient(std::size_t num_gradient_stops)
  {
    Brush* const result = aux_memory.allocateT<Brush>();

    result->type                                 = Brush::NaryLinearGradient;
    result->nary_linear_gradient_data.colors     = aux_memory.allocateArrayTrivial<GradientStop>(num_gradient_stops);
    result->nary_linear_gradient_data.num_colors = num_gradient_stops;
    result->nary_linear_gradient_data.uv_remap   = AxisQuad::make();

    return result;
  }

  Brush* CommandBuffer2D::makeBrush(bfTextureHandle texture, bfColor4f tint)
  {
    Brush* const result = aux_memory.allocateT<Brush>();

    result->type                   = Brush::Textured;
    result->textured_data.texture  = texture;
    result->textured_data.tint     = tint;
    result->textured_data.uv_remap = AxisQuad::make();

    return result;
  }

  Brush* CommandBuffer2D::makeBrush(PainterFont* font, bfColor4f tint)
  {
    Brush* const result = aux_memory.allocateT<Brush>();

    result->type           = Brush::Font;
    result->font_data.font = font;
    result->font_data.tint = tint;

    return result;
  }

  Render2DFillRect* CommandBuffer2D::fillRect(const Brush* brush, AxisQuad rect)
  {
    Render2DFillRect* result = command_stream.allocateT<Render2DFillRect>();

    result->brush = brush;
    result->rect  = rect;

    ++num_commands;
    return result;
  }

  Render2DFillRoundedRect* CommandBuffer2D::fillRoundedRect(const Brush* brush, AxisQuad rect, float border_radius)
  {
    border_radius = std::min({border_radius, rect.width() * 0.5f, rect.height() * 0.5f});

    assert(border_radius >= 0.0f && "A border radius of less than 0.0f does not make sense.");

    Render2DFillRoundedRect* result = command_stream.allocateT<Render2DFillRoundedRect>();

    result->brush         = brush;
    result->rect          = rect;
    result->border_radius = border_radius;

    ++num_commands;
    return result;
  }

  Render2DBlurredRect* CommandBuffer2D::blurredRect(const Brush* brush, Rect2f rect, float shadow_sigma, float border_radius)
  {
    Render2DBlurredRect* result = command_stream.allocateT<Render2DBlurredRect>();

    result->brush        = brush;
    result->rect         = rect;
    result->shadow_sigma = shadow_sigma;

    for (float& border_radius_ : result->border_radii)
    {
      border_radius_ = border_radius;
    }

    ++num_commands;
    return result;
  }

  Render2DFillArc* CommandBuffer2D::fillArc(const Brush* brush, Vector2f position, float radius, float start_angle, float arc_angle)
  {
    assert(radius > 0.0f && "A radius of zero or less does not make sense.");
    assert(arc_angle > 0.0f && "An arc angle of zero or less does not make sense.");

    Render2DFillArc* result = command_stream.allocateT<Render2DFillArc>();

    result->brush       = brush;
    result->position    = position;
    result->radius      = radius;
    result->start_angle = start_angle;
    result->arc_angle   = arc_angle > k_TwoPI ? k_TwoPI : arc_angle;

    ++num_commands;
    return result;
  }

  Render2DPolyline* CommandBuffer2D::polyline(const Brush* brush, const Vector2f* points, UIIndexType num_points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bool is_overlap_allowed)
  {
    Render2DPolyline* result = command_stream.allocateT<Render2DPolyline>();

    result->brush              = brush;
    result->points             = aux_memory.allocateArrayTrivial<Vector2f>(num_points);
    result->num_points         = num_points;
    result->thickness          = thickness;
    result->join_style         = join_style;
    result->end_style          = end_style;
    result->is_overlap_allowed = is_overlap_allowed;

    std::memcpy(result->points, points, sizeof(Vector2f) * num_points);

    ++num_commands;
    return result;
  }

  Render2DText* CommandBuffer2D::text(const Brush* brush, Vector2f position, StringRange utf8_text)
  {
    assert(brush->type == Brush::Font && "Text must be drawn with a Font brush.");

    Render2DText* result = command_stream.allocateT<Render2DText>();

    const BufferLen cloned_string = string_utils::clone(aux_memory, utf8_text);
    UIIndexType     num_codepoints;

    result->brush          = brush;
    result->utf8_text      = cloned_string.toStringRange();
    result->bounds_size    = calculateTextSize(utf8_text, brush->font_data.font, num_codepoints);
    result->num_codepoints = num_codepoints;
    result->position       = position;

    ++num_commands;
    return result;
  }

  void CommandBuffer2D::TEST(RenderQueue& render_queue, const DescSetBind& object_binding)
  {
    if (!num_commands)
    {
      return;
    }

    // Flags for Gfx2DElement

    static constexpr std::uint8_t HAS_BEEN_DRAWN      = bfBit(0);
    static constexpr std::uint8_t IS_IN_CURRENT_BATCH = bfBit(1);

    struct Gfx2DElement
    {
      //
      // This object is used in a few 'passes' od processing
      //
      // Field usage by pass:
      //   (1) Batch creation uses:  `bounds`, `is_in_current_batch`, `command`, `next`
      //   (2) Vertex counting uses: `command`, `vertex_idx_count`, `next`.
      //   (3) Vertex GPU Upload:    `command`, `vertex_idx_count`, `next`.
      //

      Rect2f                      bounds;
      std::uint8_t                flags;
      const BaseRender2DCommand_* command;
      Gfx2DElement*               next;
      VertIdxCountResult          vertex_idx_count;

      bool hasDrawn() const { return flags & HAS_BEEN_DRAWN; }
      bool isInCurrentBatch() const { return flags & IS_IN_CURRENT_BATCH; }
    };

    // All batches have at least one command.
    struct Batch2D
    {
      TempList<Gfx2DElement> commands    = {};
      Batch2D*               next        = nullptr;
      UIIndexType            first_index = 0u;
      UIIndexType            num_indices = 0u;
    };

    struct BatchList : public TempList<Batch2D>
    {
      void findOrAdd(LinearAllocator& alloc, Gfx2DElement* item)
      {
        Batch2D* compatible_batch = first;

        while (compatible_batch)
        {
          Batch2D* const                    it_next = compatible_batch->next;
          const BaseRender2DCommand_* const command = compatible_batch->commands.first->command;

          if (
           command->isBlurred() == item->command->isBlurred() &&
           command->brush->canBeBatchedWith(*item->command->brush))
          {
            break;
          }

          compatible_batch = it_next;
        }

        if (!compatible_batch)
        {
          compatible_batch = alloc.allocateT<Batch2D>();
          add(compatible_batch);
        }

        compatible_batch->commands.add(item);
      }
    };

    Gfx2DElement*     elements                  = aux_memory.allocateArray<Gfx2DElement>(num_commands);
    const std::size_t num_elements              = num_commands;
    std::size_t       num_elements_left_to_sort = num_elements;
    BatchList         final_batches             = {};

    char* byte_stream = command_stream.begin();

    for (std::size_t i = 0u; i < num_commands; ++i)
    {
      BaseRender2DCommand_* command = reinterpret_cast<BaseRender2DCommand_*>(byte_stream);

      elements[i].bounds  = calcCommandBounds(command);
      elements[i].flags   = 0x0;
      elements[i].command = command;
      elements[i].next    = nullptr;
      // elements[i].vertex_idx_count = /* ---, this will be written to later */

      byte_stream += command->size;
    }

    //
    // Invariants for Why This Works:
    // - All `Gfx2DElement`s are in back to front order.
    // - All elements marked as `is_in_current_batch` are behind the sprite currently being processed.
    //
    {
      BatchList working_list = {};

      while (num_elements_left_to_sort)
      {
        for (std::size_t i = 0; i < num_elements; ++i)
        {
          Gfx2DElement& element = elements[i];

          if (!element.hasDrawn())
          {
            bool can_add_to_batch = true;

            for (std::size_t j = 0; j < i; ++j)
            {
              const Gfx2DElement& behind_element = elements[j];

              // The first two checks are cheaper than the intersection.
              if ((behind_element.isInCurrentBatch() || !behind_element.hasDrawn()) &&
                  element.bounds.intersectsRect(behind_element.bounds))
              {
                can_add_to_batch = false;
                break;
              }
            }

            if (can_add_to_batch)
            {
              working_list.findOrAdd(aux_memory, &element);
              element.flags |= HAS_BEEN_DRAWN | IS_IN_CURRENT_BATCH;
              --num_elements_left_to_sort;
            }
          }
        }

        // Add all but the last one to the final list
        Batch2D* it = working_list.first;

        while (it)
        {
          Batch2D* const it_next = it->next;
          Gfx2DElement*  element = it->commands.first;

          while (element)
          {
            element->flags &= ~IS_IN_CURRENT_BATCH;
            element = element->next;
          }

          if (it != working_list.last)
          {
            final_batches.add(it);
          }

          it = it_next;
        }

        // Merging with the last active batch is preferable.
        working_list.first = working_list.last;
      }

      // Add the rest of the batches.
      Batch2D* it = working_list.first;

      while (it)
      {
        Batch2D* const it_next = it->next;
        final_batches.add(it);
        it = it_next;
      }
    }

    // Count Up Vertices / Indices Needed

    struct Counts
    {
      UIIndexType num_vertices      = 0u;
      UIIndexType num_indices       = 0u;
      UIIndexType num_blur_vertices = 0u;
      UIIndexType num_blur_indices  = 0u;
    };

    Counts counts = {};

    final_batches.forEach([this, &counts](Batch2D* batch) {
      batch->commands.forEach([this, &counts](Gfx2DElement* element) {
        element->vertex_idx_count = calcVertexCount(counts.num_vertices, element->command);

        if (!element->command->isBlurred())
        {
          counts.num_vertices += element->vertex_idx_count.num_vertices;
          counts.num_indices += element->vertex_idx_count.num_indices;
        }
        else
        {
          counts.num_blur_vertices += element->vertex_idx_count.num_vertices;
          counts.num_blur_indices += element->vertex_idx_count.num_indices;
        }
      });
    });

    //
    // Upload the Vertex / Index Data To The GPU
    //

    const auto&              frame_info = bfGfxContext_getFrameInfo(render_data.ctx);
    Gfx2DPerFrameRenderData& frame_data = render_data.frame_datas[frame_info.frame_index];

    render_data.reserve(frame_info.frame_index, counts.num_vertices, counts.num_indices);
    render_data.reserveShadow(frame_info.frame_index, counts.num_blur_vertices, counts.num_blur_indices);

    UIVertex2D* const       vertex_buffer_ptr        = frame_data.vertex_buffer ? static_cast<UIVertex2D*>(bfBuffer_map(frame_data.vertex_buffer, 0, k_bfBufferWholeSize)) : nullptr;
    UIIndexType* const      index_buffer_ptr         = frame_data.index_buffer ? static_cast<UIIndexType*>(bfBuffer_map(frame_data.index_buffer, 0, k_bfBufferWholeSize)) : nullptr;
    DropShadowVertex* const shadow_vertex_buffer_ptr = frame_data.vertex_shadow_buffer ? static_cast<DropShadowVertex*>(bfBuffer_map(frame_data.vertex_shadow_buffer, 0, k_bfBufferWholeSize)) : nullptr;
    UIIndexType* const      shadow_index_buffer_ptr  = frame_data.index_shadow_buffer ? static_cast<UIIndexType*>(bfBuffer_map(frame_data.index_shadow_buffer, 0, k_bfBufferWholeSize)) : nullptr;

    DestVerts dest;  // NOLINT(cppcoreguidelines-pro-type-member-init)

    dest.vertex_buffer_ptr        = vertex_buffer_ptr;
    dest.index_buffer_ptr         = index_buffer_ptr;
    dest.shadow_vertex_buffer_ptr = shadow_vertex_buffer_ptr;
    dest.shadow_index_buffer_ptr  = shadow_index_buffer_ptr;
    dest.vertex_offset            = 0u;
    dest.shadow_vertex_offset     = 0u;

    UIIndexType normal_index_count = 0u;
    UIIndexType shadow_index_count = 0u;

    final_batches.forEach([this, &dest, &normal_index_count, &shadow_index_count](Batch2D* batch) {
      const bool is_shadow = batch->commands.first->command->isBlurred();
      batch->first_index   = is_shadow ? shadow_index_count : normal_index_count;

      batch->commands.forEach([this, is_shadow, &dest, &normal_index_count, &shadow_index_count](Gfx2DElement* element) {
        writeVertices(dest, element->command, element->vertex_idx_count);

        if (!is_shadow)
        {
          dest.vertex_buffer_ptr += element->vertex_idx_count.num_vertices;
          dest.index_buffer_ptr += element->vertex_idx_count.num_indices;
          dest.vertex_offset += element->vertex_idx_count.num_vertices;

          normal_index_count += element->vertex_idx_count.num_indices;
        }
        else
        {
          dest.shadow_vertex_buffer_ptr += element->vertex_idx_count.num_vertices;
          dest.shadow_index_buffer_ptr += element->vertex_idx_count.num_indices;
          dest.shadow_vertex_offset += element->vertex_idx_count.num_vertices;

          shadow_index_count += element->vertex_idx_count.num_indices;
        }
      });

      batch->num_indices = (is_shadow ? shadow_index_count : normal_index_count) - batch->first_index;
    });

    if (frame_data.vertex_shadow_buffer)
    {
      bfBuffer_unMap(frame_data.vertex_shadow_buffer);
    }

    if (frame_data.index_shadow_buffer)
    {
      bfBuffer_unMap(frame_data.index_shadow_buffer);
    }

    if (frame_data.vertex_buffer)
    {
      bfBuffer_unMap(frame_data.vertex_buffer);
    }

    if (frame_data.index_buffer)
    {
      bfBuffer_unMap(frame_data.index_buffer);
    }

    // Draw Batches
    bfDrawCallPipeline pipeline;
    bfDrawCallPipeline_defaultAlphaBlending(&pipeline);

    pipeline.state.cull_face        = BF_CULL_FACE_BACK;
    pipeline.state.dynamic_scissor  = bfTrue;
    pipeline.state.dynamic_viewport = bfTrue;

    final_batches.forEach([this, &pipeline, &render_queue, &frame_data, &object_binding, &frame_info](Batch2D* batch) {
      const BaseRender2DCommand_* command = batch->commands.first->command;
      bfBufferHandle              index_buffer;
      bfBufferHandle              vertex_buffer;

      if (command->isBlurred())
      {
        pipeline.program       = render_data.rounded_rect_shadow_program;
        pipeline.vertex_layout = render_data.vertex_layouts[1];
        index_buffer           = frame_data.index_shadow_buffer;
        vertex_buffer          = frame_data.vertex_shadow_buffer;
      }
      else
      {
        pipeline.program       = render_data.shader_program;
        pipeline.vertex_layout = render_data.vertex_layouts[0];
        index_buffer           = frame_data.index_buffer;
        vertex_buffer          = frame_data.vertex_buffer;
      }

      RC_DrawIndexed* const render_command = render_queue.drawIndexed(pipeline, 1, index_buffer);

      if (!command->isBlurred())
      {
        bfDescriptorSetInfo material_desc_set = bfDescriptorSetInfo_make();
        bfTextureHandle     texture;

        if (command->brush->type == Brush::Textured)
        {
          texture = command->brush->textured_data.texture;
        }
        else if (command->brush->type == Brush::Font)
        {
          texture = command->brush->font_data.font->gpu_atlas[frame_info.frame_index].handle;
        }
        else
        {
          texture = render_data.white_texture;
        }

        bfDescriptorSetInfo_addTexture(&material_desc_set, 0, 0, &texture, 1);

        render_command->material_binding.set(material_desc_set);
      }

      render_command->object_binding            = object_binding;
      render_command->vertex_buffers[0]         = vertex_buffer;
      render_command->vertex_binding_offsets[0] = 0u;
      render_command->index_offset              = batch->first_index;
      render_command->num_indices               = batch->num_indices;

      render_queue.submit(render_command, 0.0f);
    });
  }

  Rect2f boundsFromPoints(const Vector2f* points, std::size_t num_points)
  {
    Vector2f min_point = points[0];
    Vector2f max_point = points[0];

    for (std::size_t i = 1; i < num_points; ++i)
    {
      min_point = vec::min(min_point, points[i]);
      max_point = vec::max(max_point, points[i]);
    }

    return {min_point, max_point};
  }

  Rect2f CommandBuffer2D::calcCommandBounds(const BaseRender2DCommand_* command)
  {
    switch (command->type)
    {
      case Render2DCommandType::FillRect: return static_cast<const Render2DFillRect*>(command)->rect.bounds();
      case Render2DCommandType::FillRoundedRect: return static_cast<const Render2DFillRoundedRect*>(command)->rect.bounds();
      case Render2DCommandType::BlurredRect: return static_cast<const Render2DBlurredRect*>(command)->rect;
      case Render2DCommandType::NineSliceRect: return static_cast<const Render2DNineSliceRect*>(command)->rect.bounds();
      case Render2DCommandType::FillArc:
      {
        const Render2DFillArc* typed_command = static_cast<const Render2DFillArc*>(command);

        return Rect2f{typed_command->position, typed_command->radius};
      }
      case Render2DCommandType::Polyline:
      {
        const Render2DPolyline* typed_command = static_cast<const Render2DPolyline*>(command);

        return boundsFromPoints(typed_command->points, typed_command->num_points);
      }
      case Render2DCommandType::FillTriangles:
      {
        const Render2DFillTriangles* typed_command = static_cast<const Render2DFillTriangles*>(command);

        return boundsFromPoints(typed_command->points, typed_command->num_points);
      }
      case Render2DCommandType::Text:
      {
        const Render2DText* typed_command = static_cast<const Render2DText*>(command);

        // TODO(SR):
        //   Check if this is fully correct,
        //   This is because text baseline stuff is hard.

        const Vector2f min_bounds = {typed_command->position.x, typed_command->position.y - typed_command->bounds_size.y};
        const Vector2f max_bounds = {typed_command->position.x + typed_command->bounds_size.x, typed_command->position.y};

        return {min_bounds, max_bounds};
      }
        bfInvalidDefaultCase();
    }
  }

  CommandBuffer2D::VertIdxCountResult CommandBuffer2D::calcVertexCount(UIIndexType global_index_offset, const BaseRender2DCommand_* command)
  {
    VertIdxCountResult result = {};

    const auto addRectFillCount = [&result]() {
      result.num_vertices += k_NumVertRect;
      result.num_indices += k_NumIdxRect;
    };

    const auto addArcFillCount = [&result](float border_radius) {
      const UIIndexType num_segments = calculateNumSegmentsForArc(border_radius);

      result.num_vertices += num_segments * 2 + 1;
      result.num_indices += num_segments * 3;
    };

    switch (command->type)
    {
      case Render2DCommandType::FillRect:
      {
        addRectFillCount();
        break;
      }
      case Render2DCommandType::FillRoundedRect:
      {
        const auto* const typed_command = static_cast<const Render2DFillRoundedRect*>(command);

        addArcFillCount(typed_command->border_radius);
        addArcFillCount(typed_command->border_radius);
        addArcFillCount(typed_command->border_radius);
        addArcFillCount(typed_command->border_radius);

        addRectFillCount();
        addRectFillCount();
        addRectFillCount();
        break;
      }
      case Render2DCommandType::BlurredRect:
      {
        addRectFillCount();
        break;
      }
      case Render2DCommandType::NineSliceRect:
      {
        assert(false);  // TODO(SR):
        break;
      }
      case Render2DCommandType::FillArc:
      {
        const auto* const typed_command = static_cast<const Render2DFillArc*>(command);

        addArcFillCount(typed_command->radius);
        break;
      }
      case Render2DCommandType::Polyline:
      {
        static constexpr float     k_TenDegAsRad     = 10.0f * k_DegToRad;
        static constexpr float     k_MinAngleMiter   = 15.0f * k_DegToRad;
        static constexpr bfColor4u k_UnassignedColor = {255, 0, 255, 255};  // Magenta

        const auto* const typed_command = static_cast<const Render2DPolyline*>(command);

        // Inputs:
        const auto points             = typed_command->points;
        const auto num_points         = typed_command->num_points;
        const auto thickness          = typed_command->thickness;
        const auto join_style         = typed_command->join_style;
        const auto end_style          = typed_command->end_style;
        const auto is_overlap_allowed = typed_command->is_overlap_allowed;

        if (num_points >= 2)
        {
          // References:
          //   [https://github.com/CrushedPixel/Polyline2D]
          //   [https://essence.handmade.network/blogs/p/7388-generating_polygon_outlines]

          struct LineSegment final
          {
            Vector2f p0;
            Vector2f p1;

            Vector2f normal() const
            {
              const Vector2f dir = direction();

              return {-dir.y, dir.x};
            }

            Vector2f direction() const
            {
              return vec::normalized(directionUnnormalized());
            }

            Vector2f directionUnnormalized() const
            {
              return p1 - p0;
            }

            LineSegment& operator+=(const Vector2f& offset)
            {
              p0 += offset;
              p1 += offset;

              return *this;
            }

            LineSegment& operator-=(const Vector2f& offset)
            {
              p0 -= offset;
              p1 -= offset;

              return *this;
            }

            bool intersectionWith(const LineSegment& rhs, bool is_infinite, Vector2f& out_result) const
            {
              const auto r      = directionUnnormalized();
              const auto s      = rhs.directionUnnormalized();
              const auto a_to_b = rhs.p0 - p0;
              const auto num    = Vec2f_cross(&a_to_b, &r);
              const auto denom  = Vec2f_cross(&r, &s);

              if (std::abs(denom) < k_Epsilon)
              {
                return false;
              }

              const auto u = num / denom;
              const auto t = Vec2f_cross(&a_to_b, &s) / denom;

              if (!is_infinite && (t < 0.0f || t > 1.0f || u < 0.0f || u > 1.0f))
              {
                return false;
              }

              out_result = p0 + r * t;

              return true;
            }
          };

          struct PolylineSegment final
          {
            LineSegment      center;
            LineSegment      edges[2];
            PolylineSegment* next;

            PolylineSegment(const LineSegment& center, float half_thickness) :
              center{center},
              edges{center, center},
              next{nullptr}
            {
              const Vector2f thick_normal = center.normal() * half_thickness;

              edges[0] += thick_normal;
              edges[1] -= thick_normal;
            }
          };

          struct LineSegmentList final
          {
            PolylineSegment* head;
            PolylineSegment* tail;

            LineSegmentList() :
              head{nullptr},
              tail{nullptr}
            {
            }

            bool isEmpty() const { return head == nullptr; }

            void add(IMemoryManager& memory, const Vector2f* p0, const Vector2f* p1, float half_thickness)
            {
              if (*p0 != *p1)
              {
                PolylineSegment* new_segment = memory.allocateT<PolylineSegment>(LineSegment{*p0, *p1}, half_thickness);

                // TODO(SR):
                //   Rather than asserting (also this assert should never happen because of an earlier check but...)
                //   we could just truncate the polyline and give off a warning.
                assert(new_segment && "Failed to allocate a new segment.");

                if (tail)
                {
                  tail->next = new_segment;
                }
                else
                {
                  head = new_segment;
                }

                tail = new_segment;
              }
            }
          };

          LinearAllocatorScope mem_scope      = aux_memory;
          const float          half_thickness = thickness * 0.5f;
          LineSegmentList      segments       = {};

          for (UIIndexType i = 0; (i + 1) < num_points; ++i)
          {
            const Vector2f* p0 = points + i + 0;
            const Vector2f* p1 = points + i + 1;

            segments.add(aux_memory, p0, p1, half_thickness);
          }

          if (end_style == PolylineEndStyle::CONNECTED)
          {
            const Vector2f* last_point  = points + num_points - 1;
            const Vector2f* first_point = points + 0;

            segments.add(aux_memory, last_point, first_point, half_thickness);
          }

          if (!segments.isEmpty())
          {
            const auto pushRoundedFan = [this, &result, global_index_offset](const Vector2f& center_vertex_pos, const Vector2f& origin, const Vector2f& start, const Vector2f& end) {
              const auto point0 = start - origin;
              const auto point1 = end - origin;
              auto       angle0 = std::atan2(point0.y, point0.x);
              const auto angle1 = std::atan2(point1.y, point1.x);

              if (angle0 > angle1)
              {
                angle0 = angle0 - k_TwoPI;
              }

              const auto [center_vertex_id, center_vertex] = result.requestVertices(vertex_stream, 1);
              const float join_angle                       = angle1 - angle0;
              const int   num_tris                         = std::max(1, int(std::floor(std::abs(join_angle) / k_TenDegAsRad)));
              const float tri_angle                        = join_angle / float(num_tris);

              center_vertex[0] = {center_vertex_pos, {0.0f, 0.0f}, k_UnassignedColor};

              Vector2f start_p = start;

              for (int i = 0; i < num_tris; ++i)
              {
                Vector2f end_p;

                if (i == (num_tris - 1))
                {
                  end_p = end;
                }
                else
                {
                  const float rotation = float(i + 1) * tri_angle;
                  const float cos_rot  = std::cos(rotation);
                  const float sin_rot  = std::sin(rotation);

                  end_p = Vector2f{
                           cos_rot * point0.x - sin_rot * point0.y,
                           sin_rot * point0.x + cos_rot * point0.y} +
                          origin;
                }

                const auto [vertex_id, verts] = result.requestVertices(vertex_stream, 2);

                verts[0] = {start_p, {0.0f, 0.0f}, k_UnassignedColor};
                verts[1] = {end_p, {0.0f, 0.0f}, k_UnassignedColor};

                result.pushTriIndex(global_index_offset, index_stream, vertex_id + 1, vertex_id + 0, center_vertex_id);

                start_p = end_p;
              }
            };

            const auto pushJoint = [this, &result, &pushRoundedFan, global_index_offset](
                                    const PolylineSegment& segment_one,
                                    const PolylineSegment& segment_two,
                                    PolylineJoinStyle      style,
                                    Vector2f&              out_end0,
                                    Vector2f&              out_end1,
                                    Vector2f&              out_nxt_start0,
                                    Vector2f&              out_nxt_start1,
                                    bool                   is_overlap_allowed) {
              const Vector2f dirs[]        = {segment_one.center.direction(), segment_two.center.direction()};
              const float    angle         = vec::angleBetween0ToPI(dirs[0], dirs[1]);
              const float    wrapped_angle = angle > k_HalfPI ? k_PI - angle : angle;

              if (style == PolylineJoinStyle::MITER && wrapped_angle < k_MinAngleMiter)
              {
                style = PolylineJoinStyle::BEVEL;
              }

              switch (style)
              {
                case PolylineJoinStyle::MITER:
                {
                  if (!segment_one.edges[0].intersectionWith(segment_two.edges[0], true, out_end0))
                  {
                    out_end0 = segment_one.edges[0].p1;
                  }

                  if (!segment_one.edges[1].intersectionWith(segment_two.edges[1], true, out_end1))
                  {
                    out_end1 = segment_one.edges[1].p1;
                  }

                  out_nxt_start0 = out_end0;
                  out_nxt_start1 = out_end1;
                  break;
                }
                case PolylineJoinStyle::BEVEL:
                case PolylineJoinStyle::ROUND:
                {
                  const float        x1        = dirs[0].x;
                  const float        x2        = dirs[1].x;
                  const float        y1        = dirs[0].y;
                  const float        y2        = dirs[1].y;
                  const bool         clockwise = x1 * y2 - x2 * y1 < 0;
                  const LineSegment *inner1, *inner2, *outer1, *outer2;

                  if (clockwise)
                  {
                    outer1 = &segment_one.edges[0];
                    outer2 = &segment_two.edges[0];
                    inner1 = &segment_one.edges[1];
                    inner2 = &segment_two.edges[1];
                  }
                  else
                  {
                    outer1 = &segment_one.edges[1];
                    outer2 = &segment_two.edges[1];
                    inner1 = &segment_one.edges[0];
                    inner2 = &segment_two.edges[0];
                  }

                  Vector2f   inner_intersection;
                  const bool inner_intersection_is_valid = inner1->intersectionWith(*inner2, is_overlap_allowed, inner_intersection);

                  if (!inner_intersection_is_valid)
                  {
                    inner_intersection = inner1->p1;
                  }

                  Vector2f inner_start;

                  if (inner_intersection_is_valid)
                  {
                    inner_start = inner_intersection;
                  }
                  else
                  {
                    inner_start = angle > k_TwoPI ? outer1->p1 : inner1->p1;
                  }

                  if (clockwise)
                  {
                    out_end0       = outer1->p1;
                    out_end1       = inner_intersection;
                    out_nxt_start0 = outer2->p0;
                    out_nxt_start1 = inner_start;
                  }
                  else
                  {
                    out_end0       = inner_intersection;
                    out_end1       = outer1->p1;
                    out_nxt_start0 = inner_start;
                    out_nxt_start1 = outer2->p0;
                  }

                  if (style == PolylineJoinStyle::BEVEL)
                  {
                    const auto [vertex_id, verts] = result.requestVertices(vertex_stream, 3);

                    verts[0] = {outer1->p1, {0.0f, 0.0f}, k_UnassignedColor};
                    verts[1] = {outer2->p0, {0.0f, 0.0f}, k_UnassignedColor};
                    verts[2] = {inner_intersection, {0.0f, 0.0f}, k_UnassignedColor};

                    if (!clockwise)
                    {
                      result.pushTriIndex(global_index_offset, index_stream, vertex_id + 0, vertex_id + 2, vertex_id + 1);
                    }
                    else
                    {
                      result.pushTriIndex(global_index_offset, index_stream, vertex_id + 0, vertex_id + 1, vertex_id + 2);
                    }
                  }
                  else  // ROUND
                  {
                    if (!clockwise)
                    {
                      pushRoundedFan(inner_intersection, segment_one.center.p1, outer2->p0, outer1->p1);
                    }
                    else
                    {
                      pushRoundedFan(inner_intersection, segment_one.center.p1, outer1->p1, outer2->p0);
                    }
                  }

                  break;
                }
              }
            };

            auto&    first_segment = *segments.head;
            auto&    last_segment  = *segments.tail;
            Vector2f path_starts[] = {first_segment.edges[0].p0, first_segment.edges[1].p0};
            Vector2f path_ends[]   = {last_segment.edges[0].p1, last_segment.edges[1].p1};

            switch (end_style)
            {
              case PolylineEndStyle::FLAT:
              {
                break;
              }
              case PolylineEndStyle::SQUARE:
              {
                const Vector2f first_segment_dir0 = first_segment.edges[0].direction() * half_thickness;
                const Vector2f first_segment_dir1 = first_segment.edges[1].direction() * half_thickness;
                const Vector2f last_segment_dir0  = last_segment.edges[0].direction() * half_thickness;
                const Vector2f last_segment_dir1  = last_segment.edges[1].direction() * half_thickness;

                path_starts[0] -= first_segment_dir0;
                path_starts[1] -= first_segment_dir1;
                path_ends[0] -= last_segment_dir0;
                path_ends[1] -= last_segment_dir1;

                break;
              }
              case PolylineEndStyle::ROUND:
              {
                pushRoundedFan(first_segment.center.p0, first_segment.center.p0, first_segment.edges[0].p0, first_segment.edges[1].p0);
                pushRoundedFan(last_segment.center.p1, last_segment.center.p1, last_segment.edges[1].p1, last_segment.edges[0].p1);
                break;
              }
              case PolylineEndStyle::CONNECTED:
              {
                pushJoint(last_segment, first_segment, join_style, path_ends[0], path_ends[1], path_starts[0], path_starts[1], is_overlap_allowed);
                break;
              }
            }

            PolylineSegment* segment = segments.head;

            Vector2f starts[2];
            Vector2f ends[2];

            while (segment)
            {
              PolylineSegment* const next_segment = segment->next;
              Vector2f               nxt_starts[2];

              if (segment == segments.head)
              {
                starts[0] = path_starts[0];
                starts[1] = path_starts[1];
              }

              if (segment == segments.tail)
              {
                ends[0] = path_ends[0];
                ends[1] = path_ends[1];
              }
              else
              {
                pushJoint(
                 *segment,
                 *next_segment,
                 join_style,
                 ends[0],
                 ends[1],
                 nxt_starts[0],
                 nxt_starts[1],
                 is_overlap_allowed);
              }

              const auto [vertex_id, verts] = result.requestVertices(vertex_stream, 4);

              verts[0] = {starts[0], {0.0f, 0.0f}, k_UnassignedColor};
              verts[1] = {starts[1], {0.0f, 0.0f}, k_UnassignedColor};
              verts[2] = {ends[0], {0.0f, 0.0f}, k_UnassignedColor};
              verts[3] = {ends[1], {0.0f, 0.0f}, k_UnassignedColor};

              result.pushTriIndex(global_index_offset, index_stream, vertex_id + 0, vertex_id + 2, vertex_id + 1);
              result.pushTriIndex(global_index_offset, index_stream, vertex_id + 2, vertex_id + 3, vertex_id + 1);

              segment   = next_segment;
              starts[0] = nxt_starts[0];
              starts[1] = nxt_starts[1];
            }
          }
        }

        break;
      }
      case Render2DCommandType::FillTriangles:
      {
        const auto* const typed_command = static_cast<const Render2DFillTriangles*>(command);

        result.num_vertices += typed_command->num_points;
        result.num_indices += typed_command->num_indices;
        break;
      }
      case Render2DCommandType::Text:
      {
        const auto* const typed_command = static_cast<const Render2DText*>(command);

        result.num_vertices += typed_command->num_codepoints * k_NumVertRect;
        result.num_indices += typed_command->num_codepoints * k_NumIdxRect;
        break;
      }
        bfInvalidDefaultCase();
    }

    return result;
  }

  void CommandBuffer2D::writeVertices(const DestVerts& dest, const BaseRender2DCommand_* command, VertIdxCountResult& counts)
  {
    struct VertexWrite
    {
      UIVertex2D* v;
      UIIndexType id;
    };

    struct VertexWriter
    {
      UIIndexType  vertex_offset;
      UIVertex2D*  next_vertex;
      UIIndexType* next_index;
      const Brush* brush;

      void addRect(AxisQuad rect)
      {
        const VertexWrite v = getVerts(k_NumVertRect);

        Vector2f uvs[] =
         {
          {0.0f, 1.0f},
          {1.0f, 1.0f},
          {0.0f, 0.0f},
          {1.0f, 0.0f},
         };

        bfColor4u colors[4];

        for (int i = 0; i < int(k_NumVertRect); ++i)
        {
          const BrushSampleResult brush_sample = brush->sample(uvs[i], i);

          uvs[i]    = brush_sample.remapped_uv;
          colors[i] = bfColor4u_fromColor4f(brush_sample.color);
        }

        v.v[0] = {rect.v0(), uvs[0], colors[0]};
        v.v[1] = {rect.v1(), uvs[1], colors[1]};
        v.v[2] = {rect.v2(), uvs[2], colors[2]};
        v.v[3] = {rect.v3(), uvs[3], colors[3]};

        pushTriIndex(v.id + 0, v.id + 2, v.id + 1);
        pushTriIndex(v.id + 0, v.id + 3, v.id + 2);
      }

      void addArc(const Vector2f& pos, float radius, float start_angle, float arc_angle)
      {
        const UIIndexType       num_segments      = calculateNumSegmentsForArc(radius);
        const float             theta             = arc_angle / float(num_segments);
        const float             tangential_factor = std::tan(theta);
        const float             radial_factor     = std::cos(theta);
        const VertexWrite       v                 = getVerts(num_segments * 2 + 1);
        float                   x_uv              = std::cos(start_angle);
        float                   y_uv              = std::sin(start_angle);
        float                   x                 = x_uv * radius;
        float                   y                 = y_uv * radius;
        UIIndexType             current_vertex    = 0;
        const Vector2f          middle_uv         = {0.5f, 0.5f};
        const BrushSampleResult middle_sample     = brush->sample(middle_uv, 0);

        v.v[current_vertex++] = {pos, middle_uv, bfColor4u_fromColor4f(middle_sample.color)};

        for (UIIndexType i = 0; i < num_segments; ++i)
        {
          const UIIndexType p0_index = current_vertex;
          {
            const Vector2f          p0        = Vector2f{x + pos.x, y + pos.y};
            const Vector2f          p0_uv     = {x_uv, y_uv};
            const BrushSampleResult p0_sample = brush->sample(p0_uv, i);
            v.v[current_vertex++]             = {p0, p0_uv, bfColor4u_fromColor4f(p0_sample.color)};
          }

          const float tx    = -y;
          const float ty    = x;
          const float tx_uv = -y_uv;
          const float ty_uv = x_uv;

          x += tx * tangential_factor;
          y += ty * tangential_factor;
          x *= radial_factor;
          y *= radial_factor;

          x_uv += tx_uv * tangential_factor;
          y_uv += ty_uv * tangential_factor;
          x_uv *= radial_factor;
          y_uv *= radial_factor;

          const UIIndexType p1_index = current_vertex;
          {
            const Vector2f          p1        = Vector2f{x + pos.x, y + pos.y};
            const Vector2f          p1_uv     = {x_uv, y_uv};
            const BrushSampleResult p1_sample = brush->sample(p1_uv, i);

            v.v[current_vertex++] = {p1, {0.0f, 0.0f}, bfColor4u_fromColor4f(p1_sample.color)};
          }

          pushTriIndex(v.id, v.id + p1_index, v.id + p0_index);
        }
      }

      VertexWrite getVerts(UIIndexType num_verts)
      {
        UIVertex2D* const result_v  = next_vertex;
        const UIIndexType result_id = vertex_offset;

        next_vertex += num_verts;
        vertex_offset += num_verts;

        return {result_v, result_id};
      }

      void pushTriIndex(UIIndexType index0, UIIndexType index1, UIIndexType index2)
      {
        *next_index++ = index0;
        *next_index++ = index1;
        *next_index++ = index2;
      }
    };

    VertexWriter writer = {dest.vertex_offset, dest.vertex_buffer_ptr, dest.index_buffer_ptr, command->brush};

    switch (command->type)
    {
      case Render2DCommandType::FillRect:
      {
        const auto* const typed_command = static_cast<const Render2DFillRect*>(command);

        writer.addRect(typed_command->rect);
        break;
      }
      case Render2DCommandType::FillRoundedRect:
      {
        const auto* const typed_command = static_cast<const Render2DFillRoundedRect*>(command);

        //
        // Drawing Rounded Rectangles
        //
        // (Two views since Middle And Inner Rect Overlap)
        //
        //    @@MMMMMMMMMM@@      @@----------@@
        //   @@@MMMMMMMMMM@@@    @@@----------@@@
        //   LLLMMMMMMMMMMRRR    ---IIIIIIIIII---
        //   LLLMMMMMMMMMMRRR    ---IIIIIIIIII---
        //   LLLMMMMMMMMMMRRR    ---IIIIIIIIII---
        //   @@@MMMMMMMMMM@@@    @@@----------@@@
        //    @@MMMMMMMMMM@@      @@----------@@
        //
        // Key:
        //   @ = Rounded Corners
        //   M = Middle Rect
        //   L = Left Rect
        //   R = Right Rect
        //   I = Inner Rect
        //

        const AxisQuad& rect          = typed_command->rect;
        const float     border_radius = typed_command->border_radius;

        // Fast path zero border radius
        if (!math::isAlmostEqual(border_radius, 0.0f))
        {
          const float    rect_width         = rect.width();
          const float    rect_height        = rect.height();
          const float    border_radius_x2   = border_radius * 2.0f;
          const float    middle_rect_width  = rect_width - border_radius_x2;
          const float    side_pieces_height = rect_height - border_radius_x2;
          const AxisQuad middle_rect        = rect.mutated({border_radius, 0.0f}, middle_rect_width, rect_height);
          const AxisQuad left_rect          = rect.mutated({0.0f, border_radius}, border_radius, side_pieces_height);
          const AxisQuad right_rect         = rect.mutated({middle_rect_width + border_radius, border_radius}, border_radius, side_pieces_height);
          const AxisQuad inner_rect         = rect.mutated({border_radius, border_radius}, middle_rect_width, side_pieces_height);
          const Vector2f tl_corner_pos      = inner_rect.position;
          const Vector2f tr_corner_pos      = tl_corner_pos + inner_rect.x_axis;
          const Vector2f br_corner_pos      = tr_corner_pos + inner_rect.y_axis;
          const Vector2f bl_corner_pos      = tl_corner_pos + inner_rect.y_axis;

          writer.addRect(middle_rect);
          writer.addRect(left_rect);
          writer.addRect(right_rect);
          writer.addArc(tl_corner_pos, border_radius, k_PI, k_HalfPI);
          writer.addArc(tr_corner_pos, border_radius, -k_HalfPI, k_HalfPI);
          writer.addArc(bl_corner_pos, border_radius, k_HalfPI, k_HalfPI);
          writer.addArc(br_corner_pos, border_radius, 0.0f, k_HalfPI);
        }
        else
        {
          writer.addRect(rect);
        }

        break;
      }
      case Render2DCommandType::BlurredRect:
      {
        const auto* const  typed_command         = static_cast<const Render2DBlurredRect*>(command);
        const float        shadow_sigma          = typed_command->shadow_sigma;
        const float        shadow_border_size    = shadow_sigma * 3.0f;
        const Vector2f     shadow_border_size_v2 = Vector2f{shadow_border_size};
        const Rect2f&      box                   = typed_command->rect;
        auto* const        verts                 = dest.shadow_vertex_buffer_ptr;
        const Vector2f     tl                    = box.topLeft() - shadow_border_size_v2;
        const Vector2f     tr                    = box.topRight() + Vector2f{shadow_border_size, -shadow_border_size};
        const Vector2f     bl                    = box.bottomLeft() + Vector2f{-shadow_border_size, shadow_border_size};
        const Vector2f     br                    = box.bottomRight() + shadow_border_size_v2;
        const float* const border_radii          = typed_command->border_radii;
        const bfColor4f    color_tl              = typed_command->brush->sample({0.0f, 1.0f}, 0u).color;
        const bfColor4f    color_tr              = typed_command->brush->sample({1.0f, 1.0f}, 1u).color;
        const bfColor4f    color_bl              = typed_command->brush->sample({0.0f, 0.0f}, 2u).color;
        const bfColor4f    color_br              = typed_command->brush->sample({1.0f, 0.0f}, 3u).color;

        verts[0] = {tl, shadow_sigma, border_radii[0], box, bfColor4u_fromColor4f(color_tl)};
        verts[1] = {tr, shadow_sigma, border_radii[1], box, bfColor4u_fromColor4f(color_tr)};
        verts[2] = {br, shadow_sigma, border_radii[2], box, bfColor4u_fromColor4f(color_bl)};
        verts[3] = {bl, shadow_sigma, border_radii[3], box, bfColor4u_fromColor4f(color_br)};

        dest.shadow_index_buffer_ptr[0] = dest.shadow_vertex_offset + 0u;
        dest.shadow_index_buffer_ptr[1] = dest.shadow_vertex_offset + 2u;
        dest.shadow_index_buffer_ptr[2] = dest.shadow_vertex_offset + 1u;
        dest.shadow_index_buffer_ptr[3] = dest.shadow_vertex_offset + 0u;
        dest.shadow_index_buffer_ptr[4] = dest.shadow_vertex_offset + 3u;
        dest.shadow_index_buffer_ptr[5] = dest.shadow_vertex_offset + 2u;
        break;
      }
      case Render2DCommandType::NineSliceRect:
      {
        // const auto* const typed_command = static_cast<const Render2DNineSliceRect*>(command);

        assert(false);  // TODO(SR):
        break;
      }
      case Render2DCommandType::FillArc:
      {
        const auto* const typed_command = static_cast<const Render2DFillArc*>(command);

        writer.addArc(typed_command->position, typed_command->radius, typed_command->start_angle, typed_command->arc_angle);
        break;
      }
      case Render2DCommandType::Polyline:
      {
        // const auto* const typed_command = static_cast<const Render2DPolyline*>(command);

        // TODO(SR): UV Mapping and brush sampling

        std::memcpy(dest.vertex_buffer_ptr, counts.precalculated_vertices, sizeof(UIVertex2D) * counts.num_vertices);
        std::memcpy(dest.index_buffer_ptr, counts.precalculated_indices, sizeof(UIIndexType) * counts.num_indices);
        break;
      }
      case Render2DCommandType::FillTriangles:
      {
        // const auto* const typed_command = static_cast<const Render2DFillTriangles*>(command);

        assert(false);  // TODO(SR):

        //std::memcpy(dest.vertex_buffer_ptr, typed_command->points, sizeof(UIVertex2D) * counts.num_vertices);
        //std::memcpy(dest.index_buffer_ptr, counts.precalculated_indices, sizeof(UIIndexType) * counts.num_indices);
        break;
      }
      case Render2DCommandType::Text:
      {
        const auto* const typed_command = static_cast<const Render2DText*>(command);

        assert(typed_command->brush->type == Brush::Font);

        const Vector2f& pos       = typed_command->position;
        const char*     utf8_text = typed_command->utf8_text.bgn;
        float           x         = pos.x;
        float           y         = pos.y;
        PainterFont*    font      = typed_command->brush->font_data.font;
        const bfColor4u color     = bfColor4u_fromColor4f(typed_command->brush->font_data.tint);

        while (*utf8_text)
        {
          const bool is_backslash_r = *utf8_text == '\r';

          if (is_backslash_r || *utf8_text == '\n')
          {
            x = pos.x;
            y += fontNewlineHeight(font->font);

            ++utf8_text;

            // Handle Window's '\r\n'
            if (is_backslash_r && *utf8_text == '\n')
            {
              ++utf8_text;
            }

            continue;
          }

          const auto        res       = utf8Codepoint(utf8_text);
          const CodePoint   codepoint = res.codepoint;
          const GlyphInfo&  glyph     = fontGetGlyphInfo(font->font, codepoint);
          const VertexWrite v         = writer.getVerts(4);
          const Vector2f    p         = Vector2f{x, y} + Vector2f{glyph.offset[0], glyph.offset[1]};
          const Vector2f    size_x    = {float(glyph.bmp_box[1].x), 0.0f};
          const Vector2f    size_y    = {0.0f, float(glyph.bmp_box[1].y)};
          const Vector2f    size_xy   = {size_x.x, size_y.y};
          const Vector2f    p0        = p;
          const Vector2f    p1        = p + size_x;
          const Vector2f    p2        = p + size_xy;
          const Vector2f    p3        = p + size_y;

          v.v[0] = {p0, {glyph.uvs[0], glyph.uvs[1]}, color};
          v.v[1] = {p1, {glyph.uvs[2], glyph.uvs[1]}, color};
          v.v[2] = {p2, {glyph.uvs[2], glyph.uvs[3]}, color};
          v.v[3] = {p3, {glyph.uvs[0], glyph.uvs[3]}, color};

          writer.pushTriIndex(v.id + 0, v.id + 1, v.id + 2);
          writer.pushTriIndex(v.id + 0, v.id + 2, v.id + 3);

          utf8_text = res.endpos;
          x += glyph.advance_x;

          // Not at the end
          if (*utf8_text)
          {
            // TODO(SR): This can be optimized. Since that "utf8Codepoint(utf8_text)" call calculates what we need next time through the loop.
            x += fontAdditionalAdvance(font->font, codepoint, utf8Codepoint(utf8_text).codepoint);
          }
        }

        font->device = render_data.device;

        const auto&   frame_info    = bfGfxContext_getFrameInfo(render_data.ctx);
        DynamicAtlas& current_atlas = font->gpu_atlas[frame_info.frame_index];

        for (DynamicAtlas& atlas : font->gpu_atlas)
        {
          atlas.needs_upload = atlas.needs_upload || fontAtlasNeedsUpload(font->font);
          atlas.needs_resize = atlas.needs_resize || fontAtlasHasResized(font->font);
        }

        fontResetAtlasStatus(font->font);

        if (current_atlas.needs_upload)
        {
          if (current_atlas.needs_resize)
          {
            bfGfxDevice_release(render_data.device, current_atlas.handle);
            current_atlas.handle       = nullptr;
            current_atlas.needs_resize = false;
          }

          const auto* const pixmap = fontPixelMap(font->font);

          if (!current_atlas.handle)
          {
            current_atlas.handle =
             gfx::createTexture(
              render_data.device,
              bfTextureCreateParams_init2D(BF_IMAGE_FORMAT_R8G8B8A8_UNORM, pixmap->width, pixmap->height),
              k_SamplerNearestClampToEdge,
              pixmap->pixels,
              pixmap->sizeInBytes());
          }
          else
          {
            const int32_t  offset[3] = {0, 0, 0};
            const uint32_t sizes[3]  = {pixmap->width, pixmap->height, 1};

            bfTexture_loadDataRange(
             current_atlas.handle,
             pixmap->pixels,
             pixmap->sizeInBytes(),
             offset,
             sizes);
          }

          current_atlas.needs_upload = false;
        }
        break;
      }
        bfInvalidDefaultCase();
    }
  }

  Vector2f calculateTextSize(StringRange utf8_string, PainterFont* font, UIIndexType& num_codepoints)
  {
    float       max_width      = 0.0f;
    float       current_width  = 0.0f;
    float       current_height = fontNewlineHeight(font->font);
    const char* utf8_text      = utf8_string.begin();
    const char* utf8_text_end  = utf8_string.end();

    num_codepoints = 0;

    while (utf8_text != utf8_text_end)
    {
      const bool is_backslash_r = *utf8_text == '\r';

      if (is_backslash_r || *utf8_text == '\n')
      {
        max_width     = std::max(current_width, max_width);
        current_width = 0.0f;
        current_height += fontNewlineHeight(font->font);

        ++utf8_text;

        // Handle Window's '\r\n'
        if (is_backslash_r && *utf8_text == '\n')
        {
          ++utf8_text;
        }

        continue;
      }

      const auto       res       = utf8Codepoint(utf8_text);
      const CodePoint  codepoint = res.codepoint;
      const GlyphInfo& glyph     = fontGetGlyphInfo(font->font, codepoint);

      utf8_text = res.endpos;
      current_width += glyph.advance_x;

      if (utf8_text != utf8_text_end)  // Not at the end
      {
        // TODO(SR): This can be optimized. Since that "utf8Codepoint(utf8_text)" call calculates what we need next time through the loop.
        current_width += fontAdditionalAdvance(font->font, codepoint, utf8Codepoint(utf8_text).codepoint);
      }

      ++num_codepoints;
    }

    return {std::max(current_width, max_width), current_height};
  }

  Vector2f calculateTextSize(const char* utf8_text, PainterFont* font)
  {
    float max_width      = 0.0f;
    float current_width  = 0.0f;
    float current_height = fontNewlineHeight(font->font);

    while (*utf8_text)
    {
      const bool is_backslash_r = *utf8_text == '\r';

      if (is_backslash_r || *utf8_text == '\n')
      {
        max_width     = std::max(current_width, max_width);
        current_width = 0.0f;
        current_height += fontNewlineHeight(font->font);

        ++utf8_text;

        // Handle Window's '\r\n'
        if (is_backslash_r && *utf8_text == '\n')
        {
          ++utf8_text;
        }

        continue;
      }

      const auto       res       = utf8Codepoint(utf8_text);
      const CodePoint  codepoint = res.codepoint;
      const GlyphInfo& glyph     = fontGetGlyphInfo(font->font, codepoint);

      utf8_text = res.endpos;
      current_width += glyph.advance_x;

      if (*utf8_text)  // Not at the end
      {
        // TODO(SR): This can be optimized. Since that "utf8Codepoint(utf8_text)" call calculates what we need next time through the loop.
        current_width += fontAdditionalAdvance(font->font, codepoint, utf8Codepoint(utf8_text).codepoint);
      }
    }

    return {std::max(current_width, max_width), current_height};
  }

  void Gfx2DPainter::pushText(const Vector2f& pos, const char* utf8_text, PainterFont* font)
  {
    const bfColor4u color           = bfColor4u_fromUint32(BIFROST_COLOR_BLACK);
    float           x               = pos.x;
    float           y               = pos.y;
    int             num_characters  = 0;
    UIIndexType     start_vertex_id = 0;

    while (*utf8_text)
    {
      const bool is_backslash_r = *utf8_text == '\r';

      if (is_backslash_r || *utf8_text == '\n')
      {
        x = pos.x;
        y += fontNewlineHeight(font->font);

        ++utf8_text;

        // Handle Window's '\r\n'
        if (is_backslash_r && *utf8_text == '\n')
        {
          ++utf8_text;
        }

        continue;
      }

      const auto       res          = utf8Codepoint(utf8_text);
      const CodePoint  codepoint    = res.codepoint;
      const GlyphInfo& glyph        = fontGetGlyphInfo(font->font, codepoint);
      const auto [vertex_id, verts] = requestVertices(4);

      // First time through the loop
      if (num_characters == 0)
      {
        start_vertex_id = vertex_id;
      }

      const Vector2f p       = Vector2f{x, y} + Vector2f{glyph.offset[0], glyph.offset[1]};
      const Vector2f size_x  = {float(glyph.bmp_box[1].x), 0.0f};
      const Vector2f size_y  = {0.0f, float(glyph.bmp_box[1].y)};
      const Vector2f size_xy = {size_x.x, size_y.y};
      const Vector2f p0      = p;
      const Vector2f p1      = p + size_x;
      const Vector2f p2      = p + size_xy;
      const Vector2f p3      = p + size_y;

      verts[0] = {p0, {glyph.uvs[0], glyph.uvs[1]}, color};
      verts[1] = {p1, {glyph.uvs[2], glyph.uvs[1]}, color};
      verts[2] = {p2, {glyph.uvs[2], glyph.uvs[3]}, color};
      verts[3] = {p3, {glyph.uvs[0], glyph.uvs[3]}, color};

      utf8_text = res.endpos;
      x += glyph.advance_x;

      // Not at the end
      if (*utf8_text)
      {
        // TODO(SR): This can be optimized. Since that "utf8Codepoint(utf8_text)" call calculates what we need next time through the loop.
        x += fontAdditionalAdvance(font->font, codepoint, utf8Codepoint(utf8_text).codepoint);
      }

      ++num_characters;
    }

    font->device = render_data.device;

    const auto&   frame_info    = bfGfxContext_getFrameInfo(render_data.ctx);
    DynamicAtlas& current_atlas = font->gpu_atlas[frame_info.frame_index * 0];

    for (DynamicAtlas& atlas : font->gpu_atlas)
    {
      atlas.needs_upload = atlas.needs_upload || fontAtlasNeedsUpload(font->font);
      atlas.needs_resize = atlas.needs_resize || fontAtlasHasResized(font->font);
    }

    fontResetAtlasStatus(font->font);

    if (current_atlas.needs_upload)
    {
      if (current_atlas.needs_resize)
      {
        bfGfxDevice_release(render_data.device, current_atlas.handle);
        current_atlas.handle       = nullptr;
        current_atlas.needs_resize = false;
      }

      const auto* const pixmap = fontPixelMap(font->font);

      if (!current_atlas.handle)
      {
        current_atlas.handle =
         gfx::createTexture(
          render_data.device,
          bfTextureCreateParams_init2D(BF_IMAGE_FORMAT_R8G8B8A8_UNORM, pixmap->width, pixmap->height),
          k_SamplerNearestClampToEdge,
          pixmap->pixels,
          pixmap->sizeInBytes());
      }
      else
      {
        const int32_t  offset[3] = {0, 0, 0};
        const uint32_t sizes[3]  = {pixmap->width, pixmap->height, 1};

        bfTexture_loadDataRange(
         current_atlas.handle,
         pixmap->pixels,
         pixmap->sizeInBytes(),
         offset,
         sizes);
      }

      current_atlas.needs_upload = false;
    }

    const auto old_texture = currentDrawCommand().texture;

    bindTexture(current_atlas.handle);

    for (int i = 0; i < num_characters; ++i)
    {
      const UIIndexType vertex_id = start_vertex_id + i * 4;

      pushTriIndex(vertex_id + 0, vertex_id + 1, vertex_id + 2);
      pushTriIndex(vertex_id + 0, vertex_id + 2, vertex_id + 3);
    }

    bindTexture(old_texture);
  }

  void Gfx2DPainter::render(bfGfxCommandListHandle command_list, int fb_width, int fb_height)
  {
    if (vertices.isEmpty() || indices.isEmpty())
    {
      return;
    }

    const bool               has_shadow           = !shadow_vertices.isEmpty() && !shadow_indices.isEmpty();
    uint64_t                 vertex_buffer_offset = 0;
    const auto&              frame_info           = bfGfxContext_getFrameInfo(render_data.ctx);
    Gfx2DPerFrameRenderData& frame_data           = render_data.frame_datas[frame_info.frame_index];

    {
      render_data.reserve(frame_info.frame_index, vertices.size(), indices.size());

      UIVertex2D*  vertex_buffer_ptr = static_cast<UIVertex2D*>(bfBuffer_map(frame_data.vertex_buffer, 0, k_bfBufferWholeSize));
      UIIndexType* index_buffer_ptr  = static_cast<UIIndexType*>(bfBuffer_map(frame_data.index_buffer, 0, k_bfBufferWholeSize));

      std::memcpy(vertex_buffer_ptr, vertices.data(), vertices.size() * sizeof(UIVertex2D));
      std::memcpy(index_buffer_ptr, indices.data(), indices.size() * sizeof(UIIndexType));

      bfBuffer_unMap(frame_data.vertex_buffer);
      bfBuffer_unMap(frame_data.index_buffer);
    }

    {
      auto&              ubo_buffer         = render_data.uniform;
      const bfBufferSize ubo_offset         = ubo_buffer.offset(frame_info);
      const bfBufferSize ubo_size           = sizeof(Gfx2DUniformData);
      Gfx2DUniformData*  uniform_buffer_ptr = static_cast<Gfx2DUniformData*>(bfBuffer_map(ubo_buffer.handle(), ubo_offset, ubo_size));

      static constexpr decltype(&Mat4x4_orthoVk) orthos_fns[] = {&Mat4x4_orthoVk, &Mat4x4_ortho};

      const float k_ScaleFactorDPI = 1.0f;  // TODO(SR): Need to grab this value based on what window is being drawn onto.

      orthos_fns[bfPlatformGetGfxAPI() == BIFROST_PLATFORM_GFX_OPENGL](
       &uniform_buffer_ptr->ortho_matrix,
       0.0f,
       float(fb_width) / k_ScaleFactorDPI,
       float(fb_height) / k_ScaleFactorDPI,
       0.0f,
       0.0f,
       1.0f);

      ubo_buffer.flushCurrent(frame_info);
      bfBuffer_unMap(ubo_buffer.handle());
    }

    bfGfxCmdList_setBlendSrc(command_list, 0, BF_BLEND_FACTOR_SRC_ALPHA);
    bfGfxCmdList_setBlendDst(command_list, 0, BF_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    bfGfxCmdList_setBlendSrcAlpha(command_list, 0, BF_BLEND_FACTOR_SRC_ALPHA);
    bfGfxCmdList_setBlendDstAlpha(command_list, 0, BF_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    bfGfxCmdList_setFrontFace(command_list, BF_FRONT_FACE_CW);
    bfGfxCmdList_setCullFace(command_list, BF_CULL_FACE_BACK);
    bfGfxCmdList_setDynamicStates(command_list, BF_PIPELINE_DYNAMIC_VIEWPORT | BF_PIPELINE_DYNAMIC_SCISSOR);
    bfGfxCmdList_setViewport(command_list, 0.0f, 0.0f, float(fb_width), float(fb_height), nullptr);
    bfGfxCmdList_setScissorRect(command_list, 0, 0, fb_width, fb_height);

    if (has_shadow)
    {
      render_data.reserveShadow(frame_info.frame_index, shadow_vertices.size(), shadow_indices.size());

      DropShadowVertex* vertex_buffer_ptr = static_cast<DropShadowVertex*>(bfBuffer_map(frame_data.vertex_shadow_buffer, 0, k_bfBufferWholeSize));
      UIIndexType*      index_buffer_ptr  = static_cast<UIIndexType*>(bfBuffer_map(frame_data.index_shadow_buffer, 0, k_bfBufferWholeSize));

      std::memcpy(vertex_buffer_ptr, shadow_vertices.data(), shadow_vertices.size() * sizeof(DropShadowVertex));
      std::memcpy(index_buffer_ptr, shadow_indices.data(), shadow_indices.size() * sizeof(UIIndexType));

      bfBuffer_unMap(frame_data.vertex_shadow_buffer);
      bfBuffer_unMap(frame_data.index_shadow_buffer);

      bfGfxCmdList_bindVertexDesc(command_list, render_data.vertex_layouts[1]);
      bfGfxCmdList_bindProgram(command_list, render_data.rounded_rect_shadow_program);

      {
        bfBufferSize ubo_offset = render_data.uniform.offset(frame_info);
        bfBufferSize ubo_sizes  = sizeof(Gfx2DUniformData);

        bfDescriptorSetInfo desc_set = bfDescriptorSetInfo_make();
        bfDescriptorSetInfo_addUniform(&desc_set, 0, 0, &ubo_offset, &ubo_sizes, &render_data.uniform.handle(), 1);

        bfGfxCmdList_bindDescriptorSet(command_list, k_GfxCameraSetIndex, &desc_set);
      }

      bfGfxCmdList_bindVertexBuffers(command_list, 0, &frame_data.vertex_shadow_buffer, 1, &vertex_buffer_offset);
      bfGfxCmdList_bindIndexBuffer(command_list, frame_data.index_shadow_buffer, 0, bfIndexTypeFromT<UIIndexType>());
      bfGfxCmdList_drawIndexed(command_list, UIIndexType(shadow_indices.size()), 0u, 0u);
    }

    bfGfxCmdList_bindVertexDesc(command_list, render_data.vertex_layouts[0]);
    bfGfxCmdList_bindProgram(command_list, render_data.shader_program);
    bfGfxCmdList_bindVertexBuffers(command_list, 0, &frame_data.vertex_buffer, 1, &vertex_buffer_offset);
    bfGfxCmdList_bindIndexBuffer(command_list, frame_data.index_buffer, 0, bfIndexTypeFromT<UIIndexType>());

    for (auto& draw_cmd : draw_commands)
    {
      bfBufferSize ubo_offset = render_data.uniform.offset(frame_info);
      bfBufferSize ubo_sizes  = sizeof(Gfx2DUniformData);

      bfDescriptorSetInfo cam_desc_set = bfDescriptorSetInfo_make();
      bfDescriptorSetInfo mat_desc_set = bfDescriptorSetInfo_make();

      bfDescriptorSetInfo_addUniform(&cam_desc_set, 0, 0, &ubo_offset, &ubo_sizes, &render_data.uniform.handle(), 1);
      bfDescriptorSetInfo_addTexture(&mat_desc_set, 0, 0, &draw_cmd.texture, 1);

      bfGfxCmdList_bindDescriptorSet(command_list, k_GfxCameraSetIndex, &cam_desc_set);
      bfGfxCmdList_bindDescriptorSet(command_list, k_GfxMaterialSetIndex, &mat_desc_set);
      bfGfxCmdList_drawIndexed(command_list, draw_cmd.num_indices, draw_cmd.first_index, 0u);
    }
  }

  std::pair<UIIndexType, Gfx2DPainter::SafeVertexIndexer<UIVertex2D>> Gfx2DPainter::requestVertices(UIIndexType num_verts)
  {
    const UIIndexType start_id = UIIndexType(vertices.size());

    vertices.resize(std::size_t(start_id) + num_verts);

    return {start_id, SafeVertexIndexer<UIVertex2D>{num_verts, vertices.data() + start_id}};
  }

  void Gfx2DPainter::pushTriIndex(UIIndexType index0, UIIndexType index1, UIIndexType index2)
  {
    assert(index0 < vertices.size() && index1 < vertices.size() && index2 < vertices.size());

    indices.push(index0);
    indices.push(index1);
    indices.push(index2);

    currentDrawCommand().num_indices += 3;
  }

  std::pair<UIIndexType, Gfx2DPainter::SafeVertexIndexer<DropShadowVertex>> Gfx2DPainter::requestVertices2(UIIndexType num_verts)
  {
    const UIIndexType start_id = UIIndexType(shadow_vertices.size());

    shadow_vertices.resize(std::size_t(start_id) + num_verts);

    return {start_id, SafeVertexIndexer<DropShadowVertex>{num_verts, shadow_vertices.data() + start_id}};
  }

  void Gfx2DPainter::pushTriIndex2(UIIndexType index0, UIIndexType index1, UIIndexType index2)
  {
    assert(index0 < shadow_vertices.size() && index1 < shadow_vertices.size() && index2 < shadow_vertices.size());

    shadow_indices.push(index0);
    shadow_indices.push(index1);
    shadow_indices.push(index2);
  }
}  // namespace bf
