#include "bf/anim2D/bf_animation_system.hpp"

#include "bf/asset_io/bf_path_manip.hpp"
#include "bf/asset_io/bf_spritesheet_asset.hpp"
#include "bf/asset_io/bf_document.hpp"
#include "bf/ecs/bf_entity.hpp"

#include "bf/core/bifrost_engine.hpp"

namespace bf
{
  static const bfTextureSamplerProperties k_SamplerNearestRepeat = bfTextureSamplerProperties_init(BF_SFM_NEAREST, BF_SAM_REPEAT);

  void AnimationSystem::onInit(Engine& engine)
  {
    const bfAnim2DCreateParams create_anim_ctx = {nullptr, &engine};

    m_Anim2DCtx = bfAnim2D_new(&create_anim_ctx);
  }

  template<typename T, typename F>
  T lerpAtTime(const Anim3DAsset&           animation,
               const Anim3DAsset::Track<T>& track,
               AnimationTimeType            animation_time,
               T                            default_value,
               F&&                          lerp_fn)
  {
    const std::size_t num_keys = track.numKeys(animation.m_Memory);

    if (num_keys == 0)
    {
      return default_value;
    }

    if (num_keys == 1)
    {
      return track.keys[0].value;
    }

    const std::size_t x_idx_curr = track.findKey(animation_time, animation.m_Memory);
    const std::size_t x_idx_next = (x_idx_curr + 1) % num_keys;

    // assert(x_idx_next + 1 < num_keys);

    const AnimationTimeType curr_time   = track.keys[x_idx_curr].time;
    const AnimationTimeType next_time   = track.keys[x_idx_next].time;
    const AnimationTimeType delta_time  = next_time - curr_time;
    const float             lerp_factor = float((animation_time - curr_time) / delta_time);

    assert(math::isInbetween(0.0f, lerp_factor, 1.0f));

    const T start_value = track.keys[x_idx_curr].value;
    const T end_value   = track.keys[x_idx_next].value;

    return lerp_fn(start_value, lerp_factor, end_value);
  }

  Vector3f vec3ValueAtTime(
   const Anim3DAsset&              animation,
   const Anim3DAsset::TripleTrack& track,
   AnimationTimeType               animation_time,
   float                           default_value)
  {
    const float x = lerpAtTime<float>(animation, track.x, animation_time, default_value, &math::lerp<float, float>);
    const float y = lerpAtTime<float>(animation, track.y, animation_time, default_value, &math::lerp<float, float>);
    const float z = lerpAtTime<float>(animation, track.z, animation_time, default_value, &math::lerp<float, float>);

    //
    // The w = 0.0f for vectors   because the default value would be 1.0f and
    //     w = 1.0f for positions because the default value would be 0.0f
    //
    return {x, y, z, 1.0f - default_value};
  }

  bfQuaternionf quatValueAtTime(
   const Anim3DAsset&                       animation,
   const Anim3DAsset::Track<bfQuaternionf>& track,
   AnimationTimeType                        animation_time)
  {
    bfQuaternionf value = lerpAtTime<bfQuaternionf>(
     animation,
     track,
     animation_time,
     bfQuaternionf_identity(),
     [](const bfQuaternionf& start, float factor, const bfQuaternionf& end) {
       return bfQuaternionf_slerp(&start, &end, factor);
     });

    bfQuaternionf_normalize(&value);

    return value;
  }

