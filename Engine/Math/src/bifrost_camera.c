#include "bf/math/bifrost_camera.h"

#include <math.h> /* cosf, sinf */

/* Camera API */

static const Vec3f k_DefaultPosition = {0.0f, 0.0f, 0.0f, 1.0f};
static const Vec3f k_DefaultWorldUp  = {0.0f, 1.0f, 0.0f, 0.0f};

static void camera_update_vectors(BifrostCamera* cam)
{
  const float cosf_pitch = cosf(cam->_pitch);

  cam->forward.x = sinf(cam->_yaw) * cosf_pitch;
  cam->forward.y = sinf(cam->_pitch);
  cam->forward.z = -cosf(cam->_yaw) * cosf_pitch;
  cam->forward.w = 0.0f;

  Vec3f_normalize(&cam->forward);

  Vec3f_cross(&cam->forward, &cam->_worldUp, &cam->_right);
  Vec3f_normalize(&cam->_right);

  Vec3f_cross(&cam->_right, &cam->forward, &cam->up);
  Vec3f_normalize(&cam->up);

  cam->needs_update[1] = 1;
}

void Camera_init(BifrostCamera* cam, const Vec3f* pos, const Vec3f* world_up, float yaw, float pitch)
{
  if (!pos)
  {
    pos = &k_DefaultPosition;
  }

  if (!world_up)
  {
    world_up = &k_DefaultWorldUp;
  }

  cam->position                    = *pos;
  cam->position.w                  = 1.0f;
  cam->_worldUp                    = *world_up;
  cam->_worldUp.w                  = 0.0f;
  cam->_yaw                        = yaw;
  cam->_pitch                      = pitch;
  cam->camera_mode.mode            = BIFROST_CAMERA_MODE_PRESPECTIVE;
  cam->camera_mode.field_of_view_y = 60.0f;
  cam->camera_mode.aspect_ratio    = 16.0f / 9.0f;
  cam->camera_mode.near_plane      = 0.2f;
  cam->camera_mode.far_plane       = 1000.0f;
  Mat4x4_identity(&cam->proj_cache);
  Mat4x4_identity(&cam->view_cache);
  cam->needs_update[0] = 1;
  cam->needs_update[1] = 1;

  camera_update_vectors(cam);
}

void Camera_update(BifrostCamera* cam)
{
  int needed_update = 0;

  if (cam->needs_update[0])
  {
    switch (cam->camera_mode.mode)
    {
      case BIFROST_CAMERA_MODE_ORTHOGRAPHIC:
      {
        Mat4x4_orthoVk(&cam->proj_cache,
                       cam->camera_mode.orthographic_bounds.min[0],
                       cam->camera_mode.orthographic_bounds.max[0],
                       cam->camera_mode.orthographic_bounds.max[1],
                       cam->camera_mode.orthographic_bounds.min[1],
                       cam->camera_mode.near_plane,
                       cam->camera_mode.far_plane);
        break;
      }
      case BIFROST_CAMERA_MODE_FRUSTRUM:
      {
        Mat4x4_frustum(&cam->proj_cache,
                       cam->camera_mode.orthographic_bounds.min[0],
                       cam->camera_mode.orthographic_bounds.max[0],
                       cam->camera_mode.orthographic_bounds.max[1],
                       cam->camera_mode.orthographic_bounds.min[1],
                       cam->camera_mode.near_plane,
                       cam->camera_mode.far_plane);
        break;
      }
      case BIFROST_CAMERA_MODE_PRESPECTIVE:
      {
        Mat4x4_perspectiveVk(&cam->proj_cache,
                             cam->camera_mode.field_of_view_y,
                             cam->camera_mode.aspect_ratio,
                             cam->camera_mode.near_plane,
                             cam->camera_mode.far_plane);
        break;
      }
      case BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY:
      {
        Mat4x4_perspectiveInfinity(&cam->proj_cache,
                                   cam->camera_mode.field_of_view_y,
                                   cam->camera_mode.aspect_ratio,
                                   cam->camera_mode.near_plane);
        break;
      }
    }

    Mat4x4_inverse(&cam->proj_cache, &cam->inv_proj_cache);
    cam->needs_update[0] = 0;
    needed_update        = 1;
  }

  if (cam->needs_update[1])
  {
    Vec3f target = cam->position;
    Vec3f_add(&target, &cam->forward);

    Mat4x4_initLookAt(&cam->view_cache, &cam->position, &target, &cam->up);

    Mat4x4_inverse(&cam->view_cache, &cam->inv_view_cache);
    cam->needs_update[1] = 0;
    needed_update        = 1;
  }

  if (needed_update)
  {
    Mat4x4_mult(&cam->proj_cache, &cam->view_cache, &cam->view_proj_cache);
    Mat4x4_inverse(&cam->view_proj_cache, &cam->inv_view_proj_cache);
    bfFrustum_fromMatrix(&cam->frustum, &cam->view_proj_cache);
  }
}

