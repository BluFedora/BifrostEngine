/*!
 * @file   bifrost_transform.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
#include "bf/math/bifrost_transform.h"

#include <assert.h>  // assert
#include <math.h>    // sqrt
#include <stddef.h>  // NULL

#define k_PI (3.14159265359f)
#define k_HalfPI (k_PI * 0.5f)
#define k_RadToDegf (57.2958f)
#define k_DegToRadf (0.01745329251f)
#define k_Epsilonf (0.00001f)

// Quaternion

// [https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles]

Quaternionf bfQuaternionf_init(float x, float y, float z, float w)
{
  const Quaternionf result = {{x, y, z, w}};

  return result;
}

Quaternionf bfQuaternionf_identity(void)
{
  return bfQuaternionf_init(0.0f, 0.0f, 0.0f, 1.0f);
}

Quaternionf bfQuaternionf_fromAxisAngleRad(const Vec3f* axis, float angle)
{
  const float rad_half_angle = angle * 0.5f;
  const float sin_half_angle = sinf(rad_half_angle);
  const float cos_half_angle = cosf(rad_half_angle);

  return bfQuaternionf_init(
   axis->x * sin_half_angle,
   axis->y * sin_half_angle,
   axis->z * sin_half_angle,
   cos_half_angle);
}

Quaternionf bfQuaternionf_fromAxisAngleDeg(const Vec3f* axis, float angle)
{
  return bfQuaternionf_fromAxisAngleRad(axis, angle * k_DegToRadf);
}

Quaternionf bfQuaternionf_fromMatrix(const Mat4x4* rot_mat)
{
  Quaternionf self;
  const float m00   = Mat4x4_at(rot_mat, 0, 0);
  const float m11   = Mat4x4_at(rot_mat, 1, 1);
  const float m22   = Mat4x4_at(rot_mat, 2, 2);
  const float m21   = Mat4x4_at(rot_mat, 2, 1);
  const float m10   = Mat4x4_at(rot_mat, 1, 0);
  const float m12   = Mat4x4_at(rot_mat, 1, 2);
  const float m01   = Mat4x4_at(rot_mat, 0, 1);
  const float m02   = Mat4x4_at(rot_mat, 0, 2);
  const float m20   = Mat4x4_at(rot_mat, 2, 0);
  const float trace = m00 + m11 + m22;

  if (trace > 0.0f)
  {
    const float s = 0.5f / sqrtf(trace + 1.0f);

    self.w = 0.25f / s;
    self.x = (Mat4x4_at(rot_mat, 2, 1) - Mat4x4_at(rot_mat, 1, 2)) * s;
    self.y = (Mat4x4_at(rot_mat, 0, 2) - Mat4x4_at(rot_mat, 2, 0)) * s;
    self.z = (Mat4x4_at(rot_mat, 1, 0) - Mat4x4_at(rot_mat, 0, 1)) * s;
  }
  else if (m00 > m11 && m00 > m22)
  {
    const float s = 2.0f * sqrtf(1.0f + m00 - m11 - m22);

    self.w = (m21 - m12) / s;
    self.x = 0.25f * s;
    self.y = (m01 + m10) / s;
    self.z = (m02 + m20) / s;
  }
  else if (m11 > m22)
  {
    const float s = 2.0f * sqrtf(1.0f + m11 - m00 - m22);

    self.w = (m02 - m20) / s;
    self.x = (m01 + m10) / s;
    self.y = 0.25f * s;
    self.z = (m12 + m21) / s;
  }
  else
  {
    const float s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);

    self.w = (m10 - m01) / s;
    self.x = (m02 + m20) / s;
    self.y = (m21 + m12) / s;
    self.z = 0.25f * s;
  }

  bfQuaternionf_normalize(&self);
  return self;
}

Quaternionf bfQuaternionf_fromEulerDeg(float pitch, float yaw, float roll)
{
  return bfQuaternionf_fromEulerRad(pitch * k_DegToRadf, yaw * k_DegToRadf, roll * k_DegToRadf);
}

Quaternionf bfQuaternionf_fromEulerRad(float pitch, float yaw, float roll)
{
  const float half_pitch = pitch * 0.5f;
  const float half_yaw   = yaw * 0.5f;
  const float half_roll  = roll * 0.5f;
  const float x_axis_cos = cosf(half_pitch);
  const float x_axis_sin = sinf(half_pitch);
  const float y_axis_cos = cosf(half_yaw);
  const float y_axis_sin = sinf(half_yaw);
  const float z_axis_cos = cosf(half_roll);
  const float z_axis_sin = sinf(half_roll);

  return bfQuaternionf_init(
   z_axis_cos * y_axis_cos * x_axis_sin - z_axis_sin * y_axis_sin * x_axis_cos,
   z_axis_cos * y_axis_sin * x_axis_cos + z_axis_sin * y_axis_cos * x_axis_sin,
   z_axis_sin * y_axis_cos * x_axis_cos - z_axis_cos * y_axis_sin * x_axis_sin,
   z_axis_cos * y_axis_cos * x_axis_cos + z_axis_sin * y_axis_sin * x_axis_sin);
}

Quaternionf bfQuaternionf_multQ(const Quaternionf* lhs, const Quaternionf* rhs)
{
  const float x = lhs->x * rhs->w + lhs->w * rhs->x + lhs->y * rhs->z - lhs->z * rhs->y;
  const float y = lhs->y * rhs->w + lhs->w * rhs->y + lhs->z * rhs->x - lhs->x * rhs->z;
  const float z = lhs->z * rhs->w + lhs->w * rhs->z + lhs->x * rhs->y - lhs->y * rhs->x;
  const float w = lhs->w * rhs->w - lhs->x * rhs->x - lhs->y * rhs->y - lhs->z * rhs->z;

  return bfQuaternionf_init(x, y, z, w);
}

void bfQuaternionf_multV(Quaternionf* self, const Vec3f* rhs)
{
  const float w = -self->x * rhs->x - self->y * rhs->y - self->z * rhs->z;
  const float x = self->w * rhs->x + self->y * rhs->z - self->z * rhs->y;
  const float y = self->w * rhs->y + self->z * rhs->x - self->x * rhs->z;
  const float z = self->w * rhs->z + self->x * rhs->y - self->y * rhs->x;

  self->x = x;
  self->y = y;
  self->z = z;
  self->w = w;
}

void bfQuaternionf_addVec(Quaternionf* self, const Vec3f* rhs, float multiplier)
{
  const Quaternionf q = bfQuaternionf_init(0.0f,
                                           rhs->x * multiplier,
                                           rhs->y * multiplier,
                                           rhs->z * multiplier);

  *self = bfQuaternionf_multQ(self, &q);

  self->w += q.w * 0.5f;
  self->x += q.x * 0.5f;
  self->y += q.y * 0.5f;
  self->z += q.z * 0.5f;
}

void bfQuaternionf_rotByVec(Quaternionf* self, const Vec3f* rhs)
{
  const Quaternionf q = bfQuaternionf_init(0.0f, rhs->x, rhs->y, rhs->z);
  *self               = bfQuaternionf_multQ(self, &q);
}

Quaternionf bfQuaternionf_conjugate(const Quaternionf* self)
{
  return bfQuaternionf_init(
   -self->x,
   -self->y,
   -self->z,
   self->w);
}

float bfQuaternionf_length(const Quaternionf* self)
{
  return sqrtf(bfQuaternionf_lengthSq(self));
}

float bfQuaternionf_lengthSq(const Quaternionf* self)
{
  return self->x * self->x + self->y * self->y + self->z * self->z + self->w * self->w;
}

void bfQuaternionf_normalize(Quaternionf* self)
{
  float length = bfQuaternionf_lengthSq(self);

  if (length > k_Epsilonf)
  {
    length = 1.0f / sqrtf(length);

    self->x *= length;
    self->y *= length;
    self->z *= length;
    self->w *= length;
  }
  else
  {
    *self = bfQuaternionf_identity();
  }
}

void bfQuaternionf_toMatrix(Quaternionf self, Mat4x4* out_rot_mat)
{
  // [https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm]

  bfQuaternionf_normalize(&self);

  const float qx = self.x;
  const float qy = self.y;
  const float qz = self.z;
  const float qw = self.w;

  *Mat4x4_get(out_rot_mat, 0, 0) = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
  *Mat4x4_get(out_rot_mat, 0, 1) = 2.0f * qx * qy + 2.0f * qz * qw;
  *Mat4x4_get(out_rot_mat, 0, 2) = 2.0f * qx * qz - 2.0f * qy * qw;
  *Mat4x4_get(out_rot_mat, 0, 3) = 0.0f;

  *Mat4x4_get(out_rot_mat, 1, 0) = 2.0f * qx * qy - 2.0f * qz * qw;
  *Mat4x4_get(out_rot_mat, 1, 1) = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
  *Mat4x4_get(out_rot_mat, 1, 2) = 2.0f * qy * qz + 2.0f * qx * qw;
  *Mat4x4_get(out_rot_mat, 1, 3) = 0.0f;

  *Mat4x4_get(out_rot_mat, 2, 0) = 2.0f * qx * qz + 2.0f * qy * qw;
  *Mat4x4_get(out_rot_mat, 2, 1) = 2.0f * qy * qz - 2.0f * qx * qw;
  *Mat4x4_get(out_rot_mat, 2, 2) = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;
  *Mat4x4_get(out_rot_mat, 2, 3) = 0.0f;

  *Mat4x4_get(out_rot_mat, 3, 0) = 0.0f;
  *Mat4x4_get(out_rot_mat, 3, 1) = 0.0f;
  *Mat4x4_get(out_rot_mat, 3, 2) = 0.0f;
  *Mat4x4_get(out_rot_mat, 3, 3) = 1.0f;
}

void bfQuaternionf_toEulerRad(const Quaternionf* self, Vec3f* out_rot_euler)
{
  const float sin_y_axis = 2.0f * (self->w * self->y - self->z * self->x);
  const float y_sq       = self->y * self->y;

  /* X-Axis (Pitch) */
  out_rot_euler->x = atan2f(2.0f * (self->x * self->w + self->y * self->z), 1.0f - 2.0f * (self->x * self->x + y_sq));

  /* Y-Axis (Yaw) */
  out_rot_euler->y = fabsf(sin_y_axis) >= 1.0f ? copysignf(k_HalfPI, sin_y_axis) : asinf(sin_y_axis); /* If Out of Range use 90 deg. */

  /* Z-Axis (Roll) */
  out_rot_euler->z = atan2f(2.0f * (self->w * self->z + self->x * self->y), 1.0f - 2.0f * (y_sq + self->z * self->z));
}

