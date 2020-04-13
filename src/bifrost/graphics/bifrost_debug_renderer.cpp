#include "bifrost/graphics/bifrost_debug_renderer.hpp"
#include <cassert>

namespace bifrost
{
  DebugRenderer::DebugRenderer(IMemoryManager& memory) :
    m_Gfx{nullptr},
    m_LineBufferPool{nullptr},
    m_DepthDrawCommands{memory},
    m_OverlayDrawCommands{memory},
    m_LineBuffers{Array<BufferLink*>{memory}, Array<BufferLink*>{memory}},
    m_DbgOverlay{nullptr, nullptr},
    m_DbgWorld{nullptr, nullptr},
    m_Shaders{nullptr, nullptr},
    m_DbgVertexLayout{nullptr}
  {
  }

  void DebugRenderer::init(StandardRenderer& renderer)
  {
    const auto device = renderer.device();

    m_Gfx             = &renderer;
    m_DbgOverlay[0]   = renderer.glslCompiler().createModule(device, "assets/shaders/debug/dbg_overlay.vert.glsl");
    m_DbgOverlay[1]   = renderer.glslCompiler().createModule(device, "assets/shaders/debug/dbg_overlay.frag.glsl");
    m_DbgWorld[0]     = renderer.glslCompiler().createModule(device, "assets/shaders/debug/dbg_world.vert.glsl");
    m_DbgWorld[1]     = renderer.glslCompiler().createModule(device, "assets/shaders/debug/dbg_world.frag.glsl");
    m_Shaders[0]      = gfx::createShaderProgram(device, 1, m_DbgWorld[0], m_DbgWorld[1], "Debug.World");
    m_Shaders[1]      = gfx::createShaderProgram(device, 1, m_DbgOverlay[0], m_DbgOverlay[1], "Debug.Overlay");
    m_DbgVertexLayout = bfVertexLayout_new();

    bfVertexLayout_addVertexBinding(m_DbgVertexLayout, 0, sizeof(VertexDebugLine));
    bfVertexLayout_addVertexLayout(m_DbgVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(VertexDebugLine, curr_pos));
    bfVertexLayout_addVertexLayout(m_DbgVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(VertexDebugLine, next_pos));
    bfVertexLayout_addVertexLayout(m_DbgVertexLayout, 0, BIFROST_VFA_FLOAT32_4, offsetof(VertexDebugLine, prev_pos));
    bfVertexLayout_addVertexLayout(m_DbgVertexLayout, 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(VertexDebugLine, color));
    bfVertexLayout_addVertexLayout(m_DbgVertexLayout, 0, BIFROST_VFA_FLOAT32_1, offsetof(VertexDebugLine, direction));
    bfVertexLayout_addVertexLayout(m_DbgVertexLayout, 0, BIFROST_VFA_FLOAT32_1, offsetof(VertexDebugLine, thickness));

    bfShaderProgram_addUniformBuffer(m_Shaders[0], "u_Set0", k_GfxCameraSetIndex, 0, 1, BIFROST_SHADER_STAGE_VERTEX);
    bfShaderProgram_addUniformBuffer(m_Shaders[1], "u_Set0", k_GfxCameraSetIndex, 0, 1, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(m_Shaders[0]);
    bfShaderProgram_compile(m_Shaders[1]);
  }

  void DebugRenderer::addLine(const Vector3f& a, const Vector3f& b, const bfColor4u& color, float duration, bool is_overlay)
  {
    grabCommandList(is_overlay).emplaceBack().init(duration, DrawLine{a, b, color});
  }

  void DebugRenderer::addAABB(const Vector3f& center, const Vector3f& extents, const bfColor4u& color, float duration, bool is_overlay)
  {
    grabCommandList(is_overlay).emplaceBack().init(duration, DrawAABB{center, extents, color});
  }

  void DebugRenderer::draw(bfGfxCommandListHandle command_list, Camera& camera, const bfGfxFrameInfo& frame_info, bool overlay)
  {
    auto&                          line_buffer = m_LineBuffers[overlay ? 1 : 0];
    /* const */ List<DrawCommand>& list        = grabCommandList(overlay);  // TODO: bifrost::List / ListView needs some const iterators.

    if (list.isEmpty())
    {
      return;
    }

    clearLineBuffer(line_buffer);

    for (const DrawCommand& command : list)
    {
      visit_all(
       meta::overloaded{
        [this, &frame_info, &line_buffer](const DrawLine& data) {
          addVertices(line_buffer, data.a, data.b, data.color, frame_info);
        },
        [this, &frame_info, &line_buffer](const DrawAABB& data) {
          const Vector3f half_extent = data.extents * 0.5f;
          const Vector3f min_point   = data.center - half_extent;
          const Vector3f max_point   = data.center + half_extent;

          const Vector3f points[8] =
           {
            min_point,                                // 0
            {max_point.x, min_point.y, min_point.z},  // 1
            {min_point.x, max_point.y, min_point.z},  // 2
            {min_point.x, min_point.y, max_point.z},  // 3
            max_point,                                // 4
            {min_point.x, max_point.y, max_point.z},  // 5
            {max_point.x, min_point.y, max_point.z},  // 6
            {max_point.x, max_point.y, min_point.z},  // 7
           };

          // Top 'Face'
          addVertices(line_buffer, points[1], points[0], data.color, frame_info);
          addVertices(line_buffer, points[1], points[6], data.color, frame_info);
          addVertices(line_buffer, points[3], points[6], data.color, frame_info);
          addVertices(line_buffer, points[3], points[0], data.color, frame_info);

          // Bottom 'Face'
          addVertices(line_buffer, points[4], points[7], data.color, frame_info);
          addVertices(line_buffer, points[4], points[5], data.color, frame_info);
          addVertices(line_buffer, points[2], points[5], data.color, frame_info);
          addVertices(line_buffer, points[2], points[7], data.color, frame_info);

          // Sides
          addVertices(line_buffer, points[0], points[2], data.color, frame_info);
          addVertices(line_buffer, points[1], points[7], data.color, frame_info);
          addVertices(line_buffer, points[3], points[5], data.color, frame_info);
          addVertices(line_buffer, points[6], points[4], data.color, frame_info);
        },
       },
       command.data);
    }

    bfGfxCmdList_setCullFace(command_list, BIFROST_CULL_FACE_NONE);
    bfGfxCmdList_bindProgram(command_list, m_Shaders[overlay ? 1 : 0]);
    bfGfxCmdList_bindVertexDesc(command_list, m_DbgVertexLayout);

    m_Gfx->bindCamera(command_list, camera);

    for (BufferLink* const link : line_buffer)
    {
      const int num_vertices = link->numVertices();

      link->gpu_buffer.flushCurrent(frame_info);
      bfBuffer_unMap(link->gpu_buffer.handle());

      if (num_vertices)
      {
        const bfBufferSize offset = link->gpu_buffer.offset(frame_info);

        bfGfxCmdList_bindVertexBuffers(command_list, 0, &link->gpu_buffer.handle(), 1, &offset);
        bfGfxCmdList_draw(command_list, 0, num_vertices);
      }
    }
    bfGfxCmdList_setCullFace(command_list, BIFROST_CULL_FACE_BACK);
  }

  void DebugRenderer::deinit()
  {
    bfVertexLayout_delete(m_DbgVertexLayout);

    for (int i = 0; i < 2; ++i)
    {
      bfGfxDevice_release(m_Gfx->device(), m_DbgOverlay[i]);
      bfGfxDevice_release(m_Gfx->device(), m_DbgWorld[i]);
      bfGfxDevice_release(m_Gfx->device(), m_Shaders[i]);
    }

    for (auto& m_LineBuffer : m_LineBuffers)
    {
      clearLineBuffer(m_LineBuffer);
    }

    while (m_LineBufferPool)
    {
      BufferLink* const next = m_LineBufferPool->next;
      m_LineBufferPool->gpu_buffer.destroy(m_Gfx->device());
      memory().deallocateT(m_LineBufferPool);
      m_LineBufferPool = next;
    }
  }

  List<DebugRenderer::DrawCommand>& DebugRenderer::grabCommandList(bool is_overlay)
  {
    return is_overlay ? m_OverlayDrawCommands : m_DepthDrawCommands;
  }

  DebugRenderer::BufferLink* DebugRenderer::grabFreeLink(const bfGfxFrameInfo& frame_info)
  {
    BufferLink* result = m_LineBufferPool;

    if (!result)
    {
      result = memory().allocateT<BufferLink>();
      result->gpu_buffer.create(
       m_Gfx->device(),
       BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER,
       frame_info,
       sizeof(Vector3f));
    }
    else
    {
      m_LineBufferPool = m_LineBufferPool->next;
    }

    result->vertices_left = k_DebugRendererLineBatchSize;
    result->next          = nullptr;

    return result;
  }

  void DebugRenderer::clearLineBuffer(Array<BufferLink*>& buffer_link_list)
  {
    for (BufferLink* const link : buffer_link_list)
    {
      link->next       = m_LineBufferPool;
      m_LineBufferPool = link;
    }

    buffer_link_list.clear();
  }

  void DebugRenderer::updateDrawCommands(List<DrawCommand>& list, float delta_time)
  {
    for (auto it = list.begin(); it != list.end();)
    {
      it->duration -= delta_time;

      if (it->duration <= 0.0f)
      {
        it = list.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  void DebugRenderer::addVertices(Array<BufferLink*>& buffer, const Vector3f& a, const Vector3f& b, const bfColor4u& color, const bfGfxFrameInfo& frame_info)
  {
    static constexpr float k_Thickness = 0.1f;

    const VertexDebugLine vertices[] =
     {
      {a, b, a, color, 1.0f, k_Thickness},
      {a, b, a, color, -1.0f, k_Thickness},
      {b, b, a, color, 1.0f, k_Thickness},
      {b, b, a, color, -1.0f, k_Thickness},
     };

    addTriangle(buffer, vertices[0], vertices[2], vertices[1], frame_info);
    addTriangle(buffer, vertices[1], vertices[2], vertices[3], frame_info);
  }

  void DebugRenderer::addTriangle(Array<BufferLink*>& buffer, const VertexDebugLine& a, const VertexDebugLine& b, const VertexDebugLine& c, const bfGfxFrameInfo& frame_info)
  {
    static constexpr int k_NumVerticesInTriangle = 3;

    if (buffer.isEmpty() || buffer.back()->vertices_left < k_NumVerticesInTriangle)
    {
      BufferLink* new_link = grabFreeLink(frame_info);

      buffer.push(new_link);

      bfBuffer_map(new_link->gpu_buffer.handle(), new_link->gpu_buffer.offset(frame_info), new_link->gpu_buffer.elementAlignedSize());
    }

    BufferLink* buffer_link = buffer.back();

    VertexDebugLine* data = buffer_link->currentVertex();

    data[0] = a;
    data[1] = b;
    data[2] = c;

    buffer_link->vertices_left -= k_NumVerticesInTriangle;
  }
}  // namespace bifrost