void bfCamera_openGLProjection(const BifrostCamera* cam, Mat4x4* out_projection)
{
  switch (cam->camera_mode.mode)
  {
    case BIFROST_CAMERA_MODE_ORTHOGRAPHIC:
    {
      Mat4x4_ortho(out_projection,
                   cam->camera_mode.orthographic_bounds.min[0],
                   cam->camera_mode.orthographic_bounds.max[0],
                   cam->camera_mode.orthographic_bounds.max[1],
                   cam->camera_mode.orthographic_bounds.min[1],
                   cam->camera_mode.near_plane,
                   cam->camera_mode.far_plane);
      break;
    }
    case BIFROST_CAMERA_MODE_FRUSTRUM:
    {
      Mat4x4_frustum(out_projection,
                     cam->camera_mode.orthographic_bounds.min[0],
                     cam->camera_mode.orthographic_bounds.max[0],
                     cam->camera_mode.orthographic_bounds.max[1],
                     cam->camera_mode.orthographic_bounds.min[1],
                     cam->camera_mode.near_plane,
                     cam->camera_mode.far_plane);
      break;
    }
    case BIFROST_CAMERA_MODE_PRESPECTIVE:
    {
      Mat4x4_perspective(out_projection,
                         cam->camera_mode.field_of_view_y,
                         cam->camera_mode.aspect_ratio,
                         cam->camera_mode.near_plane,
                         cam->camera_mode.far_plane);
      break;
    }
    case BIFROST_CAMERA_MODE_PRESPECTIVE_INFINITY:
    {
      Mat4x4_perspectiveInfinity(out_projection,
                                 cam->camera_mode.field_of_view_y,
                                 cam->camera_mode.aspect_ratio,
                                 cam->camera_mode.near_plane);
      break;
    }
  }
}

void Camera_move(BifrostCamera* cam, const Vec3f* dir, float amt)
{
  Vec3f_addScaled(&cam->position, dir, amt);
  Camera_setViewModified(cam);
}

void Camera_moveLeft(BifrostCamera* cam, float amt)
{
  Camera_moveRight(cam, -amt);
}

void Camera_moveRight(BifrostCamera* cam, float amt)
{
  Vec3f right;
  Vec3f_cross(&cam->forward, &cam->up, &right);
  Camera_move(cam, &right, amt);
}

void Camera_moveUp(BifrostCamera* cam, float amt)
{
  Camera_move(cam, &cam->up, amt);
}

void Camera_moveDown(BifrostCamera* cam, float amt)
{
  Camera_move(cam, &cam->up, -amt);
}

void Camera_moveForward(BifrostCamera* cam, float amt)
{
  Vec3f fwd = cam->forward;
  Vec3f_normalize(&fwd);
  Camera_move(cam, &fwd, amt);
}

void Camera_moveBackward(BifrostCamera* cam, float amt)
{
  Camera_moveForward(cam, -amt);
}

void Camera_addPitch(BifrostCamera* cam, float amt)
{
  cam->_pitch += amt;
  camera_update_vectors(cam);
}

void Camera_addYaw(BifrostCamera* cam, float amt)
{
  cam->_yaw += amt;
  camera_update_vectors(cam);
}

void Camera_mouse(BifrostCamera* cam, float offsetx, float offsety)
{
  static const int constrain_pitch = 1;

  cam->_yaw += offsetx;
  cam->_pitch += offsety;

  if (constrain_pitch)
  {
    // 1.55334rad = 89.0fdeg
    if (cam->_pitch > 1.55334f)
      cam->_pitch = 1.55334f;
    if (cam->_pitch < -1.55334f)
      cam->_pitch = -1.55334f;
  }

  camera_update_vectors(cam);
}

void Camera_setFovY(BifrostCamera* cam, float value)
{
  cam->camera_mode.field_of_view_y = value;
  Camera_setProjectionModified(cam);
}

void Camera_onResize(BifrostCamera* cam, uint width, uint height)
{
  cam->width                    = width;
  cam->height                   = height;
  cam->camera_mode.aspect_ratio = (float)width / (float)height;

  Camera_setProjectionModified(cam);
}

