/*!
 * @file   bifrost_transform.c
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
#include "bifrost/math/bifrost_transform.h"

#include <assert.h>  // assert
#include <math.h>    // quat::sqrt
#include <stddef.h>  // NULL

#define k_PI (3.14159265359f)
#define k_HalfPI (k_PI * 0.5f)
#define k_RadToDegf (57.2958f)
#define k_DegToRadf (0.01745329251f)
#define k_Epsilonf 0.00001f

// Quaternion

// [https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles]

Quaternionf bfQuaternionf_init(float x, float y, float z, float w)
{
  const Quaternionf ret = {{{x, y, z, w}}};
  return ret;
}

Quaternionf bfQuaternionf_identity()
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
    self.w        = (m02 - m20) / s;
    self.x        = (m01 + m10) / s;
    self.y        = 0.25f * s;
    self.z        = (m12 + m21) / s;
  }
  else
  {
    const float s = 2.0f * sqrtf(1.0f + m22 - m00 - m11);
    self.w        = (m10 - m01) / s;
    self.x        = (m02 + m20) / s;
    self.y        = (m21 + m12) / s;
    self.z        = 0.25f * s;
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
  const float half_roll  = roll * 0.5f;
  const float half_pitch = pitch * 0.5f;
  const float half_yaw   = yaw * 0.5f;
  const float cos_yaw    = cosf(half_yaw);
  const float cos_pitch  = cosf(half_pitch);
  const float cos_roll   = cosf(half_roll);
  const float sin_yaw    = sinf(half_yaw);
  const float sin_pitch  = sinf(half_pitch);
  const float sin_roll   = sinf(half_roll);

  return bfQuaternionf_init(
   cos_yaw * cos_pitch * sin_roll - sin_yaw * sin_pitch * cos_roll,
   cos_yaw * sin_pitch * cos_roll + sin_yaw * cos_pitch * sin_roll,
   sin_yaw * cos_pitch * cos_roll - cos_yaw * sin_pitch * sin_roll,
   cos_yaw * cos_pitch * cos_roll + sin_yaw * sin_pitch * sin_roll);
}

void bfQuaternionf_multQ(Quaternionf* self, const Quaternionf* rhs)
{
  const float x = self->x * rhs->w + self->w * rhs->x + self->y * rhs->z - self->z * rhs->y;
  const float y = self->y * rhs->w + self->w * rhs->y + self->z * rhs->x - self->x * rhs->z;
  const float z = self->z * rhs->w + self->w * rhs->z + self->x * rhs->y - self->y * rhs->x;
  const float w = self->w * rhs->w - self->x * rhs->x - self->y * rhs->y - self->z * rhs->z;

  self->x = x;
  self->y = y;
  self->z = z;
  self->w = w;
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

  bfQuaternionf_multQ(self, &q);

  self->w += q.w * 0.5f;
  self->x += q.x * 0.5f;
  self->y += q.y * 0.5f;
  self->z += q.z * 0.5f;
}

void bfQuaternionf_rotByVec(Quaternionf* self, const Vec3f* rhs)
{
  const Quaternionf q = bfQuaternionf_init(0.0f, rhs->x, rhs->y, rhs->z);
  bfQuaternionf_multQ(self, &q);
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

void bfQuaternionf_toMatrix(Quaternionf* self, Mat4x4* out_rot_mat)
{
  Quaternionf normalized_clone = *self;
  bfQuaternionf_normalize(&normalized_clone);

  const float qx = normalized_clone.x;
  const float qy = normalized_clone.y;
  const float qz = normalized_clone.z;
  const float qw = normalized_clone.w;

  *Mat4x4_get(out_rot_mat, 0, 0) = 1.0f - 2.0f * qy * qy - 2.0f * qz * qz;
  *Mat4x4_get(out_rot_mat, 0, 1) = 2.0f * qx * qy - 2.0f * qz * qw;
  *Mat4x4_get(out_rot_mat, 0, 2) = 2.0f * qx * qz + 2.0f * qy * qw;
  *Mat4x4_get(out_rot_mat, 0, 3) = 0.0f;

  *Mat4x4_get(out_rot_mat, 1, 0) = 2.0f * qx * qy + 2.0f * qz * qw;
  *Mat4x4_get(out_rot_mat, 1, 1) = 1.0f - 2.0f * qx * qx - 2.0f * qz * qz;
  *Mat4x4_get(out_rot_mat, 1, 2) = 2.0f * qy * qz - 2.0f * qx * qw;
  *Mat4x4_get(out_rot_mat, 1, 3) = 0.0f;

  *Mat4x4_get(out_rot_mat, 2, 0) = 2.0f * qx * qz - 2.0f * qy * qw;
  *Mat4x4_get(out_rot_mat, 2, 1) = 2.0f * qy * qz + 2.0f * qx * qw;
  *Mat4x4_get(out_rot_mat, 2, 2) = 1.0f - 2.0f * qx * qx - 2.0f * qy * qy;
  *Mat4x4_get(out_rot_mat, 2, 3) = 0.0f;

  *Mat4x4_get(out_rot_mat, 3, 0) = 0.0f;
  *Mat4x4_get(out_rot_mat, 3, 1) = 0.0f;
  *Mat4x4_get(out_rot_mat, 3, 2) = 0.0f;
  *Mat4x4_get(out_rot_mat, 3, 3) = 1.0f;
}

void bfQuaternionf_toEulerRad(const Quaternionf* self, Vec3f* out_rot_euler)
{
  /* pitch */

  const float sin_pitch = 2.0f * (self->w * self->y - self->z * self->x);
  const float y_sq      = self->y * self->y;

  if (fabsf(sin_pitch) >= 1.0f)
  {
    /* If Out of Range use 90 deg. */
    out_rot_euler->x = copysignf(k_HalfPI, sin_pitch);
  }
  else
  {
    out_rot_euler->x = asinf(sin_pitch);
  }

  /* yaw */

  out_rot_euler->y = atan2f(2.0f * (self->w * self->z + self->x * self->y), 1.0f - 2.0f * (y_sq + self->z * self->z));

  /* roll */

  out_rot_euler->z = atan2f(
   2.0f * (self->x * self->w + self->y * self->z),
   1.0f - 2.0f * (self->x * self->x + y_sq));
}

