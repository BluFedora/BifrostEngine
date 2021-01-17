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
#include "bf/graphics/bifrost_component_renderer.hpp"

#include "bf/anim2D/bf_animation_system.hpp"
#include "bf/core/bifrost_engine.hpp"
#include "bf/ecs/bifrost_entity.hpp"
#include "bf/ecs/bifrost_renderer_component.hpp"  // MeshRenderer

int g_NumDrawnObjects;

namespace bf
{
  void ComponentRenderer::onInit(Engine& engine)
  {
    const auto& gfx_device    = engine.renderer().device();
    auto&       glsl_compiler = engine.renderer().glslCompiler();

    m_ShaderModules[0] = glsl_compiler.createModule(gfx_device, "assets/shaders/sprite/sprite.vert.glsl");
    m_ShaderModules[1] = glsl_compiler.createModule(gfx_device, "assets/shaders/sprite/sprite.frag.glsl");
    m_ShaderProgram    = gfx::createShaderProgram(gfx_device, 4, m_ShaderModules[0], m_ShaderModules[1], "Renderer.Sprite");

    bfShaderProgram_link(m_ShaderProgram);

    bindings::addMaterial(m_ShaderProgram, BF_SHADER_STAGE_FRAGMENT);
    bindings::addCamera(m_ShaderProgram, BF_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(m_ShaderProgram);

    auto& memory = engine.mainMemory();

    m_SpriteVertexBuffer = memory.allocateT<VertexBuffer>(memory);
    m_SpriteVertexBuffer->init(gfx_device);

#if k_UseIndexBufferForSprites
    m_SpriteIndexBuffer = memory.allocateT<IndexBuffer>(memory);
    m_SpriteIndexBuffer->init(gfx_device);
#endif

    m_PerFrameSprites = engine.mainMemory().allocateT<Array<Renderable2DPrimitive>>(engine.mainMemory());
  }

  void ComponentRenderer::onFrameBegin(Engine& engine, float dt)
  {
    m_PerFrameSprites->clear();
  }

  void ComponentRenderer::onFrameDraw(Engine& engine, RenderView& camera, float alpha)
  {
    g_NumDrawnObjects = 0;

    const auto scene = engine.currentScene();

    if (scene)
    {
      auto&      engine_renderer = engine.renderer();
      auto&      tmp_memory      = engine.tempMemory();
      const auto frame_info      = engine_renderer.frameInfo();
      auto&      bvh             = scene->bvh();

      // Mark Visibility

      //
      // NOTE(SR): This can be optimized with Plane masking:
      //   [https://cesium.com/blog/2015/08/04/fast-hierarchical-culling/]
      //

      bvh.traverseConditionally([&cpu_camera = camera.cpu_camera, &bvh](BVHNode& node) {
        if (bvh_node::isLeaf(node))
        {
          node.is_visible = true;
          return false;
        }

        const AABB& bounds = node.bounds;

        Vector3f aabb_min;
        Vector3f aabb_max;

        memcpy(&aabb_min, &bounds.min, sizeof(float) * 3);
        memcpy(&aabb_max, &bounds.max, sizeof(float) * 3);

        const auto hit_result = bfFrustum_isAABBInside(&cpu_camera.frustum, aabb_min, aabb_max);

        switch (hit_result)
        {
          case BF_FRUSTUM_TEST_OUTSIDE:
          {
            bvh.traverse(bvh.nodeToIndex(node), [](BVHNode& node) {
              node.is_visible = false;
            });

            return false;
          }
          case BF_FRUSTUM_TEST_INTERSECTING:
          {
            return true;
          }
          case BF_FRUSTUM_TEST_INSIDE:
          {
            bvh.traverse(bvh.nodeToIndex(node), [](BVHNode& node) {
              node.is_visible = true;
            });

            return false;
          }
            bfInvalidDefaultCase();
        }
      });

      // 3D Models

      RenderQueue& opaque_render_queue      = camera.opaque_render_queue;
      RenderQueue& transparent_render_queue = camera.transparent_render_queue;

      bfDrawCallPipeline pipeline;
      bfDrawCallPipeline_defaultOpaque(&pipeline);

      pipeline.state.do_depth_test  = bfTrue;
      pipeline.state.do_depth_write = bfTrue;

      pipeline.program       = engine_renderer.m_GBufferShader;
      pipeline.vertex_layout = engine_renderer.m_StandardVertexLayout;

      for (MeshRenderer& renderer : scene->components<MeshRenderer>())
      {
        if (renderer.material() && renderer.model() && bvh.nodes[renderer.m_BHVNode].is_visible)
        {
          ModelAsset& model = *renderer.model();

          for (Mesh& mesh : model.m_Meshes)
          {
            RC_DrawIndexed* const render_command = opaque_render_queue.drawIndexed(pipeline, 2, model.m_IndexBuffer);
            MaterialAsset*        material       = model.m_Materials[mesh.material_idx];

            render_command->material_binding.set(engine_renderer.makeMaterialInfo(*material));
            render_command->object_binding.set(engine_renderer.makeObjectTransformInfo(camera.cpu_camera.view_proj_cache, camera.gpu_camera, renderer.owner()));

            render_command->vertex_buffers[0]         = model.m_VertexBuffer;
            render_command->vertex_buffers[1]         = model.m_VertexBoneData;
            render_command->vertex_binding_offsets[0] = 0u;
            render_command->vertex_binding_offsets[1] = 0u;
            render_command->index_offset              = mesh.index_offset;
            render_command->num_indices               = mesh.num_indices;

            opaque_render_queue.submit(render_command, 0.0f);

            ++g_NumDrawnObjects;
          }
        }
      }

      auto& anim_sys = engine.animationSys();

      pipeline.program       = engine_renderer.m_GBufferSkinnedShader;
      pipeline.vertex_layout = engine_renderer.m_SkinnedVertexLayout;

      for (SkinnedMeshRenderer& renderer : scene->components<SkinnedMeshRenderer>())
      {
        if (renderer.material() && renderer.model() && bvh.nodes[renderer.m_BHVNode].is_visible)
        {
          const ModelAsset& model = *renderer.model();

          if (model.numBones() > 0)  // A DescriptorSet write with size of 0 is not allowed.
          {
            auto&              uniform_bone_data = anim_sys.getRenderable(engine_renderer, renderer.owner());
            const bfBufferSize offset            = uniform_bone_data.transform_uniform.offset(frame_info);

            for (const Mesh& mesh : model.m_Meshes)
            {
              RC_DrawIndexed* const render_command  = opaque_render_queue.drawIndexed(pipeline, 2, model.m_IndexBuffer);
              MaterialAsset*        material        = model.m_Materials[mesh.material_idx];
              bfDescriptorSetInfo   desc_set_object = engine_renderer.makeObjectTransformInfo(camera.cpu_camera.view_proj_cache, camera.gpu_camera, renderer.owner());
              const bfBufferSize    size            = sizeof(Mat4x4) * model.numBones();

              bfDescriptorSetInfo_addUniform(&desc_set_object, 1, 0, &offset, &size, &uniform_bone_data.transform_uniform.handle(), 1);

              render_command->material_binding.set(engine_renderer.makeMaterialInfo(*material));
              render_command->object_binding.set(desc_set_object);

              render_command->vertex_buffers[0]         = model.m_VertexBuffer;
              render_command->vertex_buffers[1]         = model.m_VertexBoneData;
              render_command->vertex_binding_offsets[0] = 0u;
              render_command->vertex_binding_offsets[1] = 0u;
              render_command->index_offset              = mesh.index_offset;
              render_command->num_indices               = mesh.num_indices;

              opaque_render_queue.submit(render_command, 0.0f);

              ++g_NumDrawnObjects;
            }
          }
        }
      }

      // 2D Sprites

      auto&                  sprite_renderer_list  = scene->components<SpriteRenderer>();
      const std::size_t      num_per_frame_sprites = m_PerFrameSprites->size();
      Renderable2DPrimitive* sprite_list           = tmp_memory.allocateArray<Renderable2DPrimitive>(sprite_renderer_list.size() + num_per_frame_sprites);
      std::size_t            sprite_list_size      = 0;

      const auto add_sprite_to_list = [sprite_list, &sprite_list_size, &bvh](SpriteRenderer& renderer) {
        if (renderer.size().x > 0.0f && renderer.size().y > 0.0f && renderer.material() && bvh.nodes[renderer.m_BHVNode].is_visible)
        {
          Renderable2DPrimitive& dst_sprite = sprite_list[sprite_list_size++];
          const bfTransform&     transform  = renderer.owner().transform();

          dst_sprite.transform = transform.world_transform;
          dst_sprite.material  = &*renderer.material();
          dst_sprite.origin    = transform.world_position;
          dst_sprite.size      = renderer.size();
          dst_sprite.color     = renderer.color();
          dst_sprite.uv_rect   = renderer.uvRect();

          ++g_NumDrawnObjects;
        }
      };

      for (SpriteRenderer& renderer : sprite_renderer_list)
      {
        add_sprite_to_list(renderer);
      }

      if (num_per_frame_sprites)
      {
        std::memcpy(sprite_list + sprite_list_size, m_PerFrameSprites->data(), num_per_frame_sprites * sizeof(Renderable2DPrimitive));
        sprite_list_size += num_per_frame_sprites;
      }

      if (sprite_list_size)
      {
        m_SpriteVertexBuffer->clear();
#if k_UseIndexBufferForSprites
        m_SpriteIndexBuffer->clear();
#endif
        std::sort(
         sprite_list,
         sprite_list + sprite_list_size,
         [](const Renderable2DPrimitive& a, const Renderable2DPrimitive& b) -> bool {
           return a.material < b.material;
         });

        struct SpriteBatch final
        {
          MaterialAsset*      material;
          bfDescriptorSetInfo material_desc;
          SpriteIndexType     vertex_offset;
          SpriteIndexType     num_vertices;
          VertexBuffer::Link* vertex_buffer;
#if k_UseIndexBufferForSprites
          int                index_offset;
          int                num_indices;
          IndexBuffer::Link* index_buffer;
#endif
          SpriteBatch* next_batch;
        };

        SpriteBatch* batches    = nullptr;
        SpriteBatch* last_batch = nullptr;

        for (std::size_t i = 0; i < sprite_list_size; ++i)
        {
          Renderable2DPrimitive&                sprite          = sprite_list[i];
          MaterialAsset* const                  sprite_mat      = sprite.material;
          const std::pair<StandardVertex*, int> vertices_offset = m_SpriteVertexBuffer->requestVertices(frame_info, k_NumVerticesPerSprite);
#if k_UseIndexBufferForSprites
          const std::pair<SpriteIndexType*, int> index_offset = m_SpriteIndexBuffer->requestVertices(frame_info, k_NumIndicesPerSprite);
#endif
          StandardVertex* const vertices     = vertices_offset.first;
          const Vector2f&       sprite_size  = sprite.size;
          const bfColor4u&      sprite_color = sprite.color;

          if (!last_batch || last_batch->material != sprite_mat)
          {
            last_batch = tmp_memory.allocateT<SpriteBatch>();

            assert(last_batch != nullptr);

            last_batch->material      = sprite_mat;
            last_batch->material_desc = engine_renderer.makeMaterialInfo(*sprite_mat);
            last_batch->vertex_offset = SpriteIndexType(vertices_offset.second);
            last_batch->num_vertices  = 0;
            last_batch->vertex_buffer = m_SpriteVertexBuffer->currentLink();
#if k_UseIndexBufferForSprites
            last_batch->num_indices  = 0;
            last_batch->index_offset = index_offset.second;
            last_batch->index_buffer = m_SpriteIndexBuffer->currentLink();
#endif
            last_batch->next_batch = batches;
            batches                = last_batch;
          }

          const Mat4x4&   transform_mat = sprite.transform;
          const Vector3f& origin        = sprite.origin;
          Vector3f        x_axis        = {sprite_size.x, 0.0f, 0.0f, 0.0f};
          Vector3f        y_axis        = {0.0f, sprite_size.y, 0.0f, 0.0f};

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

          const auto& uv_rect = sprite.uv_rect;

          const Vector2f uvs[] =
           {
            uv_rect.bottomLeft(),
            uv_rect.bottomRight(),
            uv_rect.topLeft(),
            uv_rect.topRight(),
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

#if k_UseIndexBufferForSprites
          const SpriteIndexType index_vertex_offset = last_batch->vertex_offset + last_batch->num_vertices;
          SpriteIndexType*      indices             = index_offset.first;

          addVertex(positions[0], uvs[0]);
          addVertex(positions[1], uvs[1]);
          addVertex(positions[2], uvs[2]);
          addVertex(positions[3], uvs[3]);

          indices[0] = index_vertex_offset + 0;
          indices[1] = index_vertex_offset + 1;
          indices[2] = index_vertex_offset + 2;
          indices[3] = index_vertex_offset + 1;
          indices[4] = index_vertex_offset + 3;
          indices[5] = index_vertex_offset + 2;

          last_batch->num_indices += k_NumIndicesPerSprite;
#else
          for (int index : {0, 1, 2, 1, 3, 2})
          {
            addVertex(positions[index], uvs[index]);
          }
#endif

          last_batch->num_vertices += k_NumVerticesPerSprite;
        }

        m_SpriteVertexBuffer->flushLinks(frame_info);

#if k_UseIndexBufferForSprites
        m_SpriteIndexBuffer->flushLinks(frame_info);
#endif

        pipeline.state.cull_face = BF_CULL_FACE_NONE;
        pipeline.program         = m_ShaderProgram;
        pipeline.vertex_layout   = engine_renderer.standardVertexLayout();

        while (batches)
        {
          if (batches->num_vertices)
          {
#if k_UseIndexBufferForSprites
            RC_DrawIndexed* const render_command = transparent_render_queue.drawIndexed(pipeline, 1, batches->index_buffer->gpu_buffer.handle());

            render_command->material_binding.set(batches->material_desc);
            render_command->vertex_buffers[0]           = batches->vertex_buffer->gpu_buffer.handle();
            render_command->vertex_binding_offsets[0]   = batches->vertex_buffer->gpu_buffer.offset(frame_info);
            render_command->index_buffer_binding_offset = batches->index_buffer->gpu_buffer.offset(frame_info);
            render_command->index_offset                = batches->index_offset;
            render_command->num_indices                 = batches->num_indices;
            render_command->index_type                  = k_SpriteIndexType;

#else
            RC_DrawArrays* const render_command = render_queue.drawArrays(pipeline, 1);

            render_command->material_binding.set(batches->material_desc);
            render_command->vertex_buffers[0]         = batches->vertex_buffer->gpu_buffer.handle();
            render_command->vertex_binding_offsets[0] = batches->vertex_buffer->gpu_buffer.offset(frame_info);
            render_command->first_vertex              = batches->vertex_offset;
            render_command->num_vertices              = batches->num_vertices;

#endif

            transparent_render_queue.submit(render_command, 0.0f);
          }

          batches = batches->next_batch;
        }
      }
    }
  }

  void ComponentRenderer::onDeinit(Engine& engine)
  {
    const auto& gfx_device = engine.renderer().device();
    auto&       memory     = engine.mainMemory();

    bfGfxDevice_release(gfx_device, m_ShaderModules[0]);
    bfGfxDevice_release(gfx_device, m_ShaderModules[1]);
    bfGfxDevice_release(gfx_device, m_ShaderProgram);

#if k_UseIndexBufferForSprites
    m_SpriteIndexBuffer->deinit();
    memory.deallocateT(m_SpriteIndexBuffer);
#endif

    m_SpriteVertexBuffer->deinit();
    memory.deallocateT(m_SpriteVertexBuffer);

    engine.mainMemory().deallocateT(m_PerFrameSprites);
  }

  void ComponentRenderer::pushSprite(const Renderable2DPrimitive& sprite) const
  {
    m_PerFrameSprites->push(sprite);
  }
}  // namespace bf
