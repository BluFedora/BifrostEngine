/******************************************************************************/
/*!
 * @file   bf_draw_2d.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *    API for efficient drawing of `fancy` vector 2D graphics.
 *
 * @version 0.0.1
 * @date    2021-03-03
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#include "bf/gfx/bf_draw_2d.hpp"

#include "bf/Text.hpp"                 // CodePoint, Font
#include "bf/gfx/bf_render_queue.hpp"  // RenderQueue

#include <cmath>  // round

namespace bf
{
  //
  // Constants
  //

  static const bfTextureSamplerProperties k_SamplerNearestClampToEdge = bfTextureSamplerProperties_init(BF_SFM_NEAREST, BF_SAM_CLAMP_TO_EDGE);
  static constexpr bfColor4u              k_ColorWhite4u              = {0xFF, 0xFF, 0xFF, 0xFF};
  static constexpr float                  k_ArcSmoothingFactor        = 2.2f; /*!< This is just about the minimum before quality of the curves degrade. */
  static constexpr std::size_t            k_NumVertRect               = 4;
  static constexpr std::size_t            k_NumIdxRect                = 6;

  //
  // Helpers
  //

  static Rect2f boundsFromPoints(const Vector2f* points, std::size_t num_points)
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

  static UIIndexType calculateNumSegmentsForArc(float radius)
  {
    return UIIndexType(k_ArcSmoothingFactor * std::sqrt(radius));
  }

  static Vector2f remapUV(const AxisQuad& uv_remap, Vector2f uv)
  {
    return {
     vec::inverseLerp(uv_remap.position, uv_remap.position + uv_remap.x_axis, uv),
     vec::inverseLerp(uv_remap.position, uv_remap.position + uv_remap.y_axis, uv),
    };
  }

  void Gfx2DPerFrameRenderData::reserve(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
  {
    bfBufferCreateParams buffer_params;
    buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;

    if (!vertex_buffer || bfBuffer_size(vertex_buffer) < vertex_size)
    {
      bfGfxDevice_release(device, vertex_buffer);

      buffer_params.allocation.size = vertex_size;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      vertex_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }

    if (!index_buffer || bfBuffer_size(index_buffer) < indices_size)
    {
      bfGfxDevice_release(device, index_buffer);

      buffer_params.allocation.size = indices_size;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

      index_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }
  }

  void Gfx2DPerFrameRenderData::reserveShadow(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
  {
    bfBufferCreateParams buffer_params;
    buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;

    if (!vertex_shadow_buffer || bfBuffer_size(vertex_shadow_buffer) < vertex_size)
    {
      bfGfxDevice_release(device, vertex_shadow_buffer);

      buffer_params.allocation.size = vertex_size;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      vertex_shadow_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }

    if (!index_shadow_buffer || bfBuffer_size(index_shadow_buffer) < indices_size)
    {
      bfGfxDevice_release(device, index_shadow_buffer);

      buffer_params.allocation.size = indices_size;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

      index_shadow_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }
  }

  Gfx2DRenderData::Gfx2DRenderData(GLSLCompiler& glsl_compiler) :
    device{bfGfxGetDevice()},
    vertex_layouts{nullptr, nullptr},
    vertex_shader{nullptr},
    fragment_shader{nullptr},
    shader_program{nullptr},
    shadow_modules{nullptr, nullptr, nullptr},
    rect_shadow_program{nullptr},
    rounded_rect_shadow_program{nullptr},
    white_texture{nullptr},
    num_frame_datas{0},
    frame_datas{}
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
    const auto& frame_info = bfGfxGetFrameInfo();

    num_frame_datas = int(frame_info.num_frame_indices);

    for (int i = 0; i < num_frame_datas; ++i)
    {
      frame_datas[i] = Gfx2DPerFrameRenderData();
    }
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

        if (it == stops_bgn)  // Clamp To Start of gradient.
        {
          result.color = stops_bgn->value;
        }
        else if (it == stops_end)  // Clamp To End of gradient.
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

          result.color = bfMathLerpColor4f(color_a, color_b, local_lerp_factor);
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

  CommandBuffer2D::CommandBuffer2D(GLSLCompiler& glsl_compiler) :
    render_data{glsl_compiler},
    aux_memory{},
    command_stream{},
    vertex_stream{},
    index_stream{},
    num_commands{0u},
    current_clip_rect{nullptr}
  {
  }

  Brush* CommandBuffer2D::makeBrush(bfColor32u color)
  {
    return makeBrush(
     bfColor4f_fromColor4u(
      bfColor4u_fromUint32(color)));
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
    Render2DFillRect* result = allocCommand<Render2DFillRect>(brush);

    result->rect = rect;

    ++num_commands;
    return result;
  }

  Render2DFillRoundedRect* CommandBuffer2D::fillRoundedRect(const Brush* brush, AxisQuad rect, float border_radius)
  {
    border_radius = std::min({border_radius, rect.width() * 0.5f, rect.height() * 0.5f});

    assert(border_radius >= 0.0f && "A border radius of less than 0.0f does not make sense.");

    Render2DFillRoundedRect* result = allocCommand<Render2DFillRoundedRect>(brush);

    result->rect          = rect;
    result->border_radius = border_radius;

    ++num_commands;
    return result;
  }

  Render2DBlurredRect* CommandBuffer2D::blurredRect(const Brush* brush, Rect2f rect, float shadow_sigma, float border_radius)
  {
    Render2DBlurredRect* result = allocCommand<Render2DBlurredRect>(brush);

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

    Render2DFillArc* result = allocCommand<Render2DFillArc>(brush);

    result->position    = position;
    result->radius      = radius;
    result->start_angle = start_angle;
    result->arc_angle   = arc_angle > k_TwoPI ? k_TwoPI : arc_angle;

    ++num_commands;
    return result;
  }

  Render2DPolyline* CommandBuffer2D::polyline(const Brush* brush, const Vector2f* points, UIIndexType num_points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bool is_overlap_allowed)
  {
    Render2DPolyline* result = allocCommand<Render2DPolyline>(brush);

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

  Render2DText* CommandBuffer2D::text(const Brush* brush, Vector2f position, StringRange utf8_text, float scale)
  {
    assert(brush->type == Brush::Font && "Text must be drawn with a Font brush.");

    Render2DText* result = allocCommand<Render2DText>(brush);

    const BufferRange cloned_string = string_utils::clone(aux_memory, utf8_text);
    UIIndexType       num_codepoints;

    result->utf8_text      = cloned_string.toStringRange();
    result->bounds_size    = calculateTextSize(utf8_text, brush->font_data.font, &num_codepoints) * scale;
    result->num_codepoints = num_codepoints;
    result->position       = position;
    result->scale          = scale;

    ++num_commands;
    return result;
  }

  ClipRect* CommandBuffer2D::pushClipRect(Rect2i rect)
  {
    ClipRect* const clip_rect = aux_memory.allocateT<ClipRect>();

    rect.setLeft(std::max(0, rect.left()));
    rect.setTop(std::max(0, rect.top()));
    rect.setRight(std::max(0, rect.right()));
    rect.setBottom(std::max(0, rect.bottom()));

    clip_rect->rect   = rect;
    clip_rect->prev   = current_clip_rect;
    current_clip_rect = clip_rect;

    return clip_rect;
  }

  void CommandBuffer2D::popClipRect()
  {
    current_clip_rect = current_clip_rect->prev;

    assert(current_clip_rect != nullptr && "Mismatched push(es) and pop(s) for cliup rect.");
  }

  void CommandBuffer2D::clear(Rect2i default_clip_rect)
  {
    aux_memory.clear();
    command_stream.clear();
    vertex_stream.clear();
    index_stream.clear();
    num_commands = 0u;

    current_clip_rect = nullptr;
    pushClipRect(default_clip_rect);
  }

  //
  // Assumes `T` has a member named 'next' of type `T*`.
  //
  template<typename T>
  struct TempFwdList
  {
    T* first = nullptr;
    T* last  = nullptr;

    template<typename F>
    void forEach(F&& callback) const
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
      if (!first) { first = item; }
      if (last) { last->next = item; }

      last = item;
    }
  };

  void CommandBuffer2D::renderToQueue(RenderQueue& render_queue, const DescSetBind& object_binding)
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

      Rect2f                     bounds;            //!< Cached screen bounds of element.
      std::uint8_t               flags;             //!< Some flags for batc/sort-ing algorithm.
      const BaseRender2DCommand* command;           //!< The command that corresponds to this element.
      Gfx2DElement*              next;              //!< Intrusive linked list used by `Batch2D`.
      VertIdxCountResult         vertex_idx_count;  //!< Cached vertex count results.

      bool hasBeenDrawn() const { return flags & HAS_BEEN_DRAWN; }
      bool isInCurrentBatch() const { return flags & IS_IN_CURRENT_BATCH; }
    };

    // All batches have at least one command.
    struct Batch2D
    {
      TempFwdList<Gfx2DElement> commands    = {};
      Batch2D*                  next        = nullptr;
      UIIndexType               first_index = 0u;
      UIIndexType               num_indices = 0u;
    };

    struct BatchList : public TempFwdList<Batch2D>
    {
      void findOrAdd(LinearAllocator& alloc, Gfx2DElement* item)
      {
        Batch2D* compatible_batch = first;

        while (compatible_batch)
        {
          Batch2D* const                   it_next = compatible_batch->next;
          const BaseRender2DCommand* const command = compatible_batch->commands.first->command;

          if (command->canBeBatchedWith(*item->command))
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

    const char* byte_stream = command_stream.begin();

    for (std::size_t i = 0u; i < num_elements; ++i)
    {
      const BaseRender2DCommand* command = reinterpret_cast<const BaseRender2DCommand*>(byte_stream);
      byte_stream += command->size;

      elements[i].bounds  = calcCommandBounds(command);
      elements[i].flags   = 0x0;
      elements[i].command = command;
      elements[i].next    = nullptr;
      // elements[i].vertex_idx_count = /* ---, this will be written to later */

      // We do not want to actually draw zero size objects.
      // So we mark it as drawn so it never gets added to a batch.
      if (elements[i].bounds.area() == 0.0f)
      {
        elements[i].flags |= HAS_BEEN_DRAWN;
        --num_elements_left_to_sort;
      }
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

          if (!element.hasBeenDrawn())
          {
            bool can_add_to_batch = true;

            for (std::size_t j = 0; j < i; ++j)
            {
              const Gfx2DElement& behind_element = elements[j];

              // The first two checks are cheaper than the intersection test.
              if ((behind_element.isInCurrentBatch() || !behind_element.hasBeenDrawn()) &&
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

        // Merging with the last active batch can happen in the next iteration of the loop.
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

    final_batches.forEach([this](Batch2D* batch) {
      batch->commands.forEach([batch](Gfx2DElement* a) {
        batch->commands.forEach([batch, a](Gfx2DElement* b) {
          if (a->command->clip_rect != b->command->clip_rect)
          {
            __debugbreak();
          }
        });
      });
    });

    //
    // Upload the Vertex / Index Data To The GPU
    //

    const auto&              frame_info = bfGfxGetFrameInfo();
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
        writeVertices(dest, element->command, element->vertex_idx_count, element->bounds);

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

    const ClipRect* last_clip_rect = nullptr;

    final_batches.forEach([this, &pipeline, &render_queue, &frame_data, &object_binding, &frame_info, &last_clip_rect](Batch2D* batch) mutable -> void {
      const BaseRender2DCommand* command   = batch->commands.first->command;
      const ClipRect* const      clip_rect = command->clip_rect;
      bfBufferHandle             index_buffer;
      bfBufferHandle             vertex_buffer;

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

      RC_Group* const       group_cmd      = render_queue.group();
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

      constexpr float k_DistanceToCamera = 0.0f;

      if (last_clip_rect != clip_rect)
      {
        const Rect2i&            clip     = clip_rect->rect;
        RC_SetScissorRect* const set_clip = render_queue.setScissorRect({clip.left(), clip.top(), std::uint32_t(clip.width()), std::uint32_t(clip.height())});

        group_cmd->push(set_clip);

        last_clip_rect = clip_rect;
      }

      group_cmd->push(render_command);
      render_queue.submit(render_queue.makeKeyFor(render_command, k_DistanceToCamera), group_cmd);
    });
  }

  void CommandBuffer2D::renderToQueue(RenderQueue& render_queue)
  {
    renderToQueue(render_queue, {});
  }

  Rect2f CommandBuffer2D::calcCommandBounds(const BaseRender2DCommand* command)
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

        return boundsFromPoints(typed_command->points, typed_command->num_points).expandedFromCenter(typed_command->thickness * 0.5f);
      }
      case Render2DCommandType::FillTriangles:
      {
        const Render2DFillTriangles* typed_command = static_cast<const Render2DFillTriangles*>(command);

        return boundsFromPoints(typed_command->points, typed_command->num_points);
      }
      case Render2DCommandType::Text:
      {
        const Render2DText* typed_command = static_cast<const Render2DText*>(command);

        const auto     baseline_info = fontBaselineInfo(typed_command->brush->font_data.font->font);
        const Vector2f min_bounds    = {typed_command->position.x, typed_command->position.y - typed_command->bounds_size.y};
        const Vector2f max_bounds    = {
         min_bounds.x + typed_command->bounds_size.x * typed_command->scale,
         typed_command->position.y - baseline_info.descent_px * typed_command->scale,
        };

        return {min_bounds, max_bounds};
      }
        bfInvalidDefaultCase();
    }
  }

  std::pair<UIIndexType, UIVertex2D*> CommandBuffer2D::VertIdxCountResult::requestVertices(VertexStreamMem& vertex_memory, UIIndexType count)
  {
    const UIIndexType result_offset   = num_vertices;
    UIVertex2D* const result_vertices = static_cast<UIVertex2D*>(vertex_memory.allocate(sizeof(UIVertex2D) * count));

    if (!precalculated_vertices)
    {
      precalculated_vertices = result_vertices;
    }

    num_vertices += count;

    return {result_offset, result_vertices};
  }

  void CommandBuffer2D::VertIdxCountResult::pushTriIndex(UIIndexType global_index_offset, IndexStreamMem& index_memory, UIIndexType index0, UIIndexType index1, UIIndexType index2)
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

  CommandBuffer2D::VertIdxCountResult CommandBuffer2D::calcVertexCount(UIIndexType global_index_offset, const BaseRender2DCommand* command)
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
        auto* const points             = typed_command->points;
        const auto  num_points         = typed_command->num_points;
        const auto  thickness          = typed_command->thickness;
        const auto  join_style         = typed_command->join_style;
        const auto  end_style          = typed_command->end_style;
        const auto  is_overlap_allowed = typed_command->is_overlap_allowed;

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

  void CommandBuffer2D::writeVertices(const DestVerts& dest, const BaseRender2DCommand* command, VertIdxCountResult& counts, const Rect2f& bounds)
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
      float        pos_to_uv[4]; /* x, y, inv_width, inv_height */

      Vector2f mapPosUV(Vector2f pos) const
      {
        return {
         (pos.x - pos_to_uv[0]) * pos_to_uv[2],
         (pos.y - pos_to_uv[1]) * pos_to_uv[3],
        };
      }

      void addRect(AxisQuad rect)
      {
        const VertexWrite v = getVerts(k_NumVertRect);

        const Vector2f positions[] =
         {
          rect.v0(),
          rect.v1(),
          rect.v2(),
          rect.v3(),
         };

        Vector2f  uvs[4];
        bfColor4u colors[4];

        for (int i = 0; i < int(k_NumVertRect); ++i)
        {
          const BrushSampleResult brush_sample = brush->sample(mapPosUV(positions[i]), i);

          uvs[i]    = brush_sample.remapped_uv;
          colors[i] = bfColor4u_fromColor4f(brush_sample.color);
        }

        v.v[0] = {positions[0], uvs[0], colors[0]};
        v.v[1] = {positions[1], uvs[1], colors[1]};
        v.v[2] = {positions[2], uvs[2], colors[2]};
        v.v[3] = {positions[3], uvs[3], colors[3]};

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
        float                   x                 = std::cos(start_angle) * radius;
        float                   y                 = std::sin(start_angle) * radius;
        UIIndexType             current_vertex    = 0;
        const BrushSampleResult middle_sample     = brush->sample(mapPosUV(pos), current_vertex);

        v.v[current_vertex++] = {pos, middle_sample.remapped_uv, bfColor4u_fromColor4f(middle_sample.color)};

        for (UIIndexType i = 0; i < num_segments; ++i)
        {
          const UIIndexType p0_index = current_vertex;
          {
            const Vector2f          p0        = Vector2f{x + pos.x, y + pos.y};
            const Vector2f          p0_uv     = mapPosUV(p0);
            const BrushSampleResult p0_sample = brush->sample(p0_uv, current_vertex);
            v.v[current_vertex++]             = {p0, p0_sample.remapped_uv, bfColor4u_fromColor4f(p0_sample.color)};
          }

          const float tx = -y;
          const float ty = x;

          x += tx * tangential_factor;
          y += ty * tangential_factor;
          x *= radial_factor;
          y *= radial_factor;

          const UIIndexType p1_index = current_vertex;
          {
            const Vector2f          p1        = Vector2f{x + pos.x, y + pos.y};
            const Vector2f          p1_uv     = mapPosUV(p1);
            const BrushSampleResult p1_sample = brush->sample(p1_uv, current_vertex);

            v.v[current_vertex++] = {p1, p1_sample.remapped_uv, bfColor4u_fromColor4f(p1_sample.color)};
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

    // Bounds are guaranteed to be non zero size at this point in the code.
    const float bounds_width  = bounds.width();
    const float bounds_height = bounds.height();

    VertexWriter writer = {
     dest.vertex_offset,
     dest.vertex_buffer_ptr,
     dest.index_buffer_ptr,
     command->brush,
     {
      bounds.left(),
      bounds.top(),
      1.0f / bounds_width,
      1.0f / bounds_height,
     },
    };

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
        const auto* const typed_command = static_cast<const Render2DPolyline*>(command);

        for (UIIndexType i = 0; i < counts.num_vertices; ++i)
        {
          UIVertex2D& vertex       = counts.precalculated_vertices[i];
          const auto  brush_sample = typed_command->brush->sample(writer.mapPosUV(vertex.pos), i);

          vertex.color = bfColor4u_fromColor4f(brush_sample.color);
          vertex.uv    = brush_sample.remapped_uv;
        }

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

        const Vector2f& pos            = typed_command->position;
        const char*     utf8_text      = typed_command->utf8_text.begin();
        const char*     utf8_text_end  = typed_command->utf8_text.end();
        float           x              = pos.x;
        float           y              = pos.y;
        PainterFont*    font           = typed_command->brush->font_data.font;
        const bfColor4u color          = bfColor4u_fromColor4f(typed_command->brush->font_data.tint);
        const float     newline_height = fontNewlineHeight(font->font);

        if (utf8_text != utf8_text_end)
        {
          TextEncodingResult<TextEncoding::UTF8> res   = utf8Codepoint(utf8_text);
          const float                            scale = typed_command->scale;

          while (utf8_text < utf8_text_end)
          {
            const bool is_backslash_r = *utf8_text == '\r';

            if (is_backslash_r || *utf8_text == '\n')
            {
              x = pos.x;
              y += newline_height;

              ++utf8_text;

              if (is_backslash_r && *utf8_text == '\n')  // Handle Window's '\r\n'
              {
                ++utf8_text;
              }

              continue;
            }

            // NOTE(SR):
            //  `x` and `y` are rounded because to have good text
            //  rendering we must be aligned to a pixel boundary.

            const CodePoint   codepoint = res.codepoint;
            const GlyphInfo&  glyph     = fontGetGlyphInfo(font->font, codepoint);
            const VertexWrite v         = writer.getVerts(4);
            const Vector2f    size_x    = {float(glyph.bmp_box[1].x) * scale, 0.0f};
            const Vector2f    size_y    = {0.0f, float(glyph.bmp_box[1].y) * scale};
            const Vector2f    size_xy   = {size_x.x, size_y.y};
            Vector2f          p0        = Vector2f{x, y} + Vector2f{glyph.offset[0], glyph.offset[1]} * scale;
            p0.x                        = std::round(p0.x);
            p0.y                        = std::round(p0.y);
            const Vector2f p1           = p0 + size_x;
            const Vector2f p2           = p0 + size_xy;
            const Vector2f p3           = p0 + size_y;

            v.v[0] = {p0, {glyph.uvs[0], glyph.uvs[1]}, color};
            v.v[1] = {p1, {glyph.uvs[2], glyph.uvs[1]}, color};
            v.v[2] = {p2, {glyph.uvs[2], glyph.uvs[3]}, color};
            v.v[3] = {p3, {glyph.uvs[0], glyph.uvs[3]}, color};

            writer.pushTriIndex(v.id + 0, v.id + 3, v.id + 2);
            writer.pushTriIndex(v.id + 0, v.id + 2, v.id + 1);

            utf8_text = res.endpos;
            x += glyph.advance_x * scale;

            if (utf8_text < utf8_text_end)  // Not at the end
            {
              res = utf8Codepoint(utf8_text);
              x += fontAdditionalAdvance(font->font, codepoint, res.codepoint) * scale;
            }
          }
        }

        font->device = render_data.device;

        const auto&   frame_info    = bfGfxGetFrameInfo();
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

  Vector2f calculateTextSize(StringRange utf8_string, PainterFont* font, std::uint32_t* num_codepoints)
  {
    float         max_width            = 0.0f;
    float         current_width        = 0.0f;
    float         current_height       = 0.0f;
    const char*   utf8_text            = utf8_string.begin();
    const char*   utf8_text_end        = utf8_string.end();
    std::uint32_t num_codepoints_local = 0;

    if (utf8_text != utf8_text_end)
    {
      const float newline_height = fontNewlineHeight(font->font);
      auto        res            = utf8Codepoint(utf8_text);

      current_height += newline_height;

      while (utf8_text < utf8_text_end)
      {
        const bool is_backslash_r = *utf8_text == '\r';

        if (is_backslash_r || *utf8_text == '\n')
        {
          max_width     = std::max(current_width, max_width);
          current_width = 0.0f;
          current_height += newline_height;

          ++utf8_text;

          // Handle Window's '\r\n'
          if (is_backslash_r && *utf8_text == '\n')
          {
            ++utf8_text;
          }

          continue;
        }

        const CodePoint  codepoint = res.codepoint;
        const GlyphInfo& glyph     = fontGetGlyphInfo(font->font, codepoint);

        utf8_text = res.endpos;
        current_width += glyph.advance_x;

        if (utf8_text < utf8_text_end)  // Not at the end
        {
          res = utf8Codepoint(utf8_text);
          current_width += fontAdditionalAdvance(font->font, codepoint, res.codepoint);
        }

        ++num_codepoints_local;
      }
    }

    if (num_codepoints)
    {
      *num_codepoints = num_codepoints_local;
    }

    return {std::max(current_width, max_width), current_height};
  }
}  // namespace bf

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
