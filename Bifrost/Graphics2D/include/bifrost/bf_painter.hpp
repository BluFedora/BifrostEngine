#ifndef BF_PAINTER_HPP
#define BF_PAINTER_HPP

#include "bifrost/graphics/bifrost_standard_renderer.hpp"  // Math and Graphics
#include "bifrost/memory/bifrost_linear_allocator.hpp"     // LinearAllocator
#include "bifrost/memory/bifrost_memory_utils.h"           // bfMegabytes

namespace bifrost
{
  struct UIVertex2D final
  {
    Vector2f  pos;
    Vector2f  uv;
    bfColor4u color;
  };

  using UIIndexType = std::uint32_t;

  struct Gfx2DPerFrameRenderData final
  {
    bfBufferHandle vertex_buffer = nullptr;
    bfBufferHandle index_buffer  = nullptr;

    // Size measured in number of bytes
    void reserve(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size);
  };

  struct Gfx2DUnifromData final
  {
    Mat4x4 ortho_matrix;
  };

  struct Gfx2DRenderData final : private bfNonCopyMoveable<Gfx2DRenderData>
  {
    IMemoryManager&               memory;
    bfGfxContextHandle            ctx;
    bfGfxDeviceHandle             device;
    bfVertexLayoutSetHandle       vertex_layout;
    bfShaderModuleHandle          vertex_shader;
    bfShaderModuleHandle          fragment_shader;
    bfShaderProgramHandle         shader_program;
    bfTextureHandle               white_texture;
    int                           num_frame_datas;
    Gfx2DPerFrameRenderData*      frame_datas;
    MultiBuffer<Gfx2DUnifromData> uniform;

    explicit Gfx2DRenderData(IMemoryManager& memory, GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics);

    // Size measured in number of items.
    void reserve(int index, size_t vertex_size, size_t indices_size) const;
    ~Gfx2DRenderData();

   private:
    template<typename F>
    void forEachBuffer(F&& fn) const
    {
      for (std::size_t i = 0; i < num_frame_datas; ++i)
      {
        fn(frame_datas[i]);
      }
    }
  };

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

  struct Gfx2DPainter final : private bfNonCopyMoveable<Gfx2DPainter>
  {
    static const UIIndexType k_TempMemorySize = bfMegabytes(2);

    // private:
    Gfx2DRenderData    render_data;                           //!< GPU Data
    Array<UIVertex2D>  vertices;                              //!< CPU Vertices
    Array<UIIndexType> indices;                               //!< CPU Index Buffer
    char               tmp_memory_backing[k_TempMemorySize];  //!< Memory pool for Gfx2DPainter::tmp_memory_backing
    LinearAllocator    tmp_memory;                            //!< The allocator interface

    // public:
    explicit Gfx2DPainter(IMemoryManager& memory, GLSLCompiler& glsl_compiler, bfGfxContextHandle graphics);

    void reset();

    void pushRect(const Vector2f& pos, float width, float height, bfColor4u color);
    void pushRect(const Vector2f& pos, float width, float height, bfColor32u color = BIFROST_COLOR_PINK);
    void pushFillRoundedRect(const Vector2f& pos, float width, float height, float border_radius, bfColor32u color);
    void pushFilledArc(const Vector2f& pos, float radius, float start_angle, float arc_angle, bfColor32u color);
    void pushFilledCircle(const Vector2f& pos, float radius, bfColor32u color);
    void pushLinedArc(const Vector2f& pos, float radius, float start_angle, float arc_angle, bfColor32u color);
    void pushPolyline(const Vector2f* points, UIIndexType num_points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bfColor32u color, bool is_overlap_allowed = false);
    void pushPolyline(ArrayView<const Vector2f> points, float thickness, PolylineJoinStyle join_style, PolylineEndStyle end_style, bfColor32u color);

    void render(bfGfxCommandListHandle command_list, int fb_width, int fb_height);

   private:
    // Small helper type to make sure no mistakes happen when writing vertices.
    struct SafeVertexIndexer final
    {
      UIIndexType num_verts;
      UIVertex2D* verts;

      UIVertex2D& operator[](UIIndexType index) const
      {
        assert(index < num_verts);
        return verts[index];
      }
    };

    std::pair<UIIndexType, SafeVertexIndexer> requestVertices(UIIndexType num_verts);
    void                                      pushTriIndex(UIIndexType index0, UIIndexType index1, UIIndexType index2);
  };
}  // namespace bifrost

#endif  // BF_PAINTER_HPP
