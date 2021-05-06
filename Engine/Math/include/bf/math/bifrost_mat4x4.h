#ifndef mat4x4_h
#define mat4x4_h

#include "bifrost_math_export.h" /* BF_MATH_API */

#include "bifrost_vec3.h"

#if _MSC_VER
#define ALIGN_STRUCT(n) __declspec(align(n))
#else  // Clang and GCC
#define ALIGN_STRUCT(n) __attribute__((aligned(n)))
#endif

#define MATRIX_ROW_MAJOR 0
#define MATRIX_COL_MAJOR !MATRIX_ROW_MAJOR

#if defined(__SSE__)
// TODO: #define MATRIX_SSE 1
#define MATRIX_SSE 0
#else
#define MATRIX_SSE 0
#endif

// #ifdef __arm__   // AArch32
// #ifdef __arm64__ // AArch64

#if __cplusplus
extern "C" {
#endif
#if MATRIX_ROW_MAJOR
#define Mat4x4_get(self, x, y) (&((self)->data[(x) + ((y) << 2)]))
#define Mat4x4_at(self, x, y) ((self)->data[(x) + ((y) << 2)])
#else
#define Mat4x4_get(self, x, y) (&((self)->data[(y) + ((x) << 2)]))
#define Mat4x4_at(self, x, y) ((self)->data[(y) + ((x) << 2)])
#endif

typedef struct /*ALIGN_STRUCT(16)*/ Mat4x4
{
  float data[16];

} /*ALIGN_STRUCT(16)*/ Mat4x4;

// For Vulkan
// gl_Position.y = -gl_Position.y;
// gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;
// Alternative is to pre mult proj by :
//   |+1.0 +0.0 +0.0 +0.0|
//   |+0.0 -1.0 +0.0 +0.0|
//   |+0.0 +0.0 +1/2 +1/2|
//   |+0.0 +0.0 +0.0 +1.0|

BF_MATH_API void  Mat4x4_identity(Mat4x4* self);
BF_MATH_API void  Mat4x4_initTranslatef(Mat4x4* self, float x, float y, float z);
BF_MATH_API void  Mat4x4_initScalef(Mat4x4* self, float x, float y, float z);
BF_MATH_API void  Mat4x4_initRotationf(Mat4x4* self, float x, float y, float z);
BF_MATH_API void  Mat4x4_ortho(Mat4x4* self, float left, float right, float bottom, float top, float near, float far);
BF_MATH_API void  Mat4x4_orthoVk(Mat4x4* self, float left, float right, float bottom, float top, float near, float far);
BF_MATH_API void  Mat4x4_perspective(Mat4x4* self, float fovDeg, float aspect, float near, float far);
BF_MATH_API void  Mat4x4_perspectiveVk(Mat4x4* self, float fovDeg, float aspect, float near, float far);
BF_MATH_API void  Mat4x4_frustum(Mat4x4* const self, float left, float right, float bottom, float top, float znear, float zfar);
BF_MATH_API void  Mat4x4_perspectiveInfinity(Mat4x4* const self, const float fovDeg, const float aspect, const float near);
BF_MATH_API void  Mat4x4_initLookAt(Mat4x4* self, const Vec3f* position, const Vec3f* target, const Vec3f* inUp);
BF_MATH_API void  Mat4x4_copy(const Mat4x4* self, Mat4x4* outCopy);
BF_MATH_API void  Mat4x4_transpose(Mat4x4* self);
BF_MATH_API int   Mat4x4_inverse(const Mat4x4* self, Mat4x4* outInverse);
BF_MATH_API float Mat4x4_det(const Mat4x4* self);
BF_MATH_API float Mat4x4_trace(const Mat4x4* self);
// The order is [self * other] which means other happens 'first'.
// Ex: 'out = self * other'
BF_MATH_API void Mat4x4_mult(const Mat4x4* self, const Mat4x4* other, Mat4x4* out);
BF_MATH_API void Mat4x4_multVec(const Mat4x4* self, const Vec3f* vec, Vec3f* outVec);

// New API

//
// index must be in the range [0, 4)
//
inline Vec4f bfMat4x4_row(const Mat4x4* self, int row_index)
{
  Vec4f result;

  result.x = Mat4x4_at(self, 0, row_index);
  result.y = Mat4x4_at(self, 1, row_index);
  result.z = Mat4x4_at(self, 2, row_index);
  result.w = Mat4x4_at(self, 3, row_index);

  return result;
}
//
// Returns the absolute value of each element.
//
inline Mat4x4 bfMat4x4f_abs(const Mat4x4* self)
{
  Mat4x4 result;

  for (int i = 0; i < 16; ++i)
  {
    const float value = self->data[i];

    result.data[i] = value < 0.0f ? -value : value;
  }

  return result;
}

#if __cplusplus
}
#endif

#endif /* mat4x4_h */
