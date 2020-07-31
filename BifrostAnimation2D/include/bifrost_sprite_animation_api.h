
#ifndef BIFROST_SPRITE_ANIMATION_H
#define BIFROST_SPRITE_ANIMATION_H

#include "bifrost_animation2D_export.h"

#include <stddef.h> /* size_t            */
#include <stdint.h> /* uint32_t, uint8_t */

#if __cplusplus
extern "C" {
#endif

struct bfSpritesheet_t;
typedef struct bfSpritesheet_t bfSpritesheet;

typedef struct
{
  float x;      /*!< The left side of the rectangle. */
  float y;      /*!< The top of the rectangle.       */
  float width;  /*!< The horizontal extent.          */
  float height; /*!< The vertical extent.            */

} bfUVRect; /*!< */

typedef struct
{
  uint32_t frame_index; /*!< */
  float    frame_time;  /*!< */

} bfAnimationFrame; /*!< */

typedef struct
{
  char*  str;     /*!< Nul terminated string pointer.*/
  size_t str_len; /*!< The cached length of the str.  */

} bfOwnedString; /*!< */

typedef struct
{
  const char* str;     /*!< Nul terminated string pointer.*/
  size_t      str_len; /*!< The cached length of the str.  */

} bfStringSpan; /*!< */

typedef struct
{
  bfSpritesheet*    spritesheet; /*!< */
  bfOwnedString     name;        /*!< */
  bfAnimationFrame* frames;      /*!< */
  int32_t           num_frames;  /*!< */

} bfAnimation; /*!< */

struct bfSpritesheet_t
{
  bfOwnedString name;           /*!< */
  bfAnimation*  animations;     /*!< Sorted array of animations.                           */
  bfUVRect*     uvs;            /*!< All the uvs for the frames.                           */
  uint32_t      num_animations; /*!< The number of elements in [bfSpriteSheet::animations] */
  uint32_t      num_uvs;        /*!< The number of elements in [bfSpriteSheet::uvs]        */
  void*         texture_data;

}; /*!< */

/* clang-format off */

#define k_bfAnim2DVersion            0

#define k_bfAnimSpriteFlagIsPlaying  ((uint8_t)(1u << 0u)) /*!< Must be defined as 1 for an optimization in the 'bfAnimationScene_advance'. */
#define k_bfAnimSpriteFlagIsLooping  ((uint8_t)(1u << 1u)) /*!< */
#define k_bfAnimSpriteFlagIsPingPong ((uint8_t)(1u << 2u)) /*!< Only matters if the 'k_bfAnimSpriteFlagIsLooping' flag is also set. */
#define k_bfAnimSpriteFlagIsDone     ((uint8_t)(1u << 3u)) /*!< */

#define k_bfAnimSpriteLastFrame      ((int32_t)0x7FFFFFFF) /*! Max signed int */

/* clang-format on */

typedef struct
{
  bfAnimation* current_animation;
  float        playback_speed;
  float        time_left_for_frame;
  int32_t      current_frame;
  uint8_t      flags;

} BifrostAnimatedSprite;

typedef uint32_t bfAnimatedSpriteHandle;

BIFROST_SPRITE_ANIM_API void bfAnimatedSprite_setEnabled(bfAnimatedSpriteHandle* self, int is_enabled);

typedef struct BifrostAnimationScene_t
{
  uint32_t                        num_active_sprites; /*!< The number of sprites that should be updated.                             */
  uint32_t                        num_sprites;        /*!< The number of sprites in the [BifrostSpriteAnimationCtx::sprites] array.  */
  uint32_t                        sprites_capacity;   /*!< The max number of elements [BifrostSpriteAnimationCtx::sprites] can hold. */
  BifrostAnimatedSprite*          sprites;            /*!< Array of all sprites [num_active_sprites | num_sprites]                   */
  struct BifrostAnimationScene_t* prev;               /*!< */
  struct BifrostAnimationScene_t* next;               /*!< */

} BifrostAnimationScene; /*!< Using multiple scenes is not required but makes it very easy to batch remove sprites. */

BIFROST_SPRITE_ANIM_API bfAnimatedSpriteHandle bfAnimationScene_createSprite(BifrostAnimationScene* self);
BIFROST_SPRITE_ANIM_API void                   bfAnimationScene_advance(BifrostAnimationScene* self, uint32_t offset, uint32_t num_sprites, float delta_time);
BIFROST_SPRITE_ANIM_API void                   bfAnimationScene_destroySprite(BifrostAnimationScene* self, bfAnimatedSpriteHandle sprite);

typedef void* (*bfAnimation2DAllocator)(void* ptr, size_t old_size, size_t new_size, void* user_data);

typedef struct
{
  bfAnimation2DAllocator allocator; /*!< NULL is valid, will just use the CRT's realloc and free. */
  void*                  user_data; /*!< This userdata will be passed into the allocator.         */

} bfAnimation2DCreateParams;

typedef struct
{
  bfAnimation2DCreateParams params;
  BifrostAnimationScene*    scene_list;
  BifrostAnimationScene     base_scene;

} BifrostAnimation2DCtx; /*!< Only one of these should need to be created per application, but having more than one is allowed. */

BIFROST_SPRITE_ANIM_API void bfAnimation2D_ctor(BifrostAnimation2DCtx* self, const bfAnimation2DCreateParams* params);
void                         bfAnimation2D_loadAtlas(BifrostAnimation2DCtx* self, const uint8_t* atlas_bytes, void* texture_id);
BIFROST_SPRITE_ANIM_API BifrostAnimationScene* bfAnimation2D_createScene(BifrostAnimation2DCtx* self);
BIFROST_SPRITE_ANIM_API void                   bfAnimation2D_watchSheet(BifrostAnimation2DCtx* self, const bfStringSpan absolute_path);
BIFROST_SPRITE_ANIM_API void                   bfAnimation2D_dtor(BifrostAnimation2DCtx* self);

#if __cplusplus
}
#endif

#endif /* BIFROST_SPRITE_ANIMATION_H */
