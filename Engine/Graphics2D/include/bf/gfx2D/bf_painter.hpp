#ifndef BF_PAINTER_HPP
#define BF_PAINTER_HPP

//
// Adds a check to see if all vertices requested in Gfx2DPainter are written to.
// *warning: There is a good amount of overhead both CPU and Memory when this is checked on*
//
#define bfGfx2DSafeVertexIndexing 0

#include "bf/LinearAllocator.hpp"  // LinearAllocator
#include "bf/MemoryUtils.h"        // bfMegabytes

#include "bf/graphics/bifrost_standard_renderer.hpp"  // Math and Graphics

namespace bf
{
  //
  // Forward Declarations
  //

  struct Font;

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
  // Unform Data
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
    IMemoryManager&               memory;
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
    Gfx2DPerFrameRenderData*      frame_datas;
    MultiBuffer<Gfx2DUniformData> uniform;

    explicit Gfx2DRenderData(IMemoryManager& memory, GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics);

    // Size measured in number of items.
    void reserve(int index, size_t vertex_size, size_t indices_size) const;
    void reserveShadow(int index, size_t vertex_size, size_t indices_size) const;

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
    Font*        font;
    DynamicAtlas gpu_atlas[k_bfGfxMaxFramesDelay];

    PainterFont(IMemoryManager& memory, const char* filename, float pixel_height);
    ~PainterFont();
  };

  struct Gfx2DPainter : private NonCopyMoveable<Gfx2DPainter>
  {
    static const UIIndexType k_TempMemorySize = bfMegabytes(2);

   private:
    Gfx2DRenderData         render_data;                           //!< GPU Data
    Array<UIVertex2D>       vertices;                              //!< CPU Vertices
    Array<UIIndexType>      indices;                               //!< CPU Index Buffer
    Array<DropShadowVertex> shadow_vertices;                       //!< CPU Vertices
    Array<UIIndexType>      shadow_indices;                        //!< CPU Index Buffer
    char                    tmp_memory_backing[k_TempMemorySize];  //!< Memory pool for `Gfx2DPainter::tmp_memory`
    LinearAllocator         tmp_memory;                            //!< The allocator interface
    Array<Gfx2DDrawCommand> draw_commands;                         //!< TODO(SR): A Pool Allocator would be better but whatevs

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
    // Small helper type to make sure no mistakes happen when writing vertices.
    template<typename T>
    struct SafeVertexIndexer final
    {
#if bfGfx2DSafeVertexIndexing
      static inline char            s_TmpMemoryBacking[bfKilobytes(50)];
      static inline LinearAllocator s_TmpMemory{s_TmpMemoryBacking, sizeof(s_TmpMemoryBacking)};
#endif

     public:
      UIIndexType num_verts;
      T*          verts;

#if bfGfx2DSafeVertexIndexing
      LinearAllocatorScope mem_scope;
      mutable bool*        used_vertices;

      SafeVertexIndexer(UIIndexType size, T* verts) :
        num_verts{size},
        verts{verts},
        mem_scope{s_TmpMemory},
        used_vertices(static_cast<bool*>(s_TmpMemory.allocate(sizeof(bool) * size)))
      {
      }

      SafeVertexIndexer(SafeVertexIndexer&& rhs) noexcept :
        num_verts{std::exchange(rhs.num_verts, 0)},
        verts{std::exchange(rhs.verts, nullptr)},
        mem_scope{std::move(rhs.mem_scope)},
        used_vertices(std::exchange(rhs.used_vertices, nullptr))
      {
      }
#endif

      T& operator[](UIIndexType index) const noexcept
      {
        assert(index < num_verts);

#if bfGfx2DSafeVertexIndexing
        used_vertices[index] = true;
#endif

        return verts[index];
      }

#if bfGfx2DSafeVertexIndexing
      ~SafeVertexIndexer()
      {
        for (UIIndexType i = 0; i < num_verts; ++i)
        {
          assert(used_vertices[i] && "There was an unused vertex at slot i");
        }
      }
#else
      ~SafeVertexIndexer() noexcept = default;
#endif
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
