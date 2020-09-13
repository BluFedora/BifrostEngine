#include "bf/gfx2D/bf_painter.hpp"

#include "bf/Platform.h"  // bfPlatformGetGfxAPI
#include "bf/Text.hpp"    // CodePoint, Font

#include "bifrost/memory/bifrost_proxy_allocator.hpp"

#include <cmath>

namespace bf
{
  static const bfTextureSamplerProperties k_SamplerNearestClampToEdge = bfTextureSamplerProperties_init(BIFROST_SFM_NEAREST, BIFROST_SAM_CLAMP_TO_EDGE);
  static constexpr bfColor4u              k_ColorWhite4u              = {0xFF, 0xFF, 0xFF, 0xFF};
  static constexpr float                  k_ArcSmoothingFactor        = 3.5f; /*!< This is just about the minimum before quality of the curves degrade. */

  void Gfx2DPerFrameRenderData::reserve(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
  {
    if (!vertex_buffer || bfBuffer_size(vertex_buffer) < vertex_size)
    {
      bfGfxDevice_release(device, vertex_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = vertex_size;
      buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

      vertex_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }

    if (!index_buffer || bfBuffer_size(index_buffer) < indices_size)
    {
      bfGfxDevice_release(device, index_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = indices_size;
      buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_INDEX_BUFFER;

      index_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }
  }

  void Gfx2DPerFrameRenderData::reserveShadow(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
  {
    if (!vertex_shadow_buffer || bfBuffer_size(vertex_shadow_buffer) < vertex_size)
    {
      bfGfxDevice_release(device, vertex_shadow_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = vertex_size;
      buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

      vertex_shadow_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }

    if (!index_shadow_buffer || bfBuffer_size(index_shadow_buffer) < indices_size)
    {
      bfGfxDevice_release(device, index_shadow_buffer);

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = indices_size;
      buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_INDEX_BUFFER;

      index_shadow_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }
  }

  Gfx2DRenderData::Gfx2DRenderData(IMemoryManager& memory, GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics) :
    memory{memory},
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
    frame_datas{nullptr},
    uniform{}
  {
    // Vertex Layout
    vertex_layouts[0] = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(vertex_layouts[0], 0, sizeof(UIVertex2D));
    bfVertexLayout_addVertexLayout(vertex_layouts[0], 0, BIFROST_VFA_FLOAT32_2, offsetof(UIVertex2D, pos));
    bfVertexLayout_addVertexLayout(vertex_layouts[0], 0, BIFROST_VFA_FLOAT32_2, offsetof(UIVertex2D, uv));
    bfVertexLayout_addVertexLayout(vertex_layouts[0], 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(UIVertex2D, color));

    vertex_layouts[1] = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(vertex_layouts[1], 0, sizeof(DropShadowVertex));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BIFROST_VFA_FLOAT32_2, offsetof(DropShadowVertex, pos));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BIFROST_VFA_FLOAT32_1, offsetof(DropShadowVertex, shadow_sigma));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BIFROST_VFA_FLOAT32_1, offsetof(DropShadowVertex, corner_radius));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BIFROST_VFA_FLOAT32_4, offsetof(DropShadowVertex, box));
    bfVertexLayout_addVertexLayout(vertex_layouts[1], 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(DropShadowVertex, color));

    // Shaders
    vertex_shader   = glsl_compiler.createModule(device, "assets/shaders/gfx2D/textured.vert.glsl");
    fragment_shader = glsl_compiler.createModule(device, "assets/shaders/gfx2D/textured.frag.glsl");
    shader_program  = gfx::createShaderProgram(device, 2, vertex_shader, fragment_shader, "Graphics2D.Painter");

    bfShaderProgram_addImageSampler(shader_program, "u_Texture", 0, 0, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addUniformBuffer(shader_program, "u_Set0", 0, 1, 1, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(shader_program);

    shadow_modules[0]           = glsl_compiler.createModule(device, "assets/shaders/gfx2D/drop_shadow.vert.glsl");
    shadow_modules[1]           = glsl_compiler.createModule(device, "assets/shaders/gfx2D/drop_shadow_rect.frag.glsl");
    shadow_modules[2]           = glsl_compiler.createModule(device, "assets/shaders/gfx2D/drop_shadow_rounded_rect.frag.glsl");
    rect_shadow_program         = gfx::createShaderProgram(device, 1, shadow_modules[0], shadow_modules[1], "Graphics2D.ShadowRect");
    rounded_rect_shadow_program = gfx::createShaderProgram(device, 1, shadow_modules[0], shadow_modules[2], "Graphics2D.ShadowRoundedRect");

    bfShaderProgram_addUniformBuffer(rect_shadow_program, "u_Set0", 0, 0, 1, BIFROST_SHADER_STAGE_VERTEX);
    bfShaderProgram_addUniformBuffer(rounded_rect_shadow_program, "u_Set0", 0, 0, 1, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(rect_shadow_program);
    bfShaderProgram_compile(rounded_rect_shadow_program);

    // White Texture
    white_texture = gfx::createTexture(device, bfTextureCreateParams_init2D(BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM, 1, 1), k_SamplerNearestClampToEdge, &k_ColorWhite4u, sizeof(k_ColorWhite4u));

    // Frame Datas
    const auto& frame_info = bfGfxContext_getFrameInfo(ctx);

    num_frame_datas = int(frame_info.num_frame_indices);
    frame_datas     = memory.allocateArray<Gfx2DPerFrameRenderData>(num_frame_datas);

    // Uniform Buffer
    const auto device_info = bfGfxDevice_limits(device);

    uniform.create(device, BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_UNIFORM_BUFFER, frame_info, device_info.uniform_buffer_offset_alignment);
  }

  void Gfx2DRenderData::reserve(int index, size_t vertex_size, size_t indices_size) const
  {
    assert(index < num_frame_datas);

    Gfx2DPerFrameRenderData& frame_data = frame_datas[index];

    frame_data.reserve(device, vertex_size * sizeof(UIVertex2D), indices_size * sizeof(UIIndexType));
  }

  void Gfx2DRenderData::reserveShadow(int index, size_t vertex_size, size_t indices_size) const
  {
    assert(index < num_frame_datas);

    Gfx2DPerFrameRenderData& frame_data = frame_datas[index];

    frame_data.reserveShadow(device, vertex_size * sizeof(DropShadowVertex), indices_size * sizeof(UIIndexType));
  }

  Gfx2DRenderData::~Gfx2DRenderData()
  {
    uniform.destroy(device);

    forEachBuffer([this](Gfx2DPerFrameRenderData& data) {
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

  PainterFont::PainterFont(IMemoryManager& memory, const char* filename, float size) :
    font{makeFont(memory, filename, size)},
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
  }

  Gfx2DPainter::Gfx2DPainter(IMemoryManager& memory, GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics) :
    render_data{memory, glsl_compiler, graphics},
    vertices{memory},
    indices{memory},
    shadow_vertices{memory},
    shadow_indices{memory},
    tmp_memory_backing{},
    tmp_memory{tmp_memory_backing, sizeof(tmp_memory_backing)},
    draw_commands{memory}
  {
  }

  void Gfx2DPainter::reset()
  {
#if bfGfx2DSafeVertexIndexing
    SafeVertexIndexer<UIVertex2D>::s_TmpMemory.clear();
    SafeVertexIndexer<DropShadowVertex>::s_TmpMemory.clear();
#endif

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

    // const bool  not_full_circle   = arc_angle < k_TwoPI;
    const auto  num_segments      = UIIndexType(k_ArcSmoothingFactor * std::sqrt(radius));
    const float theta             = arc_angle / float(num_segments);
    const float tangential_factor = std::tan(theta);
    const float radial_factor     = std::cos(theta);
    const auto [vertex_id, verts] = requestVertices(num_segments * 2 + 1);
    const auto  color_4u          = bfColor4u_fromUint32(color);
    float       x                 = std::cos(start_angle) * radius;
    float       y                 = std::sin(start_angle) * radius;
    UIIndexType current_vertex    = 0;

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
    const auto           num_segments      = UIIndexType(k_ArcSmoothingFactor * std::sqrt(radius));
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
    static constexpr float k_TenDegAsRad   = k_DegToRad * 10.0f;
    static constexpr float k_MinAngleMiter = k_DegToRad * 15.0f;

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
        return vec::normalized(directionUnformalized());
      }

      Vector2f directionUnformalized() const
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
        const auto r      = directionUnformalized();
        const auto s      = rhs.directionUnformalized();
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
          const auto         x1        = dirs[0].x;
          const auto         x2        = dirs[1].x;
          const auto         y1        = dirs[0].y;
          const auto         y2        = dirs[1].y;
          const auto         clockwise = x1 * y2 - x2 * y1 < 0;
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

  void Gfx2DPainter::pushText(const Vector2f& pos, const char* utf8_text, PainterFont* font)
  {
    const bfColor4u color           = bfColor4u_fromUint32(BIFROST_COLOR_BLACK);  // bfColor4u_fromUint32(BIFROST_COLOR_SALMON);
    float           x               = pos.x;
    float           y               = pos.y;  // TODO(SR): Handle New line
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
      if (!num_characters)
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
        x += fontAdditionalAdvance(font->font, codepoint, utf8Codepoint(utf8_text).codepoint);
      }

      ++num_characters;
    }

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
        current_atlas.handle = nullptr;

        current_atlas.needs_resize = false;
      }

      const auto* const pixmap = fontPixelMap(font->font);

      if (!current_atlas.handle)
      {
        current_atlas.handle =
         gfx::createTexture(
          render_data.device,
          bfTextureCreateParams_init2D(BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM, pixmap->width, pixmap->height),
          k_SamplerNearestClampToEdge,
          pixmap->pixels,
          pixmap->sizeInBytes());
      }
      else
      {
        const int32_t  offset[3] = {0, 0, 0};
        const uint32_t sizes[3]  = {pixmap->width, pixmap->height, 0};

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

      UIVertex2D*  vertex_buffer_ptr = static_cast<UIVertex2D*>(bfBuffer_map(frame_data.vertex_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE));
      UIIndexType* index_buffer_ptr  = static_cast<UIIndexType*>(bfBuffer_map(frame_data.index_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE));

      std::memcpy(vertex_buffer_ptr, vertices.data(), vertices.size() * sizeof(UIVertex2D));
      std::memcpy(index_buffer_ptr, indices.data(), indices.size() * sizeof(UIIndexType));

      bfBuffer_unMap(frame_data.vertex_buffer);
      bfBuffer_unMap(frame_data.index_buffer);
    }

    {
      auto&              ubo_buffer         = render_data.uniform;
      const bfBufferSize ubo_offset         = ubo_buffer.offset(frame_info);
      const bfBufferSize ubo_size           = sizeof(Gfx2DUnifromData);
      Gfx2DUnifromData*  uniform_buffer_ptr = static_cast<Gfx2DUnifromData*>(bfBuffer_map(ubo_buffer.handle(), ubo_offset, ubo_size));

      static constexpr decltype(&Mat4x4_orthoVk) orthos_fns[] = {&Mat4x4_orthoVk, &Mat4x4_ortho};

      orthos_fns[bfPlatformGetGfxAPI() == BIFROST_PLATFORM_GFX_OPENGL](&uniform_buffer_ptr->ortho_matrix, 0.0f, float(fb_width), float(fb_height), 0.0f, 0.0f, 1.0f);

      ubo_buffer.flushCurrent(frame_info);
      bfBuffer_unMap(ubo_buffer.handle());
    }

    bfGfxCmdList_setFrontFace(command_list, BIFROST_FRONT_FACE_CW);
    bfGfxCmdList_setCullFace(command_list, BIFROST_CULL_FACE_BACK);
    bfGfxCmdList_setDynamicStates(command_list, BIFROST_PIPELINE_DYNAMIC_VIEWPORT | BIFROST_PIPELINE_DYNAMIC_SCISSOR);
    bfGfxCmdList_setViewport(command_list, 0.0f, 0.0f, float(fb_width), float(fb_height), nullptr);
    bfGfxCmdList_setScissorRect(command_list, 0, 0, fb_width, fb_height);

    if (has_shadow)
    {
      render_data.reserveShadow(frame_info.frame_index, shadow_vertices.size(), shadow_indices.size());

      DropShadowVertex* vertex_buffer_ptr = static_cast<DropShadowVertex*>(bfBuffer_map(frame_data.vertex_shadow_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE));
      UIIndexType*      index_buffer_ptr  = static_cast<UIIndexType*>(bfBuffer_map(frame_data.index_shadow_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE));

      std::memcpy(vertex_buffer_ptr, shadow_vertices.data(), shadow_vertices.size() * sizeof(DropShadowVertex));
      std::memcpy(index_buffer_ptr, shadow_indices.data(), shadow_indices.size() * sizeof(UIIndexType));

      bfBuffer_unMap(frame_data.vertex_shadow_buffer);
      bfBuffer_unMap(frame_data.index_shadow_buffer);

      bfGfxCmdList_bindVertexDesc(command_list, render_data.vertex_layouts[1]);
      bfGfxCmdList_bindProgram(command_list, render_data.rounded_rect_shadow_program);

      {
        bfBufferSize ubo_offset = render_data.uniform.offset(frame_info);
        bfBufferSize ubo_sizes  = sizeof(Gfx2DUnifromData);

        bfDescriptorSetInfo desc_set = bfDescriptorSetInfo_make();
        bfDescriptorSetInfo_addUniform(&desc_set, 0, 0, &ubo_offset, &ubo_sizes, &render_data.uniform.handle(), 1);

        bfGfxCmdList_bindDescriptorSet(command_list, 0, &desc_set);
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
      bfBufferSize ubo_sizes  = sizeof(Gfx2DUnifromData);

      bfDescriptorSetInfo desc_set = bfDescriptorSetInfo_make();
      bfDescriptorSetInfo_addTexture(&desc_set, 0, 0, &draw_cmd.texture, 1);
      bfDescriptorSetInfo_addUniform(&desc_set, 1, 0, &ubo_offset, &ubo_sizes, &render_data.uniform.handle(), 1);

      bfGfxCmdList_bindDescriptorSet(command_list, 0, &desc_set);
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

    indices.reserve(indices.size() + 3);
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

    shadow_indices.reserve(shadow_indices.size() + 3);
    shadow_indices.push(index0);
    shadow_indices.push(index1);
    shadow_indices.push(index2);
  }
}  // namespace bf