void bfQuaternionf_toEulerDeg(const Quaternionf* self, Vec3f* out_rot_euler)
{
  bfQuaternionf_toEulerRad(self, out_rot_euler);
  out_rot_euler->x *= k_RadToDegf;
  out_rot_euler->y *= k_RadToDegf;
  out_rot_euler->z *= k_RadToDegf;
}

Vec3f bfQuaternionf_up(const Quaternionf* self)
{
  return (Vec3f){
   2.0f * (self->x * self->y - self->w * self->z),
   1.0f - 2.0f * (self->x * self->x + self->z * self->z),
   2.0f * (self->y * self->z + self->w * self->x),
   0.0f,
  };
}

Vec3f bfQuaternionf_down(const Quaternionf* self)
{
  Vec3f ret = bfQuaternionf_up(self);
  Vec3f_mul(&ret, -1.0f);
  return ret;
}

Vec3f bfQuaternionf_left(const Quaternionf* self)
{
  Vec3f ret = bfQuaternionf_right(self);
  Vec3f_mul(&ret, -1.0f);
  return ret;
}

Vec3f bfQuaternionf_right(const Quaternionf* self)
{
  return (Vec3f){
   1.0f - 2.0f * (self->y * self->y + self->z * self->z),
   2.0f * (self->x * self->y + self->w * self->z),
   2.0f * (self->x * self->z - self->w * self->y),
   0.0f,
  };
}

