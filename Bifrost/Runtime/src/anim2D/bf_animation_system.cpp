#include "bf/anim2D/bf_animation_system.hpp"

#include "bf/asset_io/bf_path_manip.hpp"
#include "bf/asset_io/bf_spritesheet_asset.hpp"
#include "bf/ecs/bf_entity.hpp"

#include "bifrost/core/bifrost_engine.hpp"

namespace bf
{
  static const bfTextureSamplerProperties k_SamplerNearestRepeat = bfTextureSamplerProperties_init(BIFROST_SFM_NEAREST, BIFROST_SAM_REPEAT);

  static void onSSChange(bfAnimation2DCtx* ctx, bfSpritesheet* spritesheet, bfAnim2DChangeEvent change_event)
  {
    if (change_event.type == bfAnim2DChange_Texture)
    {
      Engine* const               engine       = static_cast<Engine*>(bfAnimation2D_userData(ctx));
      AssetSpritesheetInfo* const ss_info      = static_cast<AssetSpritesheetInfo*>(spritesheet->user_data);
      const StringRange           ss_dir       = path::directory(ss_info->filePathAbs());
      const String                texture_dir  = path::append(ss_dir, path::nameWithoutExtension(StringRange{spritesheet->name.str, spritesheet->name.str_len})) + ".png";
      Assets&                     assets       = engine->assets();
      const BifrostUUID           texture_guid = assets.indexAsset<AssetTextureInfo>(texture_dir);
      AssetTextureInfo* const     texture_info = static_cast<AssetTextureInfo*>(assets.findAssetInfo(texture_guid));

      // This only needs a reload if it is currently being used.
      if (texture_info && texture_info->refCount())
      {
        const bfTextureCreateParams create_params = bfTextureCreateParams_init2D(
         BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM,
         BIFROST_TEXTURE_UNKNOWN_SIZE,
         BIFROST_TEXTURE_UNKNOWN_SIZE);

        Texture& texture = *texture_info->payloadT();

        texture.destroyHandle();
        texture.setHandle(
         gfx::createTexturePNG(
          texture.gfxDevice(),
          create_params,
          k_SamplerNearestRepeat,
          change_event.texture.texture_bytes_png,
          change_event.texture.texture_bytes_png_size
          ));

        // texture_info->reload(*engine);
      }
    }
  }

  void AnimationSystem::onInit(Engine& engine)
  {
    const bfAnim2DCreateParams create_anim_ctx = {nullptr, &engine, &onSSChange};

    m_Anim2DCtx = bfAnimation2D_new(&create_anim_ctx);
  }

  void AnimationSystem::onFrameUpdate(Engine& engine, float dt)
  {
    const auto scene = engine.currentScene();

    bfAnimation2D_beginFrame(m_Anim2DCtx);
    bfAnimation2D_stepFrame(m_Anim2DCtx, dt);

    if (scene)
    {
      auto& anim_sprites = scene->components<SpriteAnimator>();

      // Create an sprite handles needed.

      for (auto& anim_sprite : anim_sprites)
      {
        SpriteRenderer* const sprite = anim_sprite.owner().get<SpriteRenderer>();

        if (anim_sprite.m_Spritesheet && sprite)
        {
          bfAnim2DSpriteState state;

          if (bfAnim2DSprite_grabState(anim_sprite.m_SpriteHandle, &state))
          {
            sprite->uvRect() = {
             state.uv_rect.x,
             state.uv_rect.y,
             state.uv_rect.width,
             state.uv_rect.height,
            };
          }
        }
      }
    }
  }

  void AnimationSystem::onDeinit(Engine& engine)
  {
    bfAnimation2D_delete(m_Anim2DCtx);
  }
}  // namespace bf
