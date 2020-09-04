#ifndef BF_SPRITE_ANIMATION_H
#define BF_SPRITE_ANIMATION_H

#include "bf_animation2D_export.h" /* BF_ANIM2D_API, BF_CDECL */

#include <stddef.h> /* size_t            */
#include <stdint.h> /* uint32_t, uint8_t */

#if __cplusplus
extern "C" {
#endif
/* clang-format off */

#define k_bfAnim2DVersion               0                     /*!< */
#define k_bfSRSMServerPort              ((int16_t)4512)       /*!< */
#define k_bfAnimSpriteLastFrame         ((int32_t)0x7FFFFFFF) /*! Max signed int                           */
#define k_bfAnim2DInvalidSpriteHandleID ((uint16_t)65535u)    /*! Max amount of sprites allowed per scene. */

/* clang-format on */

typedef struct bfAnimation_t      bfAnimation;      /*!< */
typedef struct bfSpritesheet_t    bfSpritesheet;    /*!< */
typedef struct bfAnimation2DCtx_t bfAnimation2DCtx; /*!< Only one of these should need to be created per application, but having more than one is allowed. */
typedef struct bfAnim2DSprite_t   bfAnim2DSprite;   /*!< */
typedef struct bfAnim2DScene_t    bfAnim2DScene;    /*!< Using multiple scenes is not required but makes it very easy to batch remove sprites. */

typedef uint8_t bfBool8;

enum
{
  bfAnim2DChange_Texture,
  bfAnim2DChange_Animation,
};

typedef struct
{
  int            type;        /*!< bfAnim2DChange_Texture or bfAnim2DChange_Animation */
  bfSpritesheet* spritesheet; /*!< The changed spritesheet. */

  union
  {
    struct
    {
      const char* texture_bytes_png;
      uint32_t    texture_bytes_png_size;

    } texture;

    struct
    {
      char padd[sizeof(const char*) * 2];

    } animation;
  };

} bfAnim2DChangeEvent;

typedef void*(BF_CDECL* bfAnimation2DAllocator)(void* ptr, size_t old_size, size_t new_size, void* user_data);
typedef void(BF_CDECL* bfAnimation2DSpritesheetChangedFn)(bfAnimation2DCtx* ctx, bfSpritesheet* spritesheet, bfAnim2DChangeEvent change_event);

typedef struct
{
  float x;      /*!< The left side of the rectangle. */
  float y;      /*!< The top of the rectangle.       */
  float width;  /*!< The horizontal extent.          */
  float height; /*!< The vertical extent.            */

} bfUVRect; /*!< The UVs calculations assume 0,0 is the top left of the texture and 1,1 is the bottom right. */

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
  const char* str;     /*!< Start of the span (may not be NUL terminated). */
  size_t      str_len; /*!< The cached length of the str.                  */

} bfStringSpan; /*!< */

typedef struct
{
  bfAnim2DScene* scene;
  uint32_t       id;
  uint32_t       reserved; /*!< explicit padding for x64 */

} bfAnim2DSpriteHandle;

typedef struct
{
  bfAnimation2DAllocator            allocator;              /*!< NULL is valid, will just use the CRT's realloc and free. */
  void*                             user_data;              /*!< This userdata will be passed into the allocator.         */
  bfAnimation2DSpritesheetChangedFn on_spritesheet_changed; /*!< Called whenever a spritesheet has changed from SRSM.     */

} bfAnim2DCreateParams;

typedef struct
{
  bfStringSpan            name;       /*!< */
  const bfAnimationFrame* frames;     /*!< */
  uint32_t                num_frames; /*!< */

} bfAnimationCreateParams;

struct bfAnimation_t /*!<  */
{
  bfOwnedString     name;       /*!< */
  bfAnimationFrame* frames;     /*!< */
  int32_t           num_frames; /*!< */
};

typedef struct
{
  bfStringSpan                   name;           /*!< */
  const bfAnimationCreateParams* animation_data; /*!< */
  uint32_t                       num_animations; /*!< */
  const bfUVRect*                uv_data;        /*!< */
  uint32_t                       num_uvs;        /*!< */
  void*                          user_atlas;     /*!< */

} bfSpritesheetCreateParams;

