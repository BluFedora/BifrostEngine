  //
  //  vec3f.c
  //  OpenGL
  //
  //  Created by Shareef Raheem on 12/15/17.
  //  Copyright © 2017 Shareef Raheem. All rights reserved.
  //
#include "bifrost/math/bifrost_vec3.h"
#include "bifrost/math/bifrost_mat4x4.h"

#include <string.h> /* memcmp */
#include <math.h>   /* sqrt   */

#if VECTOR_SSE
#include <xmmintrin.h>
#endif

void Vec3f_set(Vec3f* self, float x, float y, float z, float w)
{
  self->x = x;
  self->y = y;
  self->z = z;
  self->w = w;
}

void Vec3f_copy(Vec3f* self, const Vec3f* other)
{
  (*self) = (*other);
}

int Vec3f_isEqual(const Vec3f* self, const Vec3f* other)
{
  return memcmp(self, other, sizeof(Vec3f)) == 0;
}

void Vec3f_add(Vec3f* self, const Vec3f* other)
{
#if VECTOR_SSE
  __m128 lhs_data = _mm_set_ps(self->w, self->z, self->y, self->x);
  __m128 rhs_data = _mm_set_ps(0.0f, other->z, other->y, other->x);
  __m128 result   = _mm_add_ps(lhs_data, rhs_data);

  _mm_storeu_ps((float*)self, result);
#else
  self->x += other->x;
  self->y += other->y;
  self->z += other->z;
#endif
}

void Vec3f_addScaled(Vec3f* self, const Vec3f* other, const float factor)
{
#if VECTOR_SSE
  __m128 lhs_data = _mm_loadu_ps((float*)self);
  __m128 rhs_data = _mm_set_ps(0.0f, factor, factor, factor);
  rhs_data        = _mm_mul_ps(_mm_loadu_ps((float*)other), rhs_data);
  __m128 result   = _mm_add_ps(lhs_data, rhs_data);

  _mm_storeu_ps((float*)self, result);
#else
  self->x += other->x * factor;
  self->y += other->y * factor;
  self->z += other->z * factor;
#endif
}

void Vec3f_sub(Vec3f* self, const Vec3f* other)
{
#if VECTOR_SSE
  __m128 lhs_data = _mm_set_ps(self->w, self->z, self->y, self->x);
  __m128 rhs_data = _mm_set_ps(0.0f, other->z, other->y, other->x);
  __m128 result   = _mm_sub_ps(lhs_data, rhs_data);

  _mm_storeu_ps((float*)self, result);
#else
  self->x -= other->x;
  self->y -= other->y;
  self->z -= other->z;
#endif
}

void Vec3f_mul(Vec3f* self, const float scalar)
{
#if VECTOR_SSE
    // for aligned use : _mm_load_ps
  __m128 lhs_data = _mm_loadu_ps((float*)self);
  __m128 rhs_data = _mm_set_ps(1.0f, scalar, scalar, scalar);
  __m128 result   = _mm_mul_ps(lhs_data, rhs_data);
    // for aligned use : _mm_store_ps
  _mm_storeu_ps((float*)self, result);
#else
  self->x *= scalar;
  self->y *= scalar;
  self->z *= scalar;
#endif
}

void Vec3f_div(Vec3f* self, const float scalar)
{
  if (scalar != 1.0f)
    Vec3f_mul(self, scalar == 0.0f ? 0.0f : 1.0f / scalar);
}

float Vec3f_lenSq(const Vec3f* self)
{
  return Vec3f_dot(self, self);
}

float Vec3f_len(const Vec3f* self)
{
  return sqrtf(Vec3f_lenSq(self));
}

void Vec3f_normalize(Vec3f* self)
{
  const float len = Vec3f_lenSq(self);

  if (len > 0.00001f)
  {
    Vec3f_div(self, sqrtf(len));
  }
}

float Vec3f_dot(const Vec3f* self, const Vec3f* other)
{
  return self->x * other->x + self->y * other->y + self->z * other->z;
}

void Vec3f_cross(const Vec3f* self, const Vec3f* other, Vec3f* output)
{
  /*
   [2 * 3 - 3 * 2]
   [3 * 1 - 1 * 3]
   [1 * 2 - 2 * 1]
   */
  const float x = (self->y * other->z) - (self->z * other->y);
  const float y = (self->z * other->x) - (self->x * other->z);
  const float z = (self->x * other->y) - (self->y * other->x);

  output->x = x;
  output->y = y;
  output->z = z;
}

void Vec3f_mulMat(Vec3f* self, const Mat4x4* matrix)
{
  Mat4x4_multVec(matrix, self, self);
}

#define MUL_AND_SHIFT(c, s) ((uint)((c) * 255.0f) << (s))

Color Vec3f_toColor(const Vec3f* self)
{
  return  MUL_AND_SHIFT(self->x,  0) |
          MUL_AND_SHIFT(self->y,  8) |
          MUL_AND_SHIFT(self->z, 16) |
          MUL_AND_SHIFT(self->w, 24);
}

#undef MUL_AND_SHIFT

uchar Color_r(const Color self)
{
  return (uchar)(((0xFFu << 0) & self) >> 0);
}

uchar Color_g(const Color self)
{
  return (uchar)(((0xFFu << 8) & self) >> 8);
}

uchar Color_b(const Color self)
{
  return (uchar)(((0xFFu << 16) & self) >> 16);
}

uchar Color_a(const Color self)
{
  return (uchar)(((0xFFu << 24) & self) >> 24);
}

void Color_setRGBA(Color* self, const uchar r, const uchar g, const uchar b, const uchar a)
{
  (*self) = (r << 0) | (g << 8) | (b << 16) | (a << 24);
}

void Color_setR(Color* self, const uint r)
{
  (*self) = ((*self) & ~(0xFFu << 0)) | (r << 0);
}

void Color_setG(Color* self, const uint g)
{
  (*self) = ((*self) & ~(0xFFu << 8)) | (g << 8);
}

void Color_setB(Color* self, const uint b)
{
  (*self) = ((*self) & ~(0xFFu << 16)) | (b << 16);
}

void Color_setA(Color* self, const uint a)
{
  (*self) = ((*self) & ~(0xFFu << 24)) | (a << 24);
}
