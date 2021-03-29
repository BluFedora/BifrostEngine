/*!
 * @file   bifrost_transform.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2021
 */
#ifndef BF_TRANSFORM_H
#define BF_TRANSFORM_H

#include "bifrost_mat4x4.h"
#include "bifrost_vec3.h"

#include <stdint.h> /* uint32_t */

#if __cplusplus
extern "C" {
#endif

// [http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/]
// [https://keithmaggio.wordpress.com/2011/02/15/math-magician-lerp-slerp-and-nlerp/]
typedef union bfQuaternionf
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

} Quaternionf;

typedef Quaternionf bfQuaternionf;

BF_MATH_API Quaternionf   bfQuaternionf_init(float x, float y, float z, float w);
BF_MATH_API Quaternionf   bfQuaternionf_identity(void);
BF_MATH_API Quaternionf   bfQuaternionf_fromAxisAngleRad(const Vec3f* axis, float angle);
BF_MATH_API Quaternionf   bfQuaternionf_fromAxisAngleDeg(const Vec3f* axis, float angle);
BF_MATH_API Quaternionf   bfQuaternionf_fromMatrix(const Mat4x4* rot_mat);
BF_MATH_API Quaternionf   bfQuaternionf_fromEulerDeg(float pitch, float yaw, float roll);       /* x (pitch), y (yaw), z (roll) */
BF_MATH_API Quaternionf   bfQuaternionf_fromEulerRad(float pitch, float yaw, float roll);       /* x (pitch), y (yaw), z (roll) */
BF_MATH_API Quaternionf   bfQuaternionf_multQ(const Quaternionf* lhs, const Quaternionf* rhs);  // rhs happens _first_
BF_MATH_API void          bfQuaternionf_multV(Quaternionf* self, const Vec3f* rhs);
BF_MATH_API void          bfQuaternionf_addVec(Quaternionf* self, const Vec3f* rhs, float multiplier);
BF_MATH_API void          bfQuaternionf_rotByVec(Quaternionf* self, const Vec3f* rhs);
BF_MATH_API Quaternionf   bfQuaternionf_conjugate(const Quaternionf* self);
BF_MATH_API float         bfQuaternionf_length(const Quaternionf* self);
BF_MATH_API float         bfQuaternionf_lengthSq(const Quaternionf* self);
BF_MATH_API void          bfQuaternionf_normalize(Quaternionf* self);
BF_MATH_API void          bfQuaternionf_toMatrix(Quaternionf self, Mat4x4* out_rot_mat);
BF_MATH_API void          bfQuaternionf_toEulerRad(const Quaternionf* self, Vec3f* out_rot_euler); /* x (pitch), y (yaw), z (roll) */
BF_MATH_API void          bfQuaternionf_toEulerDeg(const Quaternionf* self, Vec3f* out_rot_euler); /* x (pitch), y (yaw), z (roll) */
BF_MATH_API Vec3f         bfQuaternionf_up(const Quaternionf* self);                               // positive y-axis
BF_MATH_API Vec3f         bfQuaternionf_down(const Quaternionf* self);                             // negative y-axis
BF_MATH_API Vec3f         bfQuaternionf_left(const Quaternionf* self);                             // negative x-axis
BF_MATH_API Vec3f         bfQuaternionf_right(const Quaternionf* self);                            // positive x-xis
BF_MATH_API Vec3f         bfQuaternionf_forward(const Quaternionf* self);                          // positive z-axis
BF_MATH_API Vec3f         bfQuaternionf_backward(const Quaternionf* self);                         // negative z-axis
BF_MATH_API bfQuaternionf bfQuaternionf_slerp(const bfQuaternionf* lhs, const bfQuaternionf* rhs, float factor);

enum
{
  k_TransformQueueStackMax = 128,
};

