/*!
 * @file   bifrost_transform.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
#ifndef BIFROST_TRANSFORM_H
#define BIFROST_TRANSFORM_H

#include "bifrost_mat4x4.h"
#include "bifrost_vec3.h"

#include <stdint.h> /* uint8_t */

#if __cplusplus
extern "C" {
#endif

typedef struct Quaternionf_t
{
  union
  {
    struct
    {
      float x;
      float y;
      float z;
      float w;
    };

    struct
    {
      float i;
      float j;
      float k;
      float r;
    };
  };

} Quaternionf;

BIFROST_MATH_API Quaternionf bfQuaternionf_init(float x, float y, float z, float w);
BIFROST_MATH_API Quaternionf bfQuaternionf_identity();
BIFROST_MATH_API Quaternionf bfQuaternionf_fromAxisAngleRad(const Vec3f* axis, float angle);
BIFROST_MATH_API Quaternionf bfQuaternionf_fromAxisAngleDeg(const Vec3f* axis, float angle);
BIFROST_MATH_API Quaternionf bfQuaternionf_fromMatrix(const Mat4x4* rot_mat);
BIFROST_MATH_API Quaternionf bfQuaternionf_fromEulerDeg(float pitch, float yaw, float roll); /* x (pitch), y (yaw), z (roll) */
BIFROST_MATH_API Quaternionf bfQuaternionf_fromEulerRad(float pitch, float yaw, float roll); /* x (pitch), y (yaw), z (roll) */
BIFROST_MATH_API void        bfQuaternionf_multQ(Quaternionf* self, const Quaternionf* rhs);
BIFROST_MATH_API void        bfQuaternionf_multV(Quaternionf* self, const Vec3f* rhs);
BIFROST_MATH_API void        bfQuaternionf_addVec(Quaternionf* self, const Vec3f* rhs, float multiplier);
BIFROST_MATH_API void        bfQuaternionf_rotByVec(Quaternionf* self, const Vec3f* rhs);
BIFROST_MATH_API Quaternionf bfQuaternionf_conjugate(const Quaternionf* self);
BIFROST_MATH_API float       bfQuaternionf_length(const Quaternionf* self);
BIFROST_MATH_API float       bfQuaternionf_lengthSq(const Quaternionf* self);
BIFROST_MATH_API void        bfQuaternionf_normalize(Quaternionf* self);
BIFROST_MATH_API void        bfQuaternionf_toMatrix(const Quaternionf* self, Mat4x4* out_rot_mat);
BIFROST_MATH_API void        bfQuaternionf_toEulerRad(const Quaternionf* self, Vec3f* out_rot_euler); /* x (pitch), y (yaw), z (roll) */
BIFROST_MATH_API void        bfQuaternionf_toEulerDeg(const Quaternionf* self, Vec3f* out_rot_euler); /* x (pitch), y (yaw), z (roll) */
BIFROST_MATH_API Vec3f       bfQuaternionf_upVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_downVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_leftVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_rightVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_forwardVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_backwardVec(const Quaternionf* self);

enum
{
  k_TransformInvalidID     = 0,
  k_TransformQueueStackMax = 128,
};

struct IBifrostTransformSystem_t;
typedef struct IBifrostTransformSystem_t IBifrostTransformSystem;

typedef uint32_t BifrostTransformID;

typedef enum bfTransformFlags_t
{
  BIFROST_TRANSFORM_ORIGIN_DIRTY     = (1 << 0),
  BIFROST_TRANSFORM_POSITION_DIRTY   = (1 << 1),
  BIFROST_TRANSFORM_ROTATION_DIRTY   = (1 << 2),
  BIFROST_TRANSFORM_SCALE_DIRTY      = (1 << 3),
  BIFROST_TRANSFORM_PARENT_DIRTY     = (1 << 4),
  BIFROST_TRANSFORM_CHILD_DIRTY      = (1 << 5),
  BIFROST_TRANSFORM_NEEDS_GPU_UPLOAD = (1 << 6),

  /* Helper Flags */
  BIFROST_TRANSFORM_NONE        = 0x0,
  BIFROST_TRANSFORM_DIRTY       = 0xFF,
  BIFROST_TRANSFORM_LOCAL_DIRTY = BIFROST_TRANSFORM_ORIGIN_DIRTY |
                                  BIFROST_TRANSFORM_POSITION_DIRTY |
                                  BIFROST_TRANSFORM_ROTATION_DIRTY |
                                  BIFROST_TRANSFORM_SCALE_DIRTY,

} bfTransformFlags;

/*!
  All of these fields are considered 'read-only' unless you
  manually call 'bfTransform_flushChanges' after maniplulating the fields.

  You may only modify:
      - 'BifrostTransform::origin'
      - 'BifrostTransform::local_position'
      - 'BifrostTransform::local_rotation'
      - 'BifrostTransform::local_scale'

  Or use the "Transform_set*" API for automatic flushing of changes.
*/
typedef struct BifrostTransform_t
{
  Vec3f                             origin;
  Vec3f                             local_position;
  Quaternionf                       local_rotation;
  Vec3f                             local_scale;
  Vec3f                             world_position;
  Quaternionf                       world_rotation;
  Vec3f                             world_scale;
  Mat4x4                            local_transform;
  Mat4x4                            world_transform;
  Mat4x4                            normal_transform;
  BifrostTransformID                parent;
  BifrostTransformID                first_child;
  BifrostTransformID                next_sibling;
  BifrostTransformID                prev_sibling;
  struct IBifrostTransformSystem_t* system;
  struct BifrostTransform_t*        dirty_list_next;
  /* bfTransformFlags */ uint8_t    flags;

} BifrostTransform;

BIFROST_MATH_API void bfTransform_ctor(BifrostTransform* self, struct IBifrostTransformSystem_t* system);
BIFROST_MATH_API void bfTransform_setOrigin(BifrostTransform* self, const Vec3f* value);
BIFROST_MATH_API void bfTransform_setPosition(BifrostTransform* self, const Vec3f* value);
BIFROST_MATH_API void bfTransform_setRotation(BifrostTransform* self, const Quaternionf* value);
BIFROST_MATH_API void bfTransform_setScale(BifrostTransform* self, const Vec3f* value);
BIFROST_MATH_API BifrostTransform* bfTransform_parent(const BifrostTransform* self);
BIFROST_MATH_API void              bfTransform_setParent(BifrostTransform* self, BifrostTransform* value);
BIFROST_MATH_API void              bfTransform_copyFrom(BifrostTransform* self, const BifrostTransform* value); /* Copies over the local values, parent relationships are unchanged. */
BIFROST_MATH_API void              bfTransform_flushChanges(BifrostTransform* self);
BIFROST_MATH_API void              bfTransform_dtor(BifrostTransform* self);

struct IBifrostTransformSystem_t
{
  BifrostTransform* dirty_list;
  BifrostTransform* (*transformFromID)(struct IBifrostTransformSystem_t* self, BifrostTransformID id);
  BifrostTransformID (*transformToID)(struct IBifrostTransformSystem_t* self, BifrostTransform* transform);
  void (*addToDirtyList)(struct IBifrostTransformSystem_t* self, BifrostTransform* transform);
};

#if __cplusplus
}
#endif

#endif /* BIFROST_TRANSFORM_H */
