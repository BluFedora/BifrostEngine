#ifndef BIFROST_CAMERA_H
#define BIFROST_CAMERA_H

#include "bifrost_mat4x4.h"      /* Mat4x4       */
#include "bifrost_math_export.h" /* BF_MATH_API  */
#include "bifrost_vec2.h"        /* Vec2i        */
#include "bifrost_vec3.h"        /* Vec3f, Rectf */

#ifdef __cplusplus
extern "C" {
#endif

/* Frustum API */

//
// Points on the plane satisfy Dot(point, Vec3f{nx, ny, nz, 0.0f}) == d
//
// Equation of a Plane: Ax + By + Cz = D => Ax + By + Cz - D = 0
//

typedef struct
{
  float nx, ny, nz;  //!< Plane normal
  float d;           //!< Distance from the origin.

} bfPlane;

inline bfPlane bfPlane_fromPoints(Vec3f p0, Vec3f p1, Vec3f p2)
{
  bfPlane result;

  Vec3f p1_to_p0 = p1;
  Vec3f_sub(&p1_to_p0, &p0);

  Vec3f p2_to_p1 = p2;
  Vec3f_sub(&p2_to_p1, &p1);

  Vec3f normal;
  Vec3f_cross(&p1_to_p0, &p2_to_p1, &normal);

  result.nx = normal.x;
  result.ny = normal.y;
  result.nz = normal.z;
  result.d  = Vec3f_dot(&normal, &p0);

  return result;
}

inline Vec3f bfPlane_normal(bfPlane plane)
{
  const Vec3f result = {plane.nx, plane.ny, plane.nz, 0.0f};

  return result;
}

inline float bfPlane_dot(bfPlane plane, Vec3f point)
{
  const Vec3f normal = bfPlane_normal(plane);

  return bfV3f_dot(normal, point) + plane.d;
}

enum
{
  k_bfPlaneIdx_Near,
  k_bfPlaneIdx_Far,
  k_bfPlaneIdx_Left,
  k_bfPlaneIdx_Right,
  k_bfPlaneIdx_Top,
  k_bfPlaneIdx_Bottom,
  k_bfPlaneIdx_Max,
};

typedef enum
{
  BF_FRUSTUM_TEST_OUTSIDE      = 0,  //!< This is a falsy value to allow for this to be treated as a boolean.
  BF_FRUSTUM_TEST_INTERSECTING = 1,  //!< Partially inside of one of the planes
  BF_FRUSTUM_TEST_INSIDE       = 2,  //!< Completely inside all of the planes.

} bfFrustumTestResult;

typedef struct
{
  bfPlane planes[k_bfPlaneIdx_Max];

} bfFrustum;

BF_MATH_API void                bfFrustum_fromMatrix(bfFrustum* self, const Mat4x4* matrix);
BF_MATH_API bfFrustumTestResult bfFrustum_isPointInside(const bfFrustum* self, Vec3f point);
BF_MATH_API bfFrustumTestResult bfFrustum_isSphereInside(const bfFrustum* self, Vec3f center, float radius);
BF_MATH_API bfFrustumTestResult bfFrustum_isAABBInside(const bfFrustum* self, Vec3f aabb_min, Vec3f aabb_max);

/* Camera API */

typedef enum
{
  BIFROST_CAMERA_MODE_ORTHOGRAPHIC,
  BIFROST_CAMERA_MODE_FRUSTRUM,
  BIFROST_CAMERA_MODE_PRESPECTIVE,
  BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY

} CameraMode;

typedef struct
{
  CameraMode mode;

  union
  {
    // NOTE(SR):
    //   Used by:
    //     BIFROST_CAMERA_MODE_ORTHOGRAPHIC
    //     BIFROST_CAMERA_MODE_FRUSTRUM
    //   Units = Arbitrary World Space Units
    Rectf orthographic_bounds;

    struct
    {
      // NOTE(SR):
      //   Used by:
      //     BIFROST_CAMERA_MODE_PRESPECTIVE
      //     BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
      //   Units = Degrees
      float field_of_view_y;

      // NOTE(SR):
      //   Used by:
      //     BIFROST_CAMERA_MODE_PRESPECTIVE
      //     BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
      //   Units = Its just a ratio of width / height
      float aspect_ratio;
    };
  };

  // NOTE(SR):
  //   Units = Arbitrary World Space Units
  float near_plane;
  // NOTE(SR):
  //   Units = Arbitrary World Space Units
  //   Ignored by:
  //     BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY
  float far_plane;

} CameraModeParams;

typedef struct BifrostCamera_t
{
  bfFrustum        frustum;
  Vec3f            position;
  Vec3f            forward;   // This is normalized.
  Vec3f            up;        // This is normalized.
  Vec3f            _worldUp;  // This is normalized.
  Vec3f            _right;    // This is normalized.
  float            _yaw;      // Radians
  float            _pitch;    // Radians
  CameraModeParams camera_mode;
  Mat4x4           proj_cache;
  Mat4x4           view_cache;
  Mat4x4           view_proj_cache;
  Mat4x4           inv_proj_cache;       // The inverse cached for 3D picking.
  Mat4x4           inv_view_cache;       // The inverse cached for 3D picking.
  Mat4x4           inv_view_proj_cache;  //
  int              needs_update[2];      //   [0] - for proj_cache.
                                         //   [1] - for view_cache.

} BifrostCamera;

BF_MATH_API void  Camera_init(BifrostCamera* cam, const Vec3f* pos, const Vec3f* world_up, float yaw, float pitch);
BF_MATH_API void  Camera_update(BifrostCamera* cam);
BF_MATH_API void  bfCamera_openGLProjection(const BifrostCamera* cam, Mat4x4* out_projection);
BF_MATH_API void  Camera_move(BifrostCamera* cam, const Vec3f* dir, float amt);
BF_MATH_API void  Camera_moveLeft(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_moveRight(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_moveUp(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_moveDown(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_moveForward(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_moveBackward(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_addPitch(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_addYaw(BifrostCamera* cam, float amt);
BF_MATH_API void  Camera_mouse(BifrostCamera* cam, float offsetx, float offsety);
BF_MATH_API void  Camera_setFovY(BifrostCamera* cam, float value);
BF_MATH_API void  Camera_onResize(BifrostCamera* cam, uint width, uint height);
BF_MATH_API void  Camera_setProjectionModified(BifrostCamera* cam);
BF_MATH_API void  Camera_setViewModified(BifrostCamera* cam);
BF_MATH_API Vec3f Camera_castRay(BifrostCamera* cam, Vec2i screen_space, Vec2i screen_size);

// 'New' API

BF_MATH_API void bfCamera_setPosition(BifrostCamera* cam, const Vec3f* pos);

/* Ray API */

typedef struct
{
  Vec3f origin;              /* Required                   */
  Vec3f direction;           /* Required                   */
  Vec3f inv_direction;       /* Derived From direction     */
  int   inv_direction_signs; /* Derived From inv_direction */

} bfRay3D;

typedef struct
{
  int   did_hit;  /* Check this to se if the ray hit anything.      */
  float min_time; /* Set To An Undefined Value if did_hit is false. */
  float max_time; /* Set To An Undefined Value if did_hit is false. */

} bfRayCastResult;

BF_MATH_API bfRay3D         bfRay3D_make(Vec3f origin, Vec3f direction);
BF_MATH_API bfRayCastResult bfRay3D_intersectsAABB(const bfRay3D* ray, Vec3f aabb_min, Vec3f aabb_max);

#ifdef __cplusplus
}
#endif

#endif /* BIFROST_CAMERA_H */
