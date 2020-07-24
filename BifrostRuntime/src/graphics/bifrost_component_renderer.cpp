/******************************************************************************/
/*!
* @file   bifrost_component_renderer.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-07-10
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bifrost/graphics/bifrost_component_renderer.hpp"

#include "bifrost/core/bifrost_engine.hpp"
#include "bifrost/ecs/bifrost_entity.hpp"
#include "bifrost/ecs/bifrost_renderer_component.hpp"  // MeshRenderer

namespace bifrost
{
  void ComponentRenderer::onInit(Engine& engine)
  {
    const auto& gfx_device    = engine.renderer().device();
    auto&       glsl_compiler = engine.renderer().glslCompiler();

    m_ShaderModules[0] = glsl_compiler.createModule(gfx_device, "assets/shaders/sprite/sprite.vert.glsl");
    m_ShaderModules[1] = glsl_compiler.createModule(gfx_device, "assets/shaders/sprite/sprite.frag.glsl");
    m_ShaderProgram    = gfx::createShaderProgram(gfx_device, 4, m_ShaderModules[0], m_ShaderModules[1], "Renderer.Sprite");

    bfShaderProgram_link(m_ShaderProgram);

    // bindings::addObject(m_ShaderProgram, BIFROST_SHADER_STAGE_VERTEX);
    bindings::addMaterial(m_ShaderProgram, BIFROST_SHADER_STAGE_FRAGMENT);
    bindings::addCamera(m_ShaderProgram, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(m_ShaderProgram);

    m_SpriteVertexBuffer = engine.mainMemory().allocateT<VertexBuffer>(engine.mainMemory());

    m_SpriteVertexBuffer->init(gfx_device);
  }

  void ComponentRenderer::onFrameDraw(Engine& engine, CameraRender& camera, float alpha)
  {
    const auto scene = engine.currentScene();

    if (scene)
    {
      auto&      engine_renderer = engine.renderer();
      auto&      tmp_memory      = engine.tempMemory();
      const auto cmd_list        = engine_renderer.mainCommandList();
      const auto frame_info      = engine_renderer.frameInfo();

      // 3D Models

      // TODO(SR):
      //   - Sorting based on distance, material, transparency.
      //   - Culling based on the view fustrum.
      //     - [http://www.lighthouse3d.com/tutorials/view-frustum-culling/]
      //     - [http://www.lighthouse3d.com/tutorials/view-frustum-culling/clip-space-approach-implementation-details/]
      //     - [http://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-source-code/]
      //     - [https://github.com/gametutorials/tutorials/blob/master/OpenGL/Frustum%20Culling/Frustum.cpp]
      //     - [http://www.rastertek.com/dx10tut16.html]

      for (MeshRenderer& renderer : scene->components<MeshRenderer>())
      {
        if (renderer.material() && renderer.model())
        {
          engine_renderer.bindMaterial(cmd_list, *renderer.material());
          engine_renderer.bindObject(cmd_list, camera.gpu_camera, renderer.owner());
          renderer.model()->draw(cmd_list);
        }
      }

      // 2D Sprites

      // TODO(SR):
      //   - Sorting based on distance, material, transparency.
      //   - Culling based on the view fustrum.

      auto& sprite_renderer_list = scene->components<SpriteRenderer>();

      SpriteRenderer** sprite_list      = tmp_memory.allocateArray<SpriteRenderer*>(sprite_renderer_list.size());
      int              sprite_list_size = 0;

      for (SpriteRenderer& renderer : sprite_renderer_list)
      {
        // TODO(SR): Culling would go here.
        if (renderer.material() && renderer.size().x > 0.0f && renderer.size().y > 0.0f)
        {
          sprite_list[sprite_list_size++] = &renderer;
        }
      }

      if (sprite_list_size)
      {
        m_SpriteVertexBuffer->clear();

        std::sort(
         sprite_list,
         sprite_list + sprite_list_size,
         [](SpriteRenderer* a, SpriteRenderer* b) -> bool {
           return &*a->material() < &*b->material();
         });

        struct SpriteBatch final
        {
          Material*           material;
          int                 vertex_offset;
          int                 num_vertices;
          VertexBuffer::Link* vertex_buffer;
          SpriteBatch*        next_batch;
        };

        SpriteBatch* batches    = nullptr;
        SpriteBatch* last_batch = nullptr;

        for (int i = 0; i < sprite_list_size; ++i)
        {
          SpriteRenderer&                       sprite          = *sprite_list[i];
          Material* const                       sprite_mat      = &*sprite.material();
          const std::pair<StandardVertex*, int> vertices_offset = m_SpriteVertexBuffer->requestVertices(frame_info, 6);
          StandardVertex* const                 vertices        = vertices_offset.first;
          const Vector2f&                       sprite_size     = sprite.size();
          const bfColor4u&                      sprite_color    = sprite.color();

          if (!last_batch || last_batch->material != sprite_mat)
          {
            last_batch                = tmp_memory.allocateT<SpriteBatch>();
            last_batch->material      = sprite_mat;
            last_batch->vertex_offset = vertices_offset.second;
            last_batch->num_vertices  = 0;
            last_batch->vertex_buffer = m_SpriteVertexBuffer->currentLink();
            last_batch->next_batch    = batches;
            batches                   = last_batch;
          }

          const BifrostTransform& transform     = sprite.owner().transform();
          const Mat4x4&           transform_mat = transform.world_transform;
          const Vector3f&         origin        = transform.world_position;
          Vector3f                x_axis        = {sprite_size.x, 0.0f, 0.0f, 0.0f};
          Vector3f                y_axis        = {0.0f, sprite_size.y, 0.0f, 0.0f};

          Mat4x4_multVec(&transform_mat, &x_axis, &x_axis);
          Mat4x4_multVec(&transform_mat, &y_axis, &y_axis);

          const Vector3f half_x_axis = x_axis * 0.5f;
          const Vector3f half_y_axis = y_axis * 0.5f;

          // TODO(SR): This can probably be micro-optimized to do less redundant math ops.

          const Vector3f positions[] =
           {
            origin - half_x_axis - half_y_axis,
            origin + half_x_axis - half_y_axis,
            origin - half_x_axis + half_y_axis,
            origin + half_x_axis + half_y_axis,
           };

          const auto& uv_rect = sprite.uvRect();

          const Vector2f uvs[] =
           {
            uv_rect.topLeft(),
            uv_rect.topRight(),
            uv_rect.bottomLeft(),
            uv_rect.bottomRight(),
           };

          const auto& sprite_normal = vec::faceNormal(positions[0], positions[1], positions[2]);

          auto addVertex = [num_verts = 0, vertices, &sprite_normal, &sprite_color](const Vector3f& pos, const Vector2f& uv) mutable {
            static const Vector3f k_SpriteTangent = {0.0f, 1.0f, 0.0f, 0.0f};

            vertices[num_verts++] = StandardVertex{
             pos,
             sprite_normal,
             k_SpriteTangent,
             sprite_color,
             uv,
            };
          };

          // TODO(Shareef): Use an index buffer.

          for (int index : {0, 1, 2, 1, 3, 2})
          {
            addVertex(positions[index], uvs[index]);
          }

          last_batch->num_vertices += 6;
        }

        for (VertexBuffer::Link* const link : m_SpriteVertexBuffer->used_buffers)
        {
          link->gpu_buffer.flushCurrent(frame_info);
          bfBuffer_unMap(link->gpu_buffer.handle());
        }

        bfGfxCmdList_setCullFace(cmd_list, BIFROST_CULL_FACE_NONE);
        bfGfxCmdList_bindProgram(cmd_list, m_ShaderProgram);
        bfGfxCmdList_bindVertexDesc(cmd_list, engine_renderer.standardVertexLayout());

        camera.gpu_camera.bindDescriptorSet(cmd_list, frame_info);

        VertexBuffer::Link* last_link     = nullptr;
        Material*           last_material = nullptr;

        while (batches)
        {
          if (batches->num_vertices)
          {
            if (batches->vertex_buffer != last_link)
            {
              last_link = batches->vertex_buffer;

              const bfBufferSize offset = last_link->gpu_buffer.offset(frame_info);

              bfGfxCmdList_bindVertexBuffers(cmd_list, 0, &last_link->gpu_buffer.handle(), 1, &offset);
            }

            if (batches->material != last_material)
            {
              last_material = batches->material;

              engine_renderer.bindMaterial(cmd_list, *last_material);
            }

            bfGfxCmdList_draw(cmd_list, batches->vertex_offset, batches->num_vertices);
          }

          batches = batches->next_batch;
        }

        bfGfxCmdList_setCullFace(cmd_list, BIFROST_CULL_FACE_BACK);
      }
    }
  }

  void ComponentRenderer::onDeinit(Engine& engine)
  {
    const auto& gfx_device = engine.renderer().device();

    bfGfxDevice_release(gfx_device, m_ShaderModules[0]);
    bfGfxDevice_release(gfx_device, m_ShaderModules[1]);
    bfGfxDevice_release(gfx_device, m_ShaderProgram);

    m_SpriteVertexBuffer->deinit();
    engine.mainMemory().deallocateT(m_SpriteVertexBuffer);
  }
}  // namespace bifrost