Vec3f bfQuaternionf_forward(const Quaternionf* self)
{
  return (Vec3f){
   2.0f * (self->x * self->z + self->w * self->y),
   2.0f * (self->y * self->z - self->w * self->x),
   1.0f - 2.0f * (self->x * self->x + self->y * self->y),
   0.0f,
  };
}

Vec3f bfQuaternionf_backward(const Quaternionf* self)
{
  Vec3f ret = bfQuaternionf_forward(self);
  Vec3f_mul(&ret, -1.0f);
  return ret;
}

static float bfQuaternionf_dot(const bfQuaternionf* lhs, const bfQuaternionf* rhs)
{
  return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z + lhs->w * rhs->w;
}

bfQuaternionf bfQuaternionf_slerp(const bfQuaternionf* lhs, const bfQuaternionf* rhs, float factor)
{
  //
  // Adapted from Assimp / gmtl
  //

  bfQuaternionf end   = *rhs;
  float         cosom = bfQuaternionf_dot(lhs, &end);
  float         sclp;
  float         sclq;

  if (cosom < 0.0f)
  {
    cosom = -cosom;
    end.x = -end.x;
    end.y = -end.y;
    end.z = -end.z;
    end.w = -end.w;
  }

  if ((1.0f - cosom) > k_Epsilonf)
  {
    const float omega = (float)acosf(cosom);
    const float sinom = (float)sinf(omega);

    sclp = sinf((1.0f - factor) * omega) / sinom;
    sclq = sinf(factor * omega) / sinom;
  }
  else  // If the angles are close enough then just linearly interpolate.
  {
    sclp = 1.0f - factor;
    sclq = factor;
  }

  return bfQuaternionf_init(
   sclp * lhs->x + sclq * end.x,
   sclp * lhs->y + sclq * end.y,
   sclp * lhs->z + sclq * end.z,
   sclp * lhs->w + sclq * end.w);
}