struct bfSpritesheet_t /*!< */
{
  bfOwnedString name;           /*!< */
  bfAnimation*  animations;     /*!< Sorted array of animations.                           */
  bfUVRect*     uvs;            /*!< All the uvs for the frames.                           */
  uint32_t      num_animations; /*!< The number of elements in [bfSpriteSheet::animations] */
  uint32_t      num_uvs;        /*!< The number of elements in [bfSpriteSheet::uvs]        */
  void*         user_data;      /*!< */
};

typedef struct
{
  bfAnim2DScene* scene;
  uint32_t       offset;
  uint32_t       num_sprites;
  float          delta_time;

} bfAnim2DStepFrameExOptions;

BF_ANIM2D_API bfAnimation2DCtx* bfAnimation2D_new(const bfAnim2DCreateParams* params);
BF_ANIM2D_API void*             bfAnimation2D_userData(const bfAnimation2DCtx* self);
BF_ANIM2D_API bfAnim2DScene* bfAnimation2D_createScene(bfAnimation2DCtx* self);
BF_ANIM2D_API bfSpritesheet* bfAnimation2D_createSpritesheet(bfAnimation2DCtx* self, const bfSpritesheetCreateParams* params);
BF_ANIM2D_API bfSpritesheet* bfAnimation2D_loadSpritesheet(bfAnimation2DCtx* self, bfStringSpan name, const uint8_t* srsm_bytes, size_t num_srsm_bytes);
BF_ANIM2D_API void           bfAnimation2D_beginFrame(bfAnimation2DCtx* self);
BF_ANIM2D_API void           bfAnimation2D_stepFrameEx(const bfAnimation2DCtx* self, const bfAnim2DStepFrameExOptions* settings);
BF_ANIM2D_API void           bfAnimation2D_stepFrame(const bfAnimation2DCtx* self, float delta_time);
BF_ANIM2D_API void           bfAnimation2D_destroySpritesheet(bfAnimation2DCtx* self, bfSpritesheet* spritesheet);
BF_ANIM2D_API void           bfAnimation2D_destroyScene(bfAnimation2DCtx* self, bfAnim2DScene* scene);
BF_ANIM2D_API void           bfAnimation2D_delete(bfAnimation2DCtx* self);

BF_ANIM2D_API bfAnim2DSpriteHandle bfAnim2DScene_addSprite(bfAnim2DScene* self);
BF_ANIM2D_API void                 bfAnim2DScene_destroySprite(bfAnim2DScene* self, bfAnim2DSpriteHandle* sprite);

typedef struct
{
  const bfAnimation* animation;
  float              playback_speed;
  int32_t            start_frame;
  bfBool8            is_looping;
  bfBool8            does_ping_ponging;
  bfBool8            force_restart;

} bfAnim2DPlayExOptions;

typedef struct
{
  const bfAnimation* animation;
  bfUVRect           uv_rect;
  bfAnimationFrame   current_frame;
  float              time_left_for_frame;

} bfAnim2DSpriteState;

BF_ANIM2D_API bfAnim2DSpriteHandle bfAnim2DSprite_invalidHandle(void);
BF_ANIM2D_API bfBool8              bfAnim2DSprite_isInvalidHandle(const bfAnim2DSpriteHandle* handle);
BF_ANIM2D_API void                 bfAnim2DSprite_setSpritesheet(bfAnim2DSpriteHandle self, bfSpritesheet* sheet);
BF_ANIM2D_API void                 bfAnim2DSprite_playAnimationEx(bfAnim2DSpriteHandle self, const bfAnim2DPlayExOptions* options);
BF_ANIM2D_API void                 bfAnim2DSprite_pause(bfAnim2DSpriteHandle self);
BF_ANIM2D_API bfBool8              bfAnim2DSprite_grabState(bfAnim2DSpriteHandle self, bfAnim2DSpriteState* out_state);

#if __cplusplus
}
#endif

#endif /* BF_SPRITE_ANIMATION_H */
