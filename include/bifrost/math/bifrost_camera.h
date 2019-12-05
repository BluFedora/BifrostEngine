#ifndef camera_h
#define camera_h

#include "bifrost_mat4x4.h" /* Mat4x4       */
#include "bifrost_vec3.h"   /* Vec3f, Rectf */
#include "bifrost_vec2.h"   /* Vec2i        */

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
          //   BIFROST_CAMERA_MODE_PRESPECTIVE
          //   BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
          //   Units = Degrees
        float field_of_view_y;

          // NOTE(Shareef):
          //   Used by:
          //   BIFROST_CAMERA_MODE_PRESPECTIVE
          //   BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
          //   Units = Its just a ratio of width / height
        float aspect_ratio;
      };
    };

      // NOTE(Shareef): Units = Arbitrary World Space Units
    float near_plane;
      // NOTE(Shareef):
      //   Ignored by:
      //     BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
    float far_plane;

  } CameraModeParams;

  typedef struct Camera_t
  {
    Vec3f             position;
    Vec3f             front;
    Vec3f             up;
    Vec3f             _worldUp;
    Vec3f             _right;
    float             _yaw;           // NOTE(Shareef): Radians
    float             _pitch;         // NOTE(Shareef): Radians
    CameraModeParams  camera_mode;
    Mat4x4            proj_cache;
    Mat4x4            view_cache;
    Mat4x4            inv_proj_cache; // NOTE(Shareef): The inverse cached for 3D picking.
    Mat4x4            inv_view_cache; // NOTE(Shareef): The inverse cached for 3D picking.
      // NOTE(Shareef):
      //   [0] - for proj_cache.
      //   [1] - for view_cache.
    int               needs_update[2];

  } Camera;

  void  Camera_init(Camera* cam, const Vec3f* const pos, const Vec3f* const world_up, float yaw, float pitch);
  void  Camera_update(Camera* cam);
  void  Camera_move(Camera* cam, const Vec3f * dir, float amt);
  void  Camera_moveLeft(Camera* cam, float amt);
  void  Camera_moveRight(Camera* cam, float amt);
  void  Camera_moveUp(Camera* cam, float amt);
  void  Camera_moveDown(Camera* cam, float amt);
  void  Camera_moveForward(Camera* cam, float amt);
  void  Camera_moveBackward(Camera* cam, float amt);
  void  Camera_addPitch(Camera* cam, float amt);
  void  Camera_addYaw(Camera* cam, float amt);
  void  Camera_mouse(Camera* cam, float offsetx, float offsety);

  void  Camera_setFovY(Camera* cam, const float value);
  void  Camera_onResize(Camera* cam, uint width, uint height);
  void  Camera_setProjectionModified(Camera* cam);
  Vec3f Camera_castRay(Camera* cam, Vec2i screen_space, Vec2i screen_size);
#ifdef __cplusplus
}
#endif

#endif /* camera_h */