void Camera_setProjectionModified(BifrostCamera* cam)
{
  cam->needs_update[0] = 1;
}

void Camera_setViewModified(BifrostCamera* cam)
{
  cam->needs_update[1] = 1;
}

Vec3f Camera_castRay(BifrostCamera* cam, Vec2i screen_space, Vec2i screen_size)
{
  // References:
  //   [http://antongerdelan.net/opengl/raycasting.html]
  const float ray_ndc_x = 2.0f * (float)screen_space.x / (float)screen_size.x - 1.0f;
  const float ray_ndc_y = 1.0f - 2.0f * (float)screen_space.y / (float)screen_size.y;
  const Vec3f ray_clip  = {ray_ndc_x, ray_ndc_y, -1.0f, 1.0f};
  Vec3f       ray_eye;
  Vec3f       ray_world;

  Camera_update(cam);
  Mat4x4_multVec(&cam->inv_proj_cache, &ray_clip, &ray_eye);

  ray_eye.z = -1.0f;
  ray_eye.w = 0.0f;

  Mat4x4_multVec(&cam->inv_view_cache, &ray_eye, &ray_world);

  Vec3f_normalize(&ray_world);

  return ray_world;
}

void bfCamera_setPosition(bfCamera* cam, const Vec3f* pos)
{
  cam->position = *pos;
  Camera_setViewModified(cam);
}

Vec3f bfCamera_worldToScreenSpace(const bfCamera* cam, Vec3f pos)
{
  Vec3f clip_space_pos;
  Mat4x4_multVec(&cam->view_proj_cache, &pos, &clip_space_pos);

  return bfV3f_mulS(clip_space_pos, 1.0f / clip_space_pos.w);
}

/* Frustum API */

static bfPlane bfV4_toPlane(Vec4f value)
{
  bfPlane result;

  result.nx = value.x;
  result.ny = value.y;
  result.nz = value.z;
  result.d  = value.w;

  return result;
}

// Interesting Reads on Math Library Design:
//   [https://fgiesen.wordpress.com/2010/10/17/view-frustum-culling/]
//   [http://www.codersnotes.com/notes/maths-lib-2016/]
//   [http://www.reedbeta.com/blog/on-vector-math-libraries/]

void bfFrustum_fromMatrix(bfFrustum* self, const Mat4x4* matrix)
{
  // Method from "Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix"
  //  [https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf]
  //  [https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/]

  const Vec4f row0 = bfMat4x4_row(matrix, 0);
  const Vec4f row1 = bfMat4x4_row(matrix, 1);
  const Vec4f row2 = bfMat4x4_row(matrix, 2);
  const Vec4f row3 = bfMat4x4_row(matrix, 3);

  const Vec4f planes[k_bfPlaneIdx_Max] =
   {
    bfV4f_add(row3, row2),  // k_bfPlaneIdx_Near
    bfV4f_sub(row3, row2),  // k_bfPlaneIdx_Far
    bfV4f_add(row3, row0),  // k_bfPlaneIdx_Left
    bfV4f_sub(row3, row0),  // k_bfPlaneIdx_Right
    bfV4f_sub(row3, row1),  // k_bfPlaneIdx_Top
    bfV4f_add(row3, row1),  // k_bfPlaneIdx_Bottom
   };

  // Normalize the planes.
  for (int i = 0; i < k_bfPlaneIdx_Max; ++i)
  {
    Vec4f plane = bfV4f_div(planes[i], bfV3f_len(bfV4f_toV3f(planes[i])));

    plane.w *= -1.0f;

    self->planes[i] = bfV4_toPlane(plane);
  }
}

bfFrustumTestResult bfFrustum_isPointInside(const bfFrustum* self, Vec3f point)
{
  for (int i = 0; i < k_bfPlaneIdx_Max; ++i)
  {
    if (bfPlane_dot(self->planes[i], point) < 0.0f)
    {
      return BF_FRUSTUM_TEST_OUTSIDE;
    }
  }

  return BF_FRUSTUM_TEST_INSIDE;
}

bfFrustumTestResult bfFrustum_isSphereInside(const bfFrustum* self, Vec3f center, float radius)
{
  for (int i = 0; i < k_bfPlaneIdx_Max; ++i)
  {
    const float distance = bfPlane_dot(self->planes[i], center);

    if (distance < -radius)
    {
      return BF_FRUSTUM_TEST_OUTSIDE;
    }

    // The distance in in the range of [-radius, +radius]
    if (fabsf(distance) < radius)
    {
      return BF_FRUSTUM_TEST_INTERSECTING;
    }
  }

  return BF_FRUSTUM_TEST_INSIDE;
}