void bfQuaternionf_toEulerDeg(const Quaternionf* self, Vec3f* out_rot_euler)
{
  bfQuaternionf_toEulerRad(self, out_rot_euler);
  out_rot_euler->x *= k_RadToDegf;
  out_rot_euler->y *= k_RadToDegf;
  out_rot_euler->z *= k_RadToDegf;
}

Vec3f bfQuaternionf_upVec(const Quaternionf* self)
{
  return (Vec3f){
   2.0f * (self->x * self->y + self->w * self->z),
   1.0f - 2.0f * (self->x * self->x + self->z * self->z),
   2.0f * (self->y * self->z - self->w * self->x),
   0.0f,
  };
}

Vec3f bfQuaternionf_downVec(const Quaternionf* self)
{
  Vec3f ret = bfQuaternionf_upVec(self);
  Vec3f_mul(&ret, -1.0f);
  return ret;
}

Vec3f bfQuaternionf_leftVec(const Quaternionf* self)
{
  Vec3f ret = bfQuaternionf_rightVec(self);
  Vec3f_mul(&ret, -1.0f);
  return ret;
}

Vec3f bfQuaternionf_rightVec(const Quaternionf* self)
{
  return (Vec3f){
   1.0f - 2.0f * (self->y * self->y + self->z * self->z),
   2.0f * (self->x * self->y - self->w * self->z),
   2.0f * (self->x * self->z + self->w * self->y),
   0.0f,
  };
}

Vec3f bfQuaternionf_forwardVec(const Quaternionf* self)
{
  return (Vec3f){
   2.0f * (self->x * self->z - self->w * self->y),
   2.0f * (self->y * self->z + self->w * self->x),
   1.0f - 2.0f * (self->x * self->x + self->y * self->y),
   0.0f,
  };
}