typedef enum bfTransformFlags
{
  BF_TRANSFORM_ORIGIN_DIRTY     = (1 << 0),
  BF_TRANSFORM_POSITION_DIRTY   = (1 << 1),
  BF_TRANSFORM_ROTATION_DIRTY   = (1 << 2),
  BF_TRANSFORM_SCALE_DIRTY      = (1 << 3),
  BF_TRANSFORM_PARENT_DIRTY     = (1 << 4),
  BF_TRANSFORM_NEEDS_GPU_UPLOAD = (1 << 4),
  BF_TRANSFORM_ADOPT_SCALE      = (1 << 5),
  BF_TRANSFORM_ADOPT_ROTATION   = (1 << 6),
  BF_TRANSFORM_ADOPT_POSITION   = (1 << 7),

  /* Helper Flags */
  BF_TRANSFORM_NONE        = 0x0,
  BF_TRANSFORM_DIRTY       = 0xFF,
  BF_TRANSFORM_LOCAL_DIRTY = BF_TRANSFORM_ORIGIN_DIRTY |
                             BF_TRANSFORM_POSITION_DIRTY |
                             BF_TRANSFORM_ROTATION_DIRTY |
                             BF_TRANSFORM_SCALE_DIRTY,

} bfTransformFlags;

/*!
  @brief
    All of these fields are considered 'read-only' unless you
    manually call 'bfTransform_flushChanges' after manipulating the fields.
    
    You may only modify:
        - 'bfTransform::origin'
        - 'bfTransform::local_position'
        - 'bfTransform::local_rotation'
        - 'bfTransform::local_scale'
    
    Or use the "Transform_set*" API for automatic flushing of changes.
*/
typedef struct bfTransform bfTransform;
struct bfTransform
{
  /* World Transform */
  Vec3f       world_position;      /*!< Cached position in world coordinates.                         */
  Quaternionf world_rotation;      /*!< Cached rotation in world coordinates.                         */
  Vec3f       world_scale;         /*!< Cached scale in world coordinates.                            */
  Mat4x4      world_transform;     /*!< Cached matrix representing the world transform.               */
  Mat4x4      inv_world_transform; /*!< Cached matrix representing the inverse world transform.       */
  Mat4x4      normal_transform;    /*!< The inverse transpose of `world_transform`.                   */

  /* Local Transform */
  Vec3f       origin;          /*!< The pivot point from which the entity will rotate and scale from. */
  Vec3f       local_position;  /*!< Position relative to parent coordinate system.                    */
  Quaternionf local_rotation;  /*!< Rotation relative to parent coordinate system.                    */
  Vec3f       local_scale;     /*!< Scale relative to parent coordinate system.                       */
  Mat4x4      local_transform; /*!< Cached matrix representing the local transform.                   */

  /* Hierarchy */
  bfTransform* parent;       /*!< Parent transform object.                                            */
  bfTransform* first_child;  /*!< First child of this transform.                                      */
  bfTransform* next_sibling; /*!< The next sibling of this transform.                                 */
  bfTransform* prev_sibling; /*!< The previous sibling of this transform.                             */

  /* Dirty Flagging */
  bfTransform** dirty_list;      /*!< Head of the dirty list.                                          */
  bfTransform*  dirty_list_next; /*!< Next item in the embedded dirty linked list.                     */
  uint32_t      flags;           /*!< bfTransformFlags, flags for various feature and dirty states.    */
};

BF_MATH_API void bfTransform_ctor(bfTransform* self, bfTransform** dirty_list);
BF_MATH_API void bfTransform_setOrigin(bfTransform* self, const Vec3f* value);
BF_MATH_API void bfTransform_setPosition(bfTransform* self, const Vec3f* value);
BF_MATH_API void bfTransform_setRotation(bfTransform* self, const Quaternionf* value);
BF_MATH_API void bfTransform_setScale(bfTransform* self, const Vec3f* value);
BF_MATH_API void bfTransform_setParent(bfTransform* self, bfTransform* value);
BF_MATH_API void bfTransform_copyFrom(bfTransform* self, const bfTransform* value); /* Copies over the local values, parent relationships are unchanged. */
BF_MATH_API void bfTransform_flushChanges(bfTransform* self);
BF_MATH_API void bfTransform_dtor(bfTransform* self);

#if __cplusplus
}
#endif

#endif /* BF_TRANSFORM_H */