/*
quat RotateTowards(quat q1, quat q2, float maxAngle)
{
  if (maxAngle < 0.001f)
  {
    // No rotation allowed. Prevent dividing by 0 later.
    return q1;
  }

  float cosTheta = dot(q1, q2);

  // q1 and q2 are already equal.
  // Force q2 just to be sure
  if (cosTheta > 0.9999f)
  {
    return q2;
  }

  // Avoid taking the long path around the sphere
  if (cosTheta < 0)
  {
    q1 = q1 * -1.0f;
    cosTheta *= -1.0f;
  }

  float angle = acos(cosTheta);

  // If there is only a 2&deg; difference, and we are allowed 5&deg;,
  // then we arrived.
  if (angle < maxAngle)
  {
    return q2;
  }

  float fT = maxAngle / angle;
  angle    = maxAngle;

  quat res = (sin((1.0f - fT) * angle) * q1 + sin(fT * angle) * q2) / sin(angle);
  res      = normalize(res);
  return res;
}
*/

// Transform

void bfTransform_ctor(bfTransform* self, bfTransform** dirty_list)
{
  Vec3f_set(&self->origin, 0.0f, 0.0f, 0.0f, 1.0f);
  Vec3f_set(&self->local_position, 0.0f, 0.0f, 0.0f, 1.0f);
  self->local_rotation = bfQuaternionf_identity();
  Vec3f_set(&self->local_scale, 1.0f, 1.0f, 1.0f, 0.0f);
  Vec3f_set(&self->world_position, 0.0f, 0.0f, 0.0f, 1.0f);
  self->world_rotation = bfQuaternionf_identity();
  Vec3f_set(&self->world_scale, 1.0f, 1.0f, 1.0f, 0.0f);
  Mat4x4_identity(&self->local_transform);
  Mat4x4_identity(&self->world_transform);
  self->parent          = NULL;
  self->first_child     = NULL;
  self->next_sibling    = NULL;
  self->prev_sibling    = NULL;
  self->dirty_list      = dirty_list;
  self->dirty_list_next = NULL;
  bfTransform_flushChanges(self);
  self->flags = BF_TRANSFORM_DIRTY;
}

void bfTransform_setOrigin(bfTransform* self, const Vec3f* value)
{
  self->origin = *value;
  bfTransform_flushChanges(self);
  self->flags |= BF_TRANSFORM_ORIGIN_DIRTY;
}

/*
Vec3f bfTransform_localPosition(const bfTransform* self)
{
  Vec3f result;

  result.x = Mat4x4_at(&self->local_transform, 3, 0);
  result.y = Mat4x4_at(&self->local_transform, 3, 1);
  result.z = Mat4x4_at(&self->local_transform, 3, 2);
  result.w = 1.0f;

  return result;
}
*/

void bfTransform_setPosition(bfTransform* self, const Vec3f* value)
{
  self->local_position = *value;
  bfTransform_flushChanges(self);
  self->flags |= BF_TRANSFORM_POSITION_DIRTY;
}

void bfTransform_setRotation(bfTransform* self, const Quaternionf* value)
{
  self->local_rotation = *value;
  bfTransform_flushChanges(self);
  self->flags |= BF_TRANSFORM_ROTATION_DIRTY;
}

void bfTransform_setScale(bfTransform* self, const Vec3f* value)
{
  self->local_scale = *value;
  bfTransform_flushChanges(self);
  self->flags |= BF_TRANSFORM_SCALE_DIRTY;
}

