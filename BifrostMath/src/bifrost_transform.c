#include "bifrost/math/bifrost_transform.h"

#include <assert.h>  // assert
#include <math.h>    // quat::sqrt
#include <stddef.h>  // NULL

#define EPSILONf 0.00001f
#define DEG_TO_RADf 0.01745329251f
#define RAD_TO_DEGf 57.2958f

// Quaternion

// [https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles]

Quaternionf bfQuaternionf_init(float x, float y, float z, float w)
{
  const Quaternionf ret = {x, y, z, w};
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
  return bfQuaternionf_fromAxisAngleRad(axis, angle * DEG_TO_RADf);
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

Quaternionf bfQuaternionf_fromEuler(float roll, float pitch, float yaw)
{
  const float half_yaw   = yaw * 0.5f;
  const float half_pitch = pitch * 0.5f;
  const float half_roll  = roll * 0.5f;
  const float cy         = cosf(half_yaw);
  const float sy         = sinf(half_yaw);
  const float cp         = cosf(half_pitch);
  const float sp         = sinf(half_pitch);
  const float cr         = cosf(half_roll);
  const float sr         = sinf(half_roll);

  return bfQuaternionf_init(
   cy * cp * cr + sy * sp * sr,
   cy * cp * sr - sy * sp * cr,
   sy * cp * sr + cy * sp * cr,
   sy * cp * cr - cy * sp * sr);
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

  if (length > EPSILONf)
  {
    length = 1.0f / sqrtf(length);

    self->x *= length;
    self->y *= length;
    self->z *= length;
    self->w *= length;
  }
  else
  {
    // Identity
    self->w = 1.0f;
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
  out_rot_euler->x = atan2f(2.0f * (self->y * self->z + self->w * self->x), self->w * self->w - self->x * self->x - self->y * self->y + self->z * self->z);  // X
  out_rot_euler->y = asinf(-2.0f * (self->x * self->z - self->w * self->y));                                                                                 // Y
  out_rot_euler->z = atan2f(2.0f * (self->x * self->y + self->w * self->z), self->w * self->w + self->x * self->x - self->y * self->y - self->z * self->z);  // Z
}

void bfQuaternionf_toEulerDeg(const Quaternionf* self, Vec3f* out_rot_euler)
{
  bfQuaternionf_toEulerRad(self, out_rot_euler);
  out_rot_euler->x *= RAD_TO_DEGf;
  out_rot_euler->y *= RAD_TO_DEGf;
  out_rot_euler->z *= RAD_TO_DEGf;
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

enum
{
  k_MaxTransformQueueStack = 128,
};

void bfTransform_ctor(BifrostTransform* self)
{
  Vec3f_set(&self->origin, 0.0f, 0.0f, 0.0f, 0.0f);
  Vec3f_set(&self->local_position, 0.0f, 0.0f, 0.0f, 0.0f);
  self->local_rotation = bfQuaternionf_identity();
  Vec3f_set(&self->local_scale, 1.0f, 1.0f, 1.0f, 0.0f);
  Vec3f_set(&self->world_position, 0.0f, 0.0f, 0.0f, 0.0f);
  self->world_rotation = bfQuaternionf_identity();
  Vec3f_set(&self->world_scale, 1.0f, 1.0f, 1.0f, 0.0f);
  Mat4x4_identity(&self->local_transform);
  Mat4x4_identity(&self->world_transform);
  self->parent           = NULL;
  self->first_child      = NULL;
  self->next_sibling     = NULL;
  self->prev_sibling     = NULL;
  bfTransform_flushChanges(self);
  self->needs_gpu_upload = 1;
}

void bfTransform_setOrigin(BifrostTransform* self, const Vec3f* value)
{
  self->origin = *value;
  bfTransform_flushChanges(self);
}

void bfTransform_setPosition(BifrostTransform* self, const Vec3f* value)
{
  self->local_position = *value;
  bfTransform_flushChanges(self);
}

void bfTransform_setRotation(BifrostTransform* self, const Quaternionf* value)
{
  self->local_rotation = *value;
  bfTransform_flushChanges(self);
}

void bfTransform_setScale(BifrostTransform* self, const Vec3f* value)
{
  self->local_scale = *value;
  bfTransform_flushChanges(self);
}

void bfTransform_setParent(BifrostTransform* self, BifrostTransform* value)
{
  if (self->parent != value)
  {
    if (self->parent)
    {
      if (self->prev_sibling)
      {
        self->prev_sibling->next_sibling = self->next_sibling;
      }
      else
      {
        self->parent->first_child = self->next_sibling;
      }

      if (self->next_sibling)
      {
        self->next_sibling->prev_sibling = self->prev_sibling;
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
  }
}

void bfTransform_flushChanges(BifrostTransform* self)
{
  // TODO(Shareef): Make this dynamic?
  BifrostTransform* work_stack[k_MaxTransformQueueStack] = {NULL};
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
    const BifrostTransform* node_parent = node->parent;
    BifrostTransform*       child       = node->first_child;

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
      assert(work_stack_top < k_MaxTransformQueueStack);

      work_stack[work_stack_top++] = child;
      child                        = child->next_sibling;
    }

    node->needs_gpu_upload = 1;
  }
}

void bfTransform_dtor(BifrostTransform* self)
{
  bfTransform_setParent(self, NULL);
}