  static void updateNodeAnimation(
   const ModelAsset::Node*                      root_node,
   const Anim3DAsset&                           animation,
   AnimationTimeType                            animation_time,
   const ModelAsset::Node*                      node,
   const HashTable<std::uint8_t, std::uint8_t>& bone_to_channel,
   const Matrix4x4f&                            global_inv_transform,
   const ModelAsset::NodeIDBone*                input_transform,
   Matrix4x4f*                                  output_transform,
   const Matrix4x4f&                            parent_transform)
  {
    Matrix4x4f node_transform;  //  = node->transform;

    if (node->bone_idx != k_InvalidBoneID)
    {
      const auto it = bone_to_channel.find(node->bone_idx);

      if (it != bone_to_channel.end())
      {
        const std::uint8_t    channel_index = it->value();
        Anim3DAsset::Channel& channel       = animation.m_Channels[channel_index];
        const Vector3f        scale         = vec3ValueAtTime(animation, channel.scale, animation_time, 1.0f);
        const bfQuaternionf   rotation      = quatValueAtTime(animation, channel.rotation, animation_time);
        const Vector3f        translation   = vec3ValueAtTime(animation, channel.translation, animation_time, 0.0f);

        Matrix4x4f scale_mat;
        Matrix4x4f rotation_mat;
        Matrix4x4f translation_mat;

        Mat4x4_initScalef(&scale_mat, scale.x, scale.y, scale.z);
        bfQuaternionf_toMatrix(rotation, &rotation_mat);
        Mat4x4_initTranslatef(&translation_mat, translation.x, translation.y, translation.z);

        Mat4x4_mult(&rotation_mat, &scale_mat, &node_transform);
        Mat4x4_mult(&translation_mat, &node_transform, &node_transform);
      }
      else  // Bone was not part of the animation
      {
        node_transform = node->transform;
      }
    }
    else
    {
      node_transform = node->transform;
    }

    Matrix4x4f global_transform;
    Mat4x4_mult(&parent_transform, &node_transform, &global_transform);

    if (node->bone_idx != k_InvalidBoneID)
    {
      // out = global_inv_transform * global_transform * inv_bone
      Matrix4x4f* const out = output_transform + node->bone_idx;

      Mat4x4_mult(&global_transform, &input_transform[node->bone_idx].transform, out);
      Mat4x4_mult(&global_inv_transform, out, out);
    }

    std::for_each_n(
     root_node + node->first_child,
     node->num_children,
     [&](const ModelAsset::Node& child_node) -> void {
       updateNodeAnimation(
        root_node,
        animation,
        animation_time,
        &child_node,
        bone_to_channel,
        global_inv_transform,
        input_transform,
        output_transform,
        global_transform);
     });
  }

