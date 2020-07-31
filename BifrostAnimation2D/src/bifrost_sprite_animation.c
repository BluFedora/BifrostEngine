#include "bifrost_sprite_animation_api.h"

#include <stdlib.h> /* reaalloc, free, NULL */

#include <stdio.h>

static void* bfDefaultAllocator(void* ptr, size_t old_size, size_t new_size, void* user_data)
{
  (void)user_data;
  (void)old_size;

  /*
    NOTE(Shareef):
      "if new_size is zero, the behavior is implementation defined
      (null pointer may be returned in which case the old memory block may or may not be freed),
      or some non-null pointer may be returned that may
      not be used to access storage."
  */
  if (new_size == 0u)
  {
    free(ptr);
    ptr = NULL;
  }
  else
  {
    void* const new_ptr = realloc(ptr, new_size);

    if (!new_ptr)
    {
      /*
        NOTE(Shareef):
          As to not leak memory, realloc says:
            "If there is not enough memory, the old memory block is not freed and null pointer is returned."
      */
      free(ptr);
    }

    ptr = new_ptr;
  }

  return ptr;
}

void bfAnimationScene_advance(BifrostAnimationScene* self, uint32_t offset, uint32_t num_sprites, float delta_time)
{
#define BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_IS_PLAYING 1
#define BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_LOOPING 1

  BifrostAnimatedSprite* const sprites_bgn = self->sprites + offset;
  BifrostAnimatedSprite* const sprites_end = sprites_bgn + num_sprites;

  BifrostAnimatedSprite* sprite = sprites_bgn;

  while (sprite != sprites_end)
  {
#if !BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_IS_PLAYING
    if (!(sprite->flags & k_bfAnimSpriteFlagIsPlaying))
    {
      ++sprite;
      continue;
    }
#endif

    const float playback_speed       = sprite->playback_speed;
    const int   playback_is_positive = playback_speed >= 0.0f;

#if BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_IS_PLAYING

    /*
       NOTE(Shareef):
         This works because 'k_bfAnimSpriteFlagIsPlaying' is defined as 1,
         so the only options are 1 or 0.
    */

    sprite->time_left_for_frame -= (float)(sprite->flags & k_bfAnimSpriteFlagIsPlaying) * delta_time * (playback_is_positive ? playback_speed : -playback_speed);
#else
    sprite->time_left_for_frame -= delta_time * (playback_is_positive ? playback_speed : -playback_speed);
#endif

    if (sprite->time_left_for_frame <= 0.0f)
    {
      const int32_t num_frames_minus_one = sprite->current_animation->num_frames - 1;
      const int32_t last_frame           = playback_is_positive ? num_frames_minus_one : 0;

      if (sprite->current_frame == last_frame)
      {
#if !BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_LOOPING
        if (sprite->flags & k_bfAnimSpriteFlagIsLooping)
#endif
        {
          if (sprite->flags & k_bfAnimSpriteFlagIsPingPong)
          {
            sprite->playback_speed *= -1.0f;
          }
          else
          {
            const int32_t first_frame = playback_is_positive ? 0 : num_frames_minus_one;

#if BIFROST_SPRITE_OPTIMIZATION_BRANCHLESS_LOOPING
            sprite->current_frame = (sprite->flags & k_bfAnimSpriteFlagIsLooping) ? first_frame : sprite->current_frame;
#else
            sprite->current_frame = first_frame;
#endif
          }
        }

        sprite->flags |= k_bfAnimSpriteFlagIsDone;
      }
      else
      {
        const int32_t delta_frame = playback_is_positive ? 1 : -1;

        sprite->current_frame += delta_frame;

        sprite->flags &= ~k_bfAnimSpriteFlagIsDone;
      }
    }

    ++sprite;
  }
}

void bfAnimation2D_ctor(BifrostAnimation2DCtx* self, const bfAnimation2DCreateParams* params)
{
  self->params = *params;

  if (!self->params.allocator)
  {
    self->params.allocator = bfDefaultAllocator;
  }

  self->scene_list = NULL;

  printf("Initializing the animation context\n");
  fflush(stdout);
}

BifrostAnimationScene* bfAnimation2D_createScene(BifrostAnimation2DCtx* self)
{
  BifrostAnimationScene* const scene = self->params.allocator(NULL, 0u, sizeof(BifrostAnimationScene), self->params.user_data);

  if (scene)
  {

  }

  return scene;
}

void bfAnimation2D_dtor(BifrostAnimation2DCtx* self)
{
}
