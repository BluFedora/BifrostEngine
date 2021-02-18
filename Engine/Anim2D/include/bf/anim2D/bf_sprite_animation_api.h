#ifndef BF_ANIM2D_H
#define BF_ANIM2D_H

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

/* clang-format on */

typedef struct bfAnim2DCtx   bfAnim2DCtx;   /*!< Only one of these should need to be created per application, but having more than one is allowed. */
typedef struct bfSpritesheet bfSpritesheet; /*!< */
typedef struct bfAnimation   bfAnimation;   /*!< */

typedef uint8_t  bfBool8;
typedef uint16_t bfAnim2DAnimationID;                       /*!< Indexes into `bfSpritesheet::animations`. */
#define k_bfAnim2DInvalidID ((bfAnim2DAnimationID)(0xFFFF)) /*!< */

typedef enum
{
  bfAnim2DChange_Texture,
  bfAnim2DChange_Animation,  //!< An animation may have been, added, edited, removed or renamed.

} bfAnim2DChangeEventType;

typedef struct
{
  bfAnim2DChangeEventType type;        /*!< bfAnim2DChange_Texture or bfAnim2DChange_Animation */
  bfSpritesheet*          spritesheet; /*!< The changed spritesheet. */

  union
  {
    struct
    {
      const char* texture_bytes_png;
      uint32_t    texture_bytes_png_size;

    } texture;
  };

} bfAnim2DChangeEvent;

typedef void*(BF_CDECL* bfAnim2DAllocator)(void* ptr, size_t old_size, size_t new_size, void* user_data);
typedef void(BF_CDECL* bfAnim2DSpritesheetChangedFn)(bfAnim2DCtx* ctx, bfSpritesheet* spritesheet, bfAnim2DChangeEvent change_event);

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
  bfAnim2DAllocator            allocator;              /*!< NULL is valid, will just use the CRT's realloc and free. */
  void*                        user_data;              /*!< This userdata will be passed into the allocator.         */
  bfAnim2DSpritesheetChangedFn on_spritesheet_changed; /*!< Called whenever a spritesheet has changed from SRSM.     */

} bfAnim2DCreateParams;

struct bfAnimation /*!<  */
{
  bfOwnedString     name;       /*!< */
  bfAnimationFrame* frames;     /*!< */
  uint32_t          num_frames; /*!< */
};

struct bfSpritesheet /*!< */
{
  bfOwnedString  name;           /*!< */
  bfAnimation*   animations;     /*!< Sorted array of animations.                           */
  bfUVRect*      uvs;            /*!< All the uvs for the frames.                           */
  uint32_t       num_animations; /*!< The number of elements in [bfSpriteSheet::animations] */
  uint32_t       num_uvs;        /*!< The number of elements in [bfSpriteSheet::uvs]        */
  void*          user_data;      /*!< */
  char           guid[37];       /*!< */
  bfSpritesheet* prev;           /*!< */
  bfSpritesheet* next;           /*!< */
};

typedef struct bfAnim2DUpdateInput
{
  float               playback_speed;
  float               time_left_for_frame;
  bfAnim2DAnimationID animation;
  uint16_t            spritesheet_idx;
  uint32_t            current_frame : 31;
  uint32_t            is_looping : 1;

} bfAnim2DUpdateInput;

typedef struct bfAnim2DUpdateOutput
{
  float    time_left_for_frame;
  uint32_t current_frame : 31;
  uint32_t has_finished_playing : 1;

} bfAnim2DUpdateOutput;

BF_ANIM2D_API bfAnim2DCtx* bfAnim2D_new(const bfAnim2DCreateParams* params);
BF_ANIM2D_API void*        bfAnim2D_userData(const bfAnim2DCtx* self);
BF_ANIM2D_API void         bfAnim2D_networkUpdate(bfAnim2DCtx* self);
BF_ANIM2D_API void         bfAnim2D_stepFrame(
         const bfAnim2DUpdateInput* input,
         const bfSpritesheet**      input_spritesheets,
         uint16_t                   num_input,
         float                      delta_time,
         bfAnim2DUpdateOutput*      output);
BF_ANIM2D_API bfSpritesheet* bfAnim2D_loadSpritesheet(bfAnim2DCtx* self, bfStringSpan name, const uint8_t* srsm_bytes, size_t num_srsm_bytes);
BF_ANIM2D_API void           bfAnim2D_destroySpritesheet(bfAnim2DCtx* self, bfSpritesheet* spritesheet);
BF_ANIM2D_API void           bfAnim2D_delete(bfAnim2DCtx* self);

#if __cplusplus
}
#endif

#endif /* BF_ANIM2D_H */
