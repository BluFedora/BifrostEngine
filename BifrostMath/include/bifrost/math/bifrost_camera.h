#ifndef BIFROST_CAMERA_H
#define BIFROST_CAMERA_H

#include "bifrost_mat4x4.h"      /* Mat4x4       */
#include "bifrost_math_export.h" /* BIFROST_MATH_API */
#include "bifrost_vec2.h"        /* Vec2i        */
#include "bifrost_vec3.h"        /* Vec3f, Rectf */

#ifdef __cplusplus
extern "C" {
#endif
typedef enum CameraMode_t
{
  BIFROST_CAMERA_MODE_ORTHOGRAPHIC,
  BIFROST_CAMERA_MODE_FRUSTRUM,
  BIFROST_CAMERA_MODE_PRESPECTIVE,
  BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY

} CameraMode;

typedef struct CameraModeParams_t
{
  CameraMode mode;

  union
  {
    // NOTE(Shareef):
    //   Used by:
    //     BIFROST_CAMERA_MODE_ORTHOGRAPHIC
    //     BIFROST_CAMERA_MODE_FRUSTRUM
    //   Units = Arbitrary World Space Units
    Rectf orthographic_bounds;

    struct
    {
      // NOTE(Shareef):
      //   Used by:
      //     BIFROST_CAMERA_MODE_PRESPECTIVE
      //     BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
      //   Units = Degrees
      float field_of_view_y;

      // NOTE(Shareef):
      //   Used by:
      //     BIFROST_CAMERA_MODE_PRESPECTIVE
      //     BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
      //   Units = Its just a ratio of width / height
      float aspect_ratio;
    };
  };

  // NOTE(Shareef):
  //   Units = Arbitrary World Space Units
  float near_plane;
  // NOTE(Shareef):
  //   Ignored by:
  //     BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
  float far_plane;

} CameraModeParams;

typedef struct BifrostCamera_t
{
  Vec3f            position;
  Vec3f            forward;
  Vec3f            up;
  Vec3f            _worldUp;
  Vec3f            _right;
  float            _yaw;    // Radians
  float            _pitch;  // Radians
  CameraModeParams camera_mode;
  Mat4x4           proj_cache;
  Mat4x4           view_cache;
  Mat4x4           inv_proj_cache;       // The inverse cached for 3D picking.
  Mat4x4           inv_view_cache;       // The inverse cached for 3D picking.
  Mat4x4           inv_view_proj_cache;  //
  int              needs_update[2];      //   [0] - for proj_cache.
                                         //   [1] - for view_cache.

} BifrostCamera;

BIFROST_MATH_API void  Camera_init(BifrostCamera* cam, const Vec3f* pos, const Vec3f* world_up, float yaw, float pitch);
BIFROST_MATH_API void  Camera_update(BifrostCamera* cam);
BIFROST_MATH_API void  Camera_move(BifrostCamera* cam, const Vec3f* dir, float amt);
BIFROST_MATH_API void  Camera_moveLeft(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_moveRight(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_moveUp(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_moveDown(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_moveForward(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_moveBackward(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_addPitch(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_addYaw(BifrostCamera* cam, float amt);
BIFROST_MATH_API void  Camera_mouse(BifrostCamera* cam, float offsetx, float offsety);
BIFROST_MATH_API void  Camera_setFovY(BifrostCamera* cam, float value);
BIFROST_MATH_API void  Camera_onResize(BifrostCamera* cam, uint width, uint height);
BIFROST_MATH_API void  Camera_setProjectionModified(BifrostCamera* cam);
BIFROST_MATH_API void  Camera_setViewModified(BifrostCamera* cam);
BIFROST_MATH_API Vec3f Camera_castRay(BifrostCamera* cam, Vec2i screen_space, Vec2i screen_size);

#ifdef __cplusplus
}
#endif

#endif /* BIFROST_CAMERA_H */