Vec3f bfQuaternionf_backwardVec(const Quaternionf* self)
{
  Vec3f ret = bfQuaternionf_forwardVec(self);
  Vec3f_mul(&ret, -1.0f);
  return ret;
}

// Transform

/* TODO(SR): Decide if these should be in the public interface. */
static BifrostTransform* bfTransformParent(const BifrostTransform* self);
static BifrostTransform* bfTransformFirstChild(const BifrostTransform* self);
static BifrostTransform* bfTransformNextSibling(const BifrostTransform* self);
static BifrostTransform* bfTransformPrevSibling(const BifrostTransform* self);

void bfTransform_ctor(BifrostTransform* self, struct IBifrostTransformSystem_t* system)
{
  assert(system && "All transforms must have a valid system it is a part of.");

  Vec3f_set(&self->origin, 0.0f, 0.0f, 0.0f, 1.0f);
  Vec3f_set(&self->local_position, 0.0f, 0.0f, 0.0f, 1.0f);
  self->local_rotation = bfQuaternionf_identity();
  Vec3f_set(&self->local_scale, 1.0f, 1.0f, 1.0f, 0.0f);
  Vec3f_set(&self->world_position, 0.0f, 0.0f, 0.0f, 1.0f);
  self->world_rotation = bfQuaternionf_identity();
  Vec3f_set(&self->world_scale, 1.0f, 1.0f, 1.0f, 0.0f);
  Mat4x4_identity(&self->local_transform);
  Mat4x4_identity(&self->world_transform);
  self->parent          = k_TransformInvalidID;
  self->first_child     = k_TransformInvalidID;
  self->next_sibling    = k_TransformInvalidID;
  self->prev_sibling    = k_TransformInvalidID;
  self->system          = system;
  self->dirty_list_next = NULL;
  self->flags           = BIFROST_TRANSFORM_DIRTY;
  // bfTransform_flushChanges(self);
}

void bfTransform_setOrigin(BifrostTransform* self, const Vec3f* value)
{
  self->origin = *value;
  self->flags |= BIFROST_TRANSFORM_ORIGIN_DIRTY;
  bfTransform_flushChanges(self);
}

void bfTransform_setPosition(BifrostTransform* self, const Vec3f* value)
{
  self->local_position = *value;
  self->flags |= BIFROST_TRANSFORM_POSITION_DIRTY;
  bfTransform_flushChanges(self);
}

void bfTransform_setRotation(BifrostTransform* self, const Quaternionf* value)
{
  self->local_rotation = *value;
  self->flags |= BIFROST_TRANSFORM_ROTATION_DIRTY;
  bfTransform_flushChanges(self);
}

void bfTransform_setScale(BifrostTransform* self, const Vec3f* value)
{
  self->local_scale = *value;
  self->flags |= BIFROST_TRANSFORM_SCALE_DIRTY;
  bfTransform_flushChanges(self);
}

void bfTransform_setParent(BifrostTransform* self, BifrostTransform* value)
{
  const BifrostTransformID value_id = value->system->transformToID(value->system, value);

  if (self->parent != value_id)
  {
    const BifrostTransformID self_id = self->system->transformToID(self->system, self);
    BifrostTransform* const  parent  = bfTransformParent(self);

    if (parent)
    {
      BifrostTransform* const prev_sibling = bfTransformPrevSibling(self);
      BifrostTransform* const next_sibling = bfTransformNextSibling(self);

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
      self->prev_sibling = k_TransformInvalidID;

      if (value->first_child)
      {
        bfTransformFirstChild(value)->prev_sibling = self_id;
        // value->first_child->next_sibling = /*same */;
      }

      value->first_child = self_id;
    }

    self->parent = value_id;
    self->flags |= BIFROST_TRANSFORM_PARENT_DIRTY;
    bfTransform_flushChanges(self);
  }
}

