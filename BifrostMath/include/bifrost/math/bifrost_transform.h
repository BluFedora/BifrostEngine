/*!
 * @file   bifrost_transform.h
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date 2019-12-22
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef transform_h
#define transform_h

#include "bifrost_mat4x4.h"
#include "bifrost_vec3.h"

#if __cplusplus
extern "C" {
#endif

typedef struct Quaternionf_t
{
  union
  {
    struct
    {
      float x;  // i
      float y;  // j
      float z;  // k
      float w;  // r
    };

    struct
    {
      float i;  // i
      float j;  // j
      float k;  // k
      float r;  // r
    };
  };

} Quaternionf;

BIFROST_MATH_API Quaternionf bfQuaternionf_init(float x, float y, float z, float w);
BIFROST_MATH_API Quaternionf bfQuaternionf_identity();
BIFROST_MATH_API Quaternionf bfQuaternionf_fromAxisAngleRad(const Vec3f* axis, float angle);
BIFROST_MATH_API Quaternionf bfQuaternionf_fromAxisAngleDeg(const Vec3f* axis, float angle);
BIFROST_MATH_API Quaternionf bfQuaternionf_fromMatrix(const Mat4x4* rot_mat);
BIFROST_MATH_API Quaternionf bfQuaternionf_fromEuler(float roll, float pitch, float yaw);  // X, Y, Z
BIFROST_MATH_API void        bfQuaternionf_multQ(Quaternionf* self, const Quaternionf* rhs);
BIFROST_MATH_API void        bfQuaternionf_multV(Quaternionf* self, const Vec3f* rhs);
BIFROST_MATH_API void        bfQuaternionf_addVec(Quaternionf* self, const Vec3f* rhs, float multiplier);
BIFROST_MATH_API void        bfQuaternionf_rotByVec(Quaternionf* self, const Vec3f* rhs);
BIFROST_MATH_API Quaternionf bfQuaternionf_conjugate(const Quaternionf* self);
BIFROST_MATH_API float       bfQuaternionf_length(const Quaternionf* self);
BIFROST_MATH_API float       bfQuaternionf_lengthSq(const Quaternionf* self);
BIFROST_MATH_API void        bfQuaternionf_normalize(Quaternionf* self);
BIFROST_MATH_API void        bfQuaternionf_toMatrix(Quaternionf* self, Mat4x4* out_rot_mat);
BIFROST_MATH_API void        bfQuaternionf_toEulerRad(const Quaternionf* self, Vec3f* out_rot_euler);
BIFROST_MATH_API void        bfQuaternionf_toEulerDeg(const Quaternionf* self, Vec3f* out_rot_euler);
BIFROST_MATH_API Vec3f       bfQuaternionf_upVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_downVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_leftVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_rightVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_forwardVec(const Quaternionf* self);
BIFROST_MATH_API Vec3f       bfQuaternionf_backwardVec(const Quaternionf* self);

/*!
  All of these fields are 'read-only' unless you
  manully flush the changes to the Transform after maniplulating it.
  Or use the "Transform_set*" functions to do it for you.
*/
typedef struct BifrostTransform_t
{
  Vec3f                      origin;
  Vec3f                      local_position;
  Quaternionf                local_rotation;
  Vec3f                      local_scale;
  Vec3f                      world_position;
  Quaternionf                world_rotation;
  Vec3f                      world_scale;
  Mat4x4                     local_transform;
  Mat4x4                     world_transform;
  struct BifrostTransform_t* parent;
  struct BifrostTransform_t* first_child;
  struct BifrostTransform_t* next_sibling;
  struct BifrostTransform_t* prev_sibling;

} BifrostTransform;

BIFROST_MATH_API void bfTransform_ctor(BifrostTransform* self);
BIFROST_MATH_API void bfTransform_setOrigin(BifrostTransform* self, const Vec3f* value);
BIFROST_MATH_API void bfTransform_setPosition(BifrostTransform* self, const Vec3f* value);
BIFROST_MATH_API void bfTransform_setRotation(BifrostTransform* self, const Quaternionf* value);
BIFROST_MATH_API void bfTransform_setScale(BifrostTransform* self, const Vec3f* value);
BIFROST_MATH_API void bfTransform_setParent(BifrostTransform* self, BifrostTransform* value);
BIFROST_MATH_API void bfTransform_flushChanges(BifrostTransform* self);
BIFROST_MATH_API void bfTransform_dtor(BifrostTransform* self);

#if __cplusplus
}
#endif

#endif /* transform_h */
