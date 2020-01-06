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

Quaternionf bfQuaternionf_init(float x, float y, float z, float w);
Quaternionf bfQuaternionf_identity();
Quaternionf bfQuaternionf_fromAxisAngleRad(const Vec3f* axis, float angle);
Quaternionf bfQuaternionf_fromAxisAngleDeg(const Vec3f* axis, float angle);
Quaternionf bfQuaternionf_fromMatrix(const Mat4x4* rot_mat);
Quaternionf bfQuaternionf_fromEuler(float roll, float pitch, float yaw);  // X, Y, Z
void        bfQuaternionf_multQ(Quaternionf* self, const Quaternionf* rhs);
void        bfQuaternionf_multV(Quaternionf* self, const Vec3f* rhs);
void        bfQuaternionf_addVec(Quaternionf* self, const Vec3f* rhs, float multiplier);
void        bfQuaternionf_rotByVec(Quaternionf* self, const Vec3f* rhs);
Quaternionf bfQuaternionf_conjugate(const Quaternionf* self);
float       bfQuaternionf_length(const Quaternionf* self);
float       bfQuaternionf_lengthSq(const Quaternionf* self);
void        bfQuaternionf_normalize(Quaternionf* self);
void        bfQuaternionf_toMatrix(Quaternionf* self, Mat4x4* out_rot_mat);
void        bfQuaternionf_toEulerRad(const Quaternionf* self, Vec3f* out_rot_euler);
void        bfQuaternionf_toEulerDeg(const Quaternionf* self, Vec3f* out_rot_euler);
Vec3f       bfQuaternionf_upVec(const Quaternionf* self);
Vec3f       bfQuaternionf_downVec(const Quaternionf* self);
Vec3f       bfQuaternionf_leftVec(const Quaternionf* self);
Vec3f       bfQuaternionf_rightVec(const Quaternionf* self);
Vec3f       bfQuaternionf_forwardVec(const Quaternionf* self);
Vec3f       bfQuaternionf_backwardVec(const Quaternionf* self);

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

void bfTransform_ctor(BifrostTransform* self);
void bfTransform_setOrigin(BifrostTransform* self, const Vec3f* value);
void bfTransform_setPosition(BifrostTransform* self, const Vec3f* value);
void bfTransform_setRotation(BifrostTransform* self, const Quaternionf* value);
void bfTransform_setScale(BifrostTransform* self, const Vec3f* value);
void bfTransform_setParent(BifrostTransform* self, BifrostTransform* value);
void bfTransform_flushChanges(BifrostTransform* self);
void bfTransform_dtor(BifrostTransform* self);

#if __cplusplus
}
#endif

#endif /* transform_h */
