#include "bifrost/math/bifrost_camera.h"

#include <math.h> /* cosf, sinf */

static void camera_update_vectors(Camera* cam)
{
  const float cosf_pitch = cosf(cam->_pitch);

  cam->front.x = sinf(cam->_yaw) * cosf_pitch;
  cam->front.y = sinf(cam->_pitch);
  cam->front.z = -cosf(cam->_yaw) * cosf_pitch;
  cam->front.w = 0.0f;

  Vec3f_normalize(&cam->front);

  Vec3f_cross(&cam->front, &cam->_worldUp, &cam->_right);
  Vec3f_normalize(&cam->_right);

  Vec3f_cross(&cam->_right, &cam->front, &cam->up);
  Vec3f_normalize(&cam->up);

  cam->needs_update[1] = 1;
}

void Camera_init(Camera* cam, const Vec3f* const pos, const Vec3f* const world_up, float yaw, float pitch)
{
  cam->position                    = (Vec3f){pos->x,       pos->y,       pos->z,       1.0f};
  cam->_worldUp                    = (Vec3f){world_up->x,  world_up->y,  world_up->z,  0.0f};
  cam->_yaw                        = yaw;
  cam->_pitch                      = pitch;
  cam->camera_mode.mode            = BIFROST_CAMERA_MODE_PRESPECTIVE;
  cam->camera_mode.field_of_view_y = 45.0f;
  cam->camera_mode.aspect_ratio    = 16.0f / 9.0f;
  cam->camera_mode.near_plane      = 0.2f;
  cam->camera_mode.far_plane       = 1000.0f;
  Mat4x4_identity(&cam->proj_cache);
  Mat4x4_identity(&cam->view_cache);
  cam->needs_update[0]             = 1;
  cam->needs_update[1]             = 1;

  camera_update_vectors(cam);
}

void Camera_update(Camera* cam)
{
  if (cam->needs_update[0])
  {
    switch (cam->camera_mode.mode)
    {
      case BIFROST_CAMERA_MODE_ORTHOGRAPHIC:
        {
          Mat4x4_ortho(&cam->proj_cache,
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
        Mat4x4_perspective(&cam->proj_cache,
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
  }

  if (cam->needs_update[1])
  {
    Vec3f target = cam->position;
    Vec3f_add(&target, &cam->front);

    Mat4x4_initLookAt(&cam->view_cache, &cam->position, &target, &cam->up);

    Mat4x4_inverse(&cam->view_cache, &cam->inv_view_cache);
    cam->needs_update[1] = 0;
  }
}

void Camera_move(Camera* cam, const Vec3f* dir, float amt)
{
  Vec3f_addScaled(&cam->position, dir, amt);
  cam->needs_update[1] = 1;
}

void Camera_moveLeft(Camera* cam, float amt)
{
  Vec3f right;
  Vec3f_cross(&cam->front, &cam->up, &right);
  Camera_move(cam, &right, -amt);
}

void Camera_moveRight(Camera* cam, float amt)
{
  Vec3f right;
  Vec3f_cross(&cam->front, &cam->up, &right);
  Camera_move(cam, &right, amt);
}

void Camera_moveUp(Camera* cam, float amt)
{
   Camera_move(cam, &cam->up, amt);
}

void Camera_moveDown(Camera* cam, float amt)
{
  Camera_move(cam, &cam->up, -amt);
}

void Camera_moveForward(Camera* cam, float amt)
{
  Vec3f fwd = cam->front;
  Vec3f_normalize(&fwd);
  Camera_move(cam, &fwd, amt);
}

void Camera_moveBackward(Camera* cam, float amt)
{
  Camera_moveForward(cam, -amt);
}

void Camera_addPitch(Camera* cam, float amt)
{
  cam->_pitch += amt;
  camera_update_vectors(cam);
}

void Camera_addYaw(Camera* cam, float amt)
{
  cam->_yaw += amt;
  camera_update_vectors(cam);
}

void Camera_mouse(Camera* cam, float offsetx, float offsety)
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

void Camera_setFovY(Camera* cam, const float value)
{
  cam->camera_mode.field_of_view_y = value;
  Camera_setProjectionModified(cam);
}

void Camera_onResize(Camera* cam, uint width, uint height)
{
  cam->camera_mode.aspect_ratio = (float)width / (float)height;
  Camera_setProjectionModified(cam);
}

void Camera_setProjectionModified(Camera* cam)
{
  cam->needs_update[0] = 1;
}

Vec3f Camera_castRay(Camera* cam, Vec2i screen_space, Vec2i screen_size)
{
  Camera_update(cam);

  Vec3f ray_eye, result;

  //const Vec2f ndc = {
  //  ((2.0f * screen_space.x) / screen_size.x)  - 1.0f,
  //  1.0f - ((2.0f * screen_space.y) / screen_size.y)
  //};

  const Vec3f ray_clip = {
   2.0f * (float)screen_space.x / (float)screen_size.x - 1.0f,
   1.0f - 2.0f * (float)screen_space.y / (float)screen_size.y,
    -1.0f,
    1.0f
  };

  Mat4x4_multVec(&cam->inv_proj_cache, &ray_clip, &ray_eye);

  ray_eye.z = -1.0f;
  ray_eye.w = 0.0f;
  Mat4x4_multVec(&cam->inv_view_cache, &ray_eye, &result);

  Vec3f_normalize(&result);

  return result;
}