void bfTransform_setParent(bfTransform* self, bfTransform* value)
{
  if (self->parent != value)
  {
    bfTransform* const parent = self->parent;

    if (parent)
    {
      bfTransform* const prev_sibling = self->prev_sibling;
      bfTransform* const next_sibling = self->next_sibling;

      if (prev_sibling)
      {
        prev_sibling->next_sibling = self->next_sibling;
      }
      else
      {
        parent->first_child = self->next_sibling;
      }

      if (next_sibling)
      {
        next_sibling->prev_sibling = self->prev_sibling;
      }
    }

    if (value)
    {
      self->next_sibling = value->first_child;
      self->prev_sibling = NULL;

      if (value->first_child)
      {
        value->first_child->prev_sibling = self;
        // value->first_child->next_sibling = /*same */;
      }

      value->first_child = self;
    }

    self->parent = value;
    bfTransform_flushChanges(self);
    self->flags |= BF_TRANSFORM_PARENT_DIRTY;
  }
}

void bfTransform_copyFrom(bfTransform* self, const bfTransform* value)
{
  if (self != value)
  {
    self->origin         = value->origin;
    self->local_position = value->local_position;
    self->local_rotation = value->local_rotation;
    self->local_scale    = value->local_scale;

    bfTransform_flushChanges(self);
    self->flags |= BF_TRANSFORM_LOCAL_DIRTY;
  }
}

static void bfTransform_flushMatrix(Mat4x4* out, const Vec3f origin, const Vec3f position, const Quaternionf rotation, const Vec3f scale)
{
  Mat4x4 translation_mat;
  Mat4x4 rotation_mat;
  Mat4x4 scale_mat;
  Mat4x4 origin_mat;
  Vec3f  total_translation;

  total_translation = position;
  Vec3f_add(&total_translation, &origin);

  Mat4x4_initTranslatef(&translation_mat, total_translation.x, total_translation.y, total_translation.z);
  bfQuaternionf_toMatrix(rotation, &rotation_mat);
  Mat4x4_initScalef(&scale_mat, scale.x, scale.y, scale.z);
  Mat4x4_initTranslatef(&origin_mat, -origin.x, -origin.y, -origin.z);

  // TODO(SR):
  //   Multiplication by the `translation_mat` can probably be optimized
  //   since it always just adds (/ sets) the translation part of the matrix.
  //
  //   This overall can be optimized since `scale_mat` and `origin_mat` operate of diff parts of the matrix.
  //
  //  This info is kinda wrong but there ARE optimizations to be made.

  Mat4x4_mult(&scale_mat, &origin_mat, out);
  Mat4x4_mult(&rotation_mat, out, out);
  Mat4x4_mult(&translation_mat, out, out);
}

void bfTransform_flushChanges(bfTransform* self)
{
  bfTransform* work_stack[k_TransformQueueStackMax] = {NULL}; /* TODO(SR): Make this dynamically sized? */
  int          work_stack_top                       = 0;

  work_stack[work_stack_top++] = self;

  while (work_stack_top)
  {
    bfTransform*       node        = work_stack[--work_stack_top];
    const bfTransform* node_parent = node->parent;
    bfTransform*       child       = node->first_child;

    bfTransform_flushMatrix(
     &node->local_transform,
     node->origin,
     node->local_position,
     node->local_rotation,
     node->local_scale);

    if (node_parent)
    {
      const Mat4x4* const parent_mat = &node_parent->world_transform;

      Mat4x4_mult(parent_mat, &node->local_transform, &node->world_transform);

      Mat4x4_multVec(parent_mat, &node->local_position, &node->world_position);
      node->world_rotation = bfQuaternionf_multQ(&node_parent->world_rotation, &node->local_rotation);
      node->world_scale    = node->local_scale;
      Vec3f_multV(&node->world_scale, &node_parent->world_scale);
    }
    else
    {
      node->world_position  = node->local_position;
      node->world_rotation  = node->local_rotation;
      node->world_scale     = node->local_scale;
      node->world_transform = node->local_transform;
    }

    if (Mat4x4_inverse(&node->world_transform, &node->inv_world_transform))
    {
      node->normal_transform = node->inv_world_transform;
      Mat4x4_transpose(&node->normal_transform);
    }

    while (child)
    {
      assert(work_stack_top < k_TransformQueueStackMax);

      work_stack[work_stack_top++] = child;
      child                        = child->next_sibling;
    }

    // This the node was not dirty before then prepend to the dirty list.
    if ((node->flags & BF_TRANSFORM_LOCAL_DIRTY) == 0u)
    {
      node->dirty_list_next = *node->dirty_list;
      *node->dirty_list     = node;
    }

    node->flags |= BF_TRANSFORM_NEEDS_GPU_UPLOAD;

    if (node != self)
    {
      node->flags |= BF_TRANSFORM_PARENT_DIRTY;
    }
  }
}

void bfTransform_dtor(bfTransform* self)
{
  bfTransform_setParent(self, NULL);
}