void bfTransform_copyFrom(BifrostTransform* self, const BifrostTransform* value)
{
  if (self != value)
  {
    self->origin         = value->origin;
    self->local_position = value->local_position;
    self->local_rotation = value->local_rotation;
    self->local_scale    = value->local_scale;

    self->flags |= BIFROST_TRANSFORM_LOCAL_DIRTY;

    bfTransform_flushChanges(self);
  }
}

void bfTransform_flushChanges(BifrostTransform* self)
{
  BifrostTransform* work_stack[k_TransformQueueStackMax] = {NULL}; /* TODO(Shareef): Make this dynamic? */
  int               work_stack_top                       = 0;

  work_stack[work_stack_top++] = self;

  Mat4x4 translation_mat;
  Mat4x4 rotation_mat;
  Mat4x4 scale_mat;
  Mat4x4 origin_mat;
  Vec3f  total_translation;

  while (work_stack_top)
  {
    BifrostTransform*       node        = work_stack[--work_stack_top];
    const BifrostTransform* node_parent = bfTransformParent(node);
    BifrostTransform*       child       = bfTransformFirstChild(node);

    total_translation = node->local_position;
    Vec3f_add(&total_translation, &node->origin);

    Mat4x4_initTranslatef(&translation_mat, total_translation.x, total_translation.y, total_translation.z);
    bfQuaternionf_toMatrix(&self->local_rotation, &rotation_mat);
    Mat4x4_initScalef(&scale_mat, self->local_scale.x, self->local_scale.y, self->local_scale.z);
    Mat4x4_initTranslatef(&origin_mat, -self->origin.x, -self->origin.y, -self->origin.z);

    Mat4x4_mult(&scale_mat, &origin_mat, &node->local_transform);
    Mat4x4_mult(&rotation_mat, &node->local_transform, &node->local_transform);
    Mat4x4_mult(&translation_mat, &node->local_transform, &node->local_transform);

    if (node_parent)
    {
      const Mat4x4* const parent_mat = &node_parent->world_transform;

      Mat4x4_mult(&node->local_transform, parent_mat, &node->world_transform);
      Mat4x4_multVec(parent_mat, &node->local_position, &self->world_position);
      node->world_rotation = node->local_rotation;
      bfQuaternionf_multQ(&node->world_rotation, &node_parent->world_rotation);
      node->world_scale = node->local_scale;
      Vec3f_multV(&node->world_scale, &node_parent->world_scale);
    }
    else
    {
      node->world_position  = node->local_position;
      node->world_rotation  = node->local_rotation;
      node->world_scale     = node->local_scale;
      node->world_transform = node->local_transform;
    }

    if (Mat4x4_inverse(&node->world_transform, &node->normal_transform))
    {
      Mat4x4_transpose(&node->normal_transform);
    }

    while (child)
    {
      assert(work_stack_top < k_TransformQueueStackMax);

      work_stack[work_stack_top++] = child;
      child                        = bfTransformNextSibling(child);
    }

    if (!node->flags)
    {
      node->system->addToDirtyList(node->system, node);
    }

    node->flags |= BIFROST_TRANSFORM_NEEDS_GPU_UPLOAD;

    if (node != self)
    {
      node->flags |= BIFROST_TRANSFORM_PARENT_DIRTY;
    }
  }
}

void bfTransform_dtor(BifrostTransform* self)
{
  bfTransform_setParent(self, NULL);
}

static BifrostTransform* bfTransformParent(const BifrostTransform* self)
{
  return self->system->transformFromID(self->system, self->parent);
}

static BifrostTransform* bfTransformFirstChild(const BifrostTransform* self)
{
  return self->system->transformFromID(self->system, self->first_child);
}

static BifrostTransform* bfTransformNextSibling(const BifrostTransform* self)
{
  return self->system->transformFromID(self->system, self->next_sibling);
}

static BifrostTransform* bfTransformPrevSibling(const BifrostTransform* self)
{
  return self->system->transformFromID(self->system, self->prev_sibling);
}