bfFrustumTestResult bfFrustum_isAABBInside(const bfFrustum* self, Vec3f aabb_min, Vec3f aabb_max)
{
  // References:
  //   [https://cgvr.cs.uni-bremen.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/index.html]

  bfFrustumTestResult result = BF_FRUSTUM_TEST_INSIDE;

  for (int i = 0; i < k_bfPlaneIdx_Max; ++i)
  {
    const Vec3f normal         = bfPlane_normal(self->planes[i]);
    Vec3f       positive_point = aabb_min;
    Vec3f       negative_point = aabb_max;

    if (normal.x >= 0.0f)
    {
      positive_point.x = aabb_max.x;
      negative_point.x = aabb_min.x;
    }

    if (normal.y >= 0.0f)
    {
      positive_point.y = aabb_max.y;
      negative_point.y = aabb_min.y;
    }

    if (normal.z >= 0.0f)
    {
      positive_point.z = aabb_max.z;
      negative_point.z = aabb_min.z;
    }

    const float distance_to_positive = bfPlane_dot(self->planes[i], positive_point);

    if (distance_to_positive < 0.0f)
    {
      result = BF_FRUSTUM_TEST_OUTSIDE;
      break;
    }

    const float negative_to_positive = bfPlane_dot(self->planes[i], negative_point);

    if (negative_to_positive < 0.0f)
    {
      result = BF_FRUSTUM_TEST_INTERSECTING;
    }
  }

  return result;
}

/* Ray API */

enum
{
  k_RayXSignBit = (1 << 0),
  k_RayYSignBit = (1 << 1),
  k_RayZSignBit = (1 << 2),
};

bfRay3D bfRay3D_make(Vec3f origin, Vec3f direction)
{
  Vec3f_normalize(&direction);

  bfRay3D self;
  self.origin          = origin;
  self.origin.w        = 1.0f;
  self.direction       = direction;
  self.direction.w     = 0.0f;
  self.inv_direction.x = 1.0f / self.direction.x;
  self.inv_direction.y = 1.0f / self.direction.y;
  self.inv_direction.z = 1.0f / self.direction.z;
  self.inv_direction.w = 0.0f;
  self.inv_direction_signs =
   k_RayXSignBit * (self.inv_direction.x < 0.0f) |
   k_RayYSignBit * (self.inv_direction.y < 0.0f) |
   k_RayZSignBit * (self.inv_direction.z < 0.0f);

  return self;
}

inline int bfRay3D_sign(const bfRay3D* ray, int bit)
{
  return (ray->inv_direction_signs & bit) != 0;
}

// [https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection]
bfRaycastAABBResult bfRay3D_intersectsAABB(const bfRay3D* ray, Vec3f aabb_min, Vec3f aabb_max)
{
  bfRaycastAABBResult result   = (bfRaycastAABBResult){0, 0.0f, 0.0f};
  const Vec3f         bounds[] = {aabb_min, aabb_max};
  const int           r_sign_x = bfRay3D_sign(ray, k_RayXSignBit);
  const int           r_sign_y = bfRay3D_sign(ray, k_RayYSignBit);
  const float         tymin    = (bounds[r_sign_y].y - ray->origin.y) * ray->inv_direction.y;
  const float         tymax    = (bounds[1 - r_sign_y].y - ray->origin.y) * ray->inv_direction.y;

  float tmin = (bounds[r_sign_x].x - ray->origin.x) * ray->inv_direction.x;
  float tmax = (bounds[1 - r_sign_x].x - ray->origin.x) * ray->inv_direction.x;

  if (tmin > tymax || tymin > tmax)
    return result;

  if (tymin > tmin)
    tmin = tymin;

  if (tymax < tmax)
    tmax = tymax;

  const int   r_sign_z = bfRay3D_sign(ray, k_RayZSignBit);
  const float tzmin    = (bounds[r_sign_z].z - ray->origin.z) * ray->inv_direction.z;
  const float tzmax    = (bounds[1 - r_sign_z].z - ray->origin.z) * ray->inv_direction.z;

  if (tmin > tzmax || tzmin > tmax)
    return result;

  result.did_hit  = 1;
  result.min_time = tzmin > tmin ? tzmin : tmin;
  result.max_time = tzmax < tmax ? tzmax : tmax;

  return result;
}

// ReSharper disable CppSomeObjectMembersMightNotBeInitialized