  void AnimationSystem::onFrameUpdate(Engine& engine, float dt)
  {
    bfAnim2DChangeEvent ss_change_evt;

    if (bfAnim2D_networkClientUpdate(m_Anim2DCtx, &ss_change_evt))
    {
      if (ss_change_evt.type == bfAnim2DChange_Texture)
      {
        bfSpritesheet*          spritesheet = ss_change_evt.spritesheet;
        SpritesheetAsset* const ss_info     = static_cast<SpritesheetAsset*>(spritesheet->user_data);
        const StringRange       ss_dir      = path::directory(ss_info->document().fullPath());
        const String            texture_dir = path::append(ss_dir, path::nameWithoutExtension(StringRange{spritesheet->name.str, spritesheet->name.str_len})) + ".png";
        Assets&                 assets      = engine.assets();

        __debugbreak();
#if 0
      TextureAsset* const     texture     = assets.findAssetOfType<TextureAsset>(AbsPath(texture_dir));

      // This only needs a to update the texture if it is currently being used.
      if (texture && texture->document().refCount() > 0)
      {
        ARC<TextureAsset> keep_texture_alive = texture;

        texture->assignNewHandle(
         gfx::createTexturePNG(
          engine->renderer().m_GfxDevice,
          bfTextureCreateParams_init2D(
           BF_IMAGE_FORMAT_R8G8B8A8_UNORM,
           k_bfTextureUnknownSize,
           k_bfTextureUnknownSize),
          k_SamplerNearestRepeat,
          change_event.texture.texture_bytes_png,
          change_event.texture.texture_bytes_png_size));
      }
#endif
      }
    }

    const auto scene = engine.currentScene();

    if (scene)
    {
      auto& engine_renderer = engine.renderer();
      auto& anim_sprites    = scene->components<SpriteAnimator>();

      struct BackPtrs
      {
        SpriteAnimator* animator;
        SpriteRenderer* renderer;
      };

      // TODO(SR): This should use a linear allocator memory scope.

      bfAnim2DUpdateInfo*   update_info = engine.tempMemory().allocateArrayTrivial<bfAnim2DUpdateInfo>(anim_sprites.size());
      BackPtrs*             back_ptrs   = engine.tempMemory().allocateArrayTrivial<BackPtrs>(anim_sprites.size());
      const bfSpritesheet** sheets      = engine.tempMemory().allocateArrayTrivial<const bfSpritesheet*>(anim_sprites.size());
      std::uint16_t         num_sprites = 0u;

      for (auto& anim_sprite : anim_sprites)
      {
        SpriteRenderer* const sprite = anim_sprite.owner().get<SpriteRenderer>();

        if (sprite && anim_sprite.m_Spritesheet && anim_sprite.m_Anim2DUpdateInfo.animation < anim_sprite.m_Spritesheet->spritesheet()->num_animations)
        {
          const auto current_idx = num_sprites++;

          back_ptrs[current_idx].animator          = &anim_sprite;
          back_ptrs[current_idx].renderer          = sprite;
          update_info[current_idx]                 = anim_sprite.m_Anim2DUpdateInfo;
          update_info[current_idx].spritesheet_idx = current_idx;
          sheets[current_idx]                      = anim_sprite.m_Spritesheet->spritesheet();
        }
      }

      bfAnim2D_stepFrame(update_info, sheets, num_sprites, dt);

      for (std::uint16_t i = 0u; i < num_sprites; ++i)
      {
        SpriteRenderer* const     sprite = back_ptrs[i].renderer;
        const bfAnim2DUpdateInfo& result = update_info[i];
        const bfSpritesheet*      sheet  = sheets[result.spritesheet_idx];
        const bfUVRect            rect   = sheet->uvs[sheet->animations->frames[result.current_frame].frame_index];

        sprite->uvRect() = {rect.x, rect.y, rect.width, rect.height};

        back_ptrs[i].animator->m_Anim2DUpdateInfo.time_left_for_frame = result.time_left_for_frame;
        back_ptrs[i].animator->m_Anim2DUpdateInfo.current_frame       = result.current_frame;
      }

      Matrix4x4f identity;
      Mat4x4_identity(&identity);

      for (auto& mesh : scene->components<SkinnedMeshRenderer>())
      {
        const auto& model            = mesh.model();
        const auto& animation_handle = mesh.m_Animation;

        if (mesh.material() && model && animation_handle)
        {
          auto* const             animation             = &*animation_handle;
          const AnimationTimeType duration              = animation->m_Duration;
          const AnimationTimeType current_time          = mesh.m_CurrentTime;
          const AnimationTimeType current_time_in_ticks = current_time * animation->m_TicksPerSecond;
          const AnimationTimeType animation_time        = std::fmod(current_time_in_ticks, duration);

          mesh.m_CurrentTime += double(dt);

          //
          // TODO(SR): This can be baked once \/\/\/\/\/
          //

          //
          // NOTE(SR): This data must be backed per {ModelAsset, Anim3DAsset} pair.
          //

          //
          // If this was backed once then in the main loop no string lookups are needed.
          //

          // TODO(SR): Maybe this can be an array rather than a map.
          HashTable<std::uint8_t, std::uint8_t> bone_to_channel = {};

          std::uint8_t bone_index = 0u;
          for (const auto& bones : model->m_BoneToModel)
          {
            const ModelAsset::Node& node = model->m_Nodes[bones.node_idx];
            const auto              it   = animation->m_NameToChannel.find(node.name);

            assert(node.bone_idx == bone_index);

            if (it != animation->m_NameToChannel.end())
            {
              bone_to_channel.insert(bone_index, it->value());
            }

            ++bone_index;
          }

          //
          // TODO(SR): This can be baked once /\/\/\/\/\
          //

          ModelAsset::Node&           root_node         = model->m_Nodes[0];
          Renderable<ObjectBoneData>& uniform_bone_data = getRenderable(engine_renderer, mesh.owner());
          const bfBufferSize          offset            = uniform_bone_data.transform_uniform.offset(engine_renderer.frameInfo());
          const bfBufferSize          size              = sizeof(ObjectBoneData);
          ObjectBoneData* const       obj_data          = static_cast<ObjectBoneData*>(bfBuffer_map(uniform_bone_data.transform_uniform.handle(), offset, size));
          Matrix4x4f*                 output_bones      = obj_data->u_Bones;

          std::for_each_n(
           output_bones,
           model->m_BoneToModel.size(),
           [](Matrix4x4f& mat) {
             Mat4x4_identity(&mat);
           });

          updateNodeAnimation(
           &root_node,
           *animation,
           animation_time,
           &root_node,
           bone_to_channel,
           model->m_GlobalInvTransform,
           model->m_BoneToModel.data(),
           output_bones,
           identity);

          uniform_bone_data.transform_uniform.flushCurrent(engine_renderer.frameInfo(), size);
          bfBuffer_unMap(uniform_bone_data.transform_uniform.handle());
        }
      }
    }
  }

  void AnimationSystem::onDeinit(Engine& engine)
  {
    bfAnim2D_delete(m_Anim2DCtx);

    for (auto& r : m_RenderablePool)
    {
      r.destroy(engine.renderer().device());
    }
    m_RenderablePool.clear();
    m_Renderables.clear();
  }

  Renderable<ObjectBoneData>& AnimationSystem::getRenderable(StandardRenderer& renderer, Entity& entity)
  {
    auto it = m_Renderables.find(&entity);

    Renderable<ObjectBoneData>* renderable;

    if (it == m_Renderables.end())
    {
      renderable = &m_RenderablePool.emplaceFront();
      renderable->create(renderer.device(), renderer.frameInfo());
      m_Renderables.emplace(&entity, renderable);
    }
    else
    {
      renderable = it->value();
    }

    return *renderable;
  }
}  // namespace bf
