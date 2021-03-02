#ifndef BF_ANIM2D_H
#define BF_ANIM2D_H

#include "bf_anim2D_export.h" /* BF_ANIM2D_API, BF_CDECL */

#include <stddef.h> /* size_t            */
#include <stdint.h> /* uint32_t, uint8_t */

#if __cplusplus
extern "C" {
#endif
/* clang-format off */

#define k_bfAnim2DVersion   0                               /*!< Current version of the binary format this version of the code expects. */
#define k_bfSRSMServerPort  ((int16_t)4512)                 /*!< Port used on localhost to connect to the animator tool's server.       */
#define k_bfAnim2DInvalidID ((bfAnim2DAnimationID)(0xFFFF)) /*!< */

/* clang-format on */

typedef struct bfAnim2DCtx   bfAnim2DCtx;   /*!< Only one of these should need to be created per application, but having more than one is allowed. */
typedef struct bfSpritesheet bfSpritesheet; /*!< */
typedef struct bfAnimation   bfAnimation;   /*!< */

typedef uint8_t  bfBool8;
typedef uint32_t bfBool32;
typedef uint16_t bfAnim2DAnimationID; /*!< Indexes into `bfSpritesheet::animations`. */

typedef enum bfAnim2DChangeEventType
{
  bfAnim2DChange_Texture,
  bfAnim2DChange_Animation,  //!< An animation may have been, added, edited, removed or renamed.

} bfAnim2DChangeEventType;

typedef struct
{
  bfAnim2DChangeEventType type;        /*!< The type of change event. */
  bfSpritesheet*          spritesheet; /*!< The changed spritesheet.  */

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
typedef void(BF_CDECL* bfAnim2DSpritesheetChangedFn)(bfAnim2DCtx* ctx, bfSpritesheet* spritesheet, bfAnim2DChangeEvent change_event); /*!< Called whenever a spritesheet has changed from SRSM.     */

typedef struct
{
  float x;      /*!< The left side of the rectangle. */
  float y;      /*!< The top of the rectangle.       */
  float width;  /*!< The horizontal extent.          */
  float height; /*!< The vertical extent.            */

} bfUVRect; /*!< The UVs calculations assume 0,0 is the top left of the texture and 1,1 is the bottom right. */

typedef struct
{
  uint32_t frame_index; /*!< The index into the list of UVs that this frame corresponds to. */
  float    frame_time;  /*!< How long the frame will be on screen in seconds.               */

} bfAnimationFrame; /*!< */

typedef struct
{
  char*    str;     /*!< Nul terminated string pointer.*/
  uint64_t str_len; /*!< The cached length of the str.  */

} bfOwnedString; /*!< Used to indicated the string is owned by the struct that contains a member of this type. */

typedef struct
{
  const char* str;     /*!< Start of the span (may not be NUL terminated). */
  uint64_t    str_len; /*!< The cached length of the str.                  */

} bfStringSpan; /*!< A view into a constant string. */

typedef struct
{
  bfAnim2DAllocator allocator; /*!< NULL is valid, will just use the CRT's realloc and free. */
  void*             user_data; /*!< This userdata will be passed into the allocator.         */

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

// Input          - read but untouched by `bfAnim2D_stepFrame`.
// Output         - are written to and can be left uninitialized.
// Input / Output - read then written to.
typedef struct bfAnim2DUpdateInfo
{
  float               playback_speed;            //!< Input: 1.0 is normal speed and negative numbers means the animation will play backwards.
  float               time_left_for_frame;       //!< Input / Output: Time left for the current animation.
  uint16_t            spritesheet_idx;           //!< Input: The spritesheet this sprite is associated with in the 'spritesheets' array.
  bfAnim2DAnimationID animation;                 //!< Input: The animation to be used.
  uint16_t            current_frame : 14;        //!< Input / Output: The current frame of the animation.
  uint16_t            is_looping : 1;            //!< Input: Whether or not the sprite's current frame wraps around.
  uint16_t            has_finished_playing : 1;  //!< Output: True if the sprite reached the last frame of the animation this frame.

} bfAnim2DUpdateInfo;

BF_ANIM2D_API bfAnim2DCtx* bfAnim2D_new(const bfAnim2DCreateParams* params);
BF_ANIM2D_API void*        bfAnim2D_userData(const bfAnim2DCtx* self);
BF_ANIM2D_API void         bfAnim2D_networkUpdate(bfAnim2DCtx* self, bfAnim2DSpritesheetChangedFn callback);
BF_ANIM2D_API void         bfAnim2D_stepFrame(
         bfAnim2DUpdateInfo*   sprites,
         const bfSpritesheet** spritesheets,
         uint16_t              num_sprites,
         float                 delta_time);
BF_ANIM2D_API bfSpritesheet* bfAnim2D_loadSpritesheet(bfAnim2DCtx* self, bfStringSpan name, const uint8_t* srsm_bytes, size_t num_srsm_bytes);
BF_ANIM2D_API void           bfAnim2D_destroySpritesheet(bfAnim2DCtx* self, bfSpritesheet* spritesheet);
BF_ANIM2D_API void           bfAnim2D_delete(bfAnim2DCtx* self);

#if __cplusplus
}
#endif

#endif /* BF_ANIM2D_H */