bfRaycastTriangleResult bfRay3D_intersectsTriangle(const bfRay3D* ray, Vec3f triangle_a, Vec3f triangle_b, Vec3f triangle_c)
{
  bfRaycastTriangleResult result;

  const Vec3f p               = ray->origin;
  const Vec3f p_to_q          = bfV3f_mulS(ray->direction, -1.0f);
  const Vec3f b_to_a          = bfV3f_sub(triangle_b, triangle_a);
  const Vec3f c_to_a          = bfV3f_sub(triangle_c, triangle_a);
  const Vec3f triangle_normal = bfV3f_cross(b_to_a, c_to_a);
  const float d               = bfV3f_dot(p_to_q, triangle_normal);

  if (d <= 0.0f)  // Segment does not point toward the triangle (either away or parallel)
  {
    result.did_hit = 0;
    return result;
  }

  const Vec3f ap = bfV3f_sub(p, triangle_a);
  const float t  = bfV3f_dot(ap, triangle_normal);

  if (t < 0.0f)
  {
    result.did_hit = 0;
    return result;
  }

  const Vec3f e = bfV3f_cross(p_to_q, ap);
  const float v = bfV3f_dot(c_to_a, e);

  if (v < 0.0f || v > d)
  {
    result.did_hit = 0;
    return result;
  }

  const float u = -bfV3f_dot(b_to_a, e);

  if (u < 0.0f || (u + v) > d)
  {
    result.did_hit = 0;
    return result;
  }

  const float inv_d = 1.0f / d;

  result.did_hit = 1;
  result.time    = t * inv_d;
  result.v       = v * inv_d;
  result.u       = u * inv_d;
  result.w       = 1.0f - (result.v + result.u);

  return result;
}

bfRaycastTriangleResult bfSegment3D_intersectsTriangle(Vec3f p, Vec3f q, Vec3f triangle_a, Vec3f triangle_b, Vec3f triangle_c)
{
  bfRaycastTriangleResult result;

  const Vec3f p_to_q          = bfV3f_sub(p, q);
  const Vec3f b_to_a          = bfV3f_sub(triangle_b, triangle_a);
  const Vec3f c_to_a          = bfV3f_sub(triangle_c, triangle_a);
  const Vec3f triangle_normal = bfV3f_cross(b_to_a, c_to_a);
  const float d               = bfV3f_dot(p_to_q, triangle_normal);

  if (d <= 0.0f)  // Segment does not point toward the triangle (either away or parallel)
  {
    result.did_hit = 0;
    return result;
  }
  // Ray intersection: T >= 0

  const Vec3f ap = bfV3f_sub(p, triangle_a);
  const float t  = bfV3f_dot(ap, triangle_normal);

  if (t < 0.0f)
  {
    result.did_hit = 0;
    return result;
  }

  // This part is special for segments since they can exit early due to limited range.
  if (t > d)
  {
    result.did_hit = 0;

    return result;
  }

  const Vec3f e = bfV3f_cross(p_to_q, ap);
  const float v = bfV3f_dot(c_to_a, e);

  if (v < 0.0f || v > d)
  {
    result.did_hit = 0;
    return result;
  }

  const float u = -bfV3f_dot(b_to_a, e);

  if (u < 0.0f || (u + v) > d)
  {
    result.did_hit = 0;
    return result;
  }

  const float inv_d = 1.0f / d;

  result.did_hit = 1;
  result.time    = t * inv_d;
  result.v       = v * inv_d;
  result.u       = u * inv_d;
  result.w       = 1.0f - (result.v + result.u);

  return result;
}

// ReSharper restore CppSomeObjectMembersMightNotBeInitialized

bfRaycastPlaneResult bfRay3D_intersectsPlane(const bfRay3D* ray, bfPlane plane)
{
  bfRaycastPlaneResult result;

  const Vec3f plane_normal = bfPlane_normal(plane);
  const float n_dot_a      = bfV3f_dot(plane_normal, ray->origin);
  const float n_dot_ba     = bfV3f_dot(plane_normal, ray->direction);
  const float t            = -(n_dot_a - plane.d) / n_dot_ba;

  result.time    = t;
  result.did_hit = t >= 0.0f;

  return result;
}

bfRaycastPlaneResult bfSegment3D_intersectsPlane(Vec3f p, Vec3f q, bfPlane plane)
{
  bfRay3D ray;
  ray.origin    = p;
  ray.direction = bfV3f_sub(q, p);

  bfRaycastPlaneResult result = bfRay3D_intersectsPlane(&ray, plane);

  // The Segment has a max length to be considered a hit.
  result.did_hit = result.did_hit && result.time <= 1.0f;

  return result;
}
