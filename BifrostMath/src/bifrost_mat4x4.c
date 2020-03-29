//
//  mat4x4.c
//  OpenGL
//
//  Created by Shareef Raheem on 12/17/17.
//  Copyright © 2017 Shareef Raheem. All rights reserved.
//
#include "bifrost/math/bifrost_mat4x4.h"

#include "bifrost/math/bifrost_vec3.h"

#include <math.h>   /* cosf, sinf tanf  */
#include <string.h> /* memcpy           */

#if MATRIX_SSE
#include <xmmintrin.h>
#endif

#ifndef M_PI
#define M_PI 3.14159f
#endif

#define DEG_TO_RAD_f (float)(M_PI / 180.0f)

/*
float* Mat4x4_get(Mat4x4* self, const uint x, const uint y)
{
    // ROW MAJOR: [ 0,  1,  2,  3,
    //              4,  5,  6,  7,
    //              8,  9, 10, 11,
    //             12, 13, 14, 15]

    // COL MAJOR: [ 0,  4,  8, 12,
    //              1,  5,  9, 13,
    //              2,  6, 10, 14,
    //              3,  7, 11, 15]

#if MATRIX_ROW_MAJOR == 1
  return &(self->data[x + (y << 2)]);
#else
  return &(self->data[y + (x << 2)]);
#endif
}
*/

void Mat4x4_identity(Mat4x4* self)
{
  (*Mat4x4_get(self, 0, 0)) = 1.0f;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = 0.0f;
  (*Mat4x4_get(self, 3, 0)) = 0.0f;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = 1.0f;
  (*Mat4x4_get(self, 2, 1)) = 0.0f;
  (*Mat4x4_get(self, 3, 1)) = 0.0f;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = 1.0f;
  (*Mat4x4_get(self, 3, 2)) = 0.0f;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = 0.0f;
  (*Mat4x4_get(self, 3, 3)) = 1.0f;
}

void Mat4x4_initTranslatef(Mat4x4* self, float x, float y, float z)
{
  (*Mat4x4_get(self, 0, 0)) = 1.0f;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = 0.0f;
  (*Mat4x4_get(self, 3, 0)) = x;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = 1.0f;
  (*Mat4x4_get(self, 2, 1)) = 0.0f;
  (*Mat4x4_get(self, 3, 1)) = y;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = 1.0f;
  (*Mat4x4_get(self, 3, 2)) = z;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = 0.0f;
  (*Mat4x4_get(self, 3, 3)) = 1.0f;
}

void Mat4x4_initScalef(Mat4x4* self, float x, float y, float z)
{
  (*Mat4x4_get(self, 0, 0)) = x;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = 0.0f;
  (*Mat4x4_get(self, 3, 0)) = 0.0f;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = y;
  (*Mat4x4_get(self, 2, 1)) = 0.0f;
  (*Mat4x4_get(self, 3, 1)) = 0.0f;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = z;
  (*Mat4x4_get(self, 3, 2)) = 0.0f;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = 0.0f;
  (*Mat4x4_get(self, 3, 3)) = 1.0f;
}

void Mat4x4_initRotationf(Mat4x4* self, float x, float y, float z)
{
  Mat4x4_identity(self);

  if (z != 0.0f)
  {
    Mat4x4 rot_z;

    z *= DEG_TO_RAD_f;

    /* rot z */
    (*Mat4x4_get(&rot_z, 0, 0)) = cosf(z);
    (*Mat4x4_get(&rot_z, 1, 0)) = sinf(z);
    (*Mat4x4_get(&rot_z, 2, 0)) = 0.0f;
    (*Mat4x4_get(&rot_z, 3, 0)) = 0.0f;

    (*Mat4x4_get(&rot_z, 0, 1)) = -sinf(z);
    (*Mat4x4_get(&rot_z, 1, 1)) = cosf(z);
    (*Mat4x4_get(&rot_z, 2, 1)) = 0.0f;
    (*Mat4x4_get(&rot_z, 3, 1)) = 0.0f;

    (*Mat4x4_get(&rot_z, 0, 2)) = 0.0f;
    (*Mat4x4_get(&rot_z, 1, 2)) = 0.0f;
    (*Mat4x4_get(&rot_z, 2, 2)) = 1.0f;
    (*Mat4x4_get(&rot_z, 3, 2)) = 0.0f;

    (*Mat4x4_get(&rot_z, 0, 3)) = 0.0f;
    (*Mat4x4_get(&rot_z, 1, 3)) = 0.0f;
    (*Mat4x4_get(&rot_z, 2, 3)) = 0.0f;
    (*Mat4x4_get(&rot_z, 3, 3)) = 1.0f;

    Mat4x4_mult(&rot_z, self, self);
  }

  if (y != 0.0f)
  {
    Mat4x4 rot_y;

    y *= DEG_TO_RAD_f;

    /* rot y */
    (*Mat4x4_get(&rot_y, 0, 0)) = cosf(y);
    (*Mat4x4_get(&rot_y, 1, 0)) = 0.0f;
    (*Mat4x4_get(&rot_y, 2, 0)) = sinf(y);
    (*Mat4x4_get(&rot_y, 3, 0)) = 0.0f;

    (*Mat4x4_get(&rot_y, 0, 1)) = 0.0f;
    (*Mat4x4_get(&rot_y, 1, 1)) = 1.0f;
    (*Mat4x4_get(&rot_y, 2, 1)) = 0.0f;
    (*Mat4x4_get(&rot_y, 3, 1)) = 0.0f;

    (*Mat4x4_get(&rot_y, 0, 2)) = -sinf(y);
    (*Mat4x4_get(&rot_y, 1, 2)) = 0.0f;
    (*Mat4x4_get(&rot_y, 2, 2)) = cosf(y);
    (*Mat4x4_get(&rot_y, 3, 2)) = 0.0f;

    (*Mat4x4_get(&rot_y, 0, 3)) = 0.0f;
    (*Mat4x4_get(&rot_y, 1, 3)) = 0.0f;
    (*Mat4x4_get(&rot_y, 2, 3)) = 0.0f;
    (*Mat4x4_get(&rot_y, 3, 3)) = 1.0f;

    Mat4x4_mult(&rot_y, self, self);
  }

  if (x != 0.0f)
  {
    Mat4x4 rot_x;

    x *= DEG_TO_RAD_f;

    /* rot x */
    (*Mat4x4_get(&rot_x, 0, 0)) = 1.0f;
    (*Mat4x4_get(&rot_x, 1, 0)) = 0.0f;
    (*Mat4x4_get(&rot_x, 2, 0)) = 0.0f;
    (*Mat4x4_get(&rot_x, 3, 0)) = 0.0f;

    (*Mat4x4_get(&rot_x, 0, 1)) = 0.0f;
    (*Mat4x4_get(&rot_x, 1, 1)) = cosf(x);
    (*Mat4x4_get(&rot_x, 2, 1)) = sinf(x);
    (*Mat4x4_get(&rot_x, 3, 1)) = 0.0f;

    (*Mat4x4_get(&rot_x, 0, 2)) = 0.0f;
    (*Mat4x4_get(&rot_x, 1, 2)) = -sinf(x);
    (*Mat4x4_get(&rot_x, 2, 2)) = cosf(x);
    (*Mat4x4_get(&rot_x, 3, 2)) = 0.0f;

    (*Mat4x4_get(&rot_x, 0, 3)) = 0.0f;
    (*Mat4x4_get(&rot_x, 1, 3)) = 0.0f;
    (*Mat4x4_get(&rot_x, 2, 3)) = 0.0f;
    (*Mat4x4_get(&rot_x, 3, 3)) = 1.0f;

    Mat4x4_mult(&rot_x, self, self);
  }
}

void Mat4x4_ortho(Mat4x4* self, float left, float right, float bottom, float top, float near, float far)
{
  const float rminusl = right - left;
  const float tminusb = top - bottom;
  const float fminusn = far - near;

  (*Mat4x4_get(self, 0, 0)) = 2.0f / rminusl;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = 0.0f;
  (*Mat4x4_get(self, 3, 0)) = -(right + left) / rminusl;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = 2.0f / tminusb;
  (*Mat4x4_get(self, 2, 1)) = 0.0f;
  (*Mat4x4_get(self, 3, 1)) = -(top + bottom) / tminusb;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = -2.0f / fminusn;
  (*Mat4x4_get(self, 3, 2)) = -(far + near) / fminusn;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = 0.0f;
  (*Mat4x4_get(self, 3, 3)) = 1.0f;
}

void Mat4x4_orthoVk(Mat4x4* self, float left, float right, float bottom, float top, float near, float far)
{
  const float rminusl = right - left;
  const float bminust = bottom - top;
  const float nminusf = near - far;

  (*Mat4x4_get(self, 0, 0)) = 2.0f / rminusl;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = 0.0f;
  (*Mat4x4_get(self, 3, 0)) = -(right + left) / rminusl;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = 2.0f / bminust;
  (*Mat4x4_get(self, 2, 1)) = 0.0f;
  (*Mat4x4_get(self, 3, 1)) = -(top + bottom) / bminust;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = 1.0f / nminusf;
  (*Mat4x4_get(self, 3, 2)) = near / nminusf;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = 0.0f;
  (*Mat4x4_get(self, 3, 3)) = 1.0f;
}

void Mat4x4_perspective(Mat4x4* self, float fovDeg, float aspect, float near, float far)
{
  const float top      = near * tanf((fovDeg * 0.5f) * DEG_TO_RAD_f);
  const float bottom   = -top;
  const float right    = top * aspect;
  const float left     = -right;
  const float fminusn  = far - near;
  const float tminusb  = top - bottom;
  const float rminusl  = right - left;
  const float two_near = 2.0f * near;

  (*Mat4x4_get(self, 0, 0)) = (two_near) / rminusl;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = (right + left) / rminusl;
  (*Mat4x4_get(self, 3, 0)) = 0.0f;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = (two_near) / tminusb;
  (*Mat4x4_get(self, 2, 1)) = (top + bottom) / tminusb;
  (*Mat4x4_get(self, 3, 1)) = 0.0f;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = -(far + near) / fminusn;
  (*Mat4x4_get(self, 3, 2)) = -(two_near * far) / fminusn;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = -1.0f;
  (*Mat4x4_get(self, 3, 3)) = 0.0f;
}

void Mat4x4_perspectiveVk(Mat4x4* self, float fovDeg, float aspect, float near, float far)
{
  const float f       = 1.0f / tanf((fovDeg * 0.5f) * DEG_TO_RAD_f);
  const float nminusf = near - far;

  (*Mat4x4_get(self, 0, 0)) = f / aspect;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = 0.0f;
  (*Mat4x4_get(self, 3, 0)) = 0.0f;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = -f;
  (*Mat4x4_get(self, 2, 1)) = 0.0f;
  (*Mat4x4_get(self, 3, 1)) = 0.0f;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = far / nminusf;
  (*Mat4x4_get(self, 3, 2)) = (near * far) / nminusf;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = -1.0f;
  (*Mat4x4_get(self, 3, 3)) = 0.0f;
}

void Mat4x4_frustum(Mat4x4* const self, float left, float right, float bottom, float top, float znear, float zfar)
{
  const float temp  = 2.0f * znear;
  const float temp2 = 1.0f / (right - left);
  const float temp3 = 1.0f / (top - bottom);
  const float temp4 = 1.0f / (zfar - znear);

  // TODO(Shareef): This code is Column Major, I need to convert it to
  // neither row or col major
  (*Mat4x4_get(self, 0, 0)) = temp * temp2;
  self->data[1]             = 0.0f;
  self->data[2]             = 0.0f;
  self->data[3]             = 0.0f;
  self->data[4]             = 0.0f;
  self->data[5]             = temp * temp3;
  self->data[6]             = 0.0f;
  self->data[7]             = 0.0f;
  self->data[8]             = (right + left) * temp2;
  self->data[9]             = (top + bottom) * temp3;
  self->data[10]            = (-zfar - znear) * temp4;
  self->data[11]            = -1.0f;
  self->data[12]            = 0.0f;
  self->data[13]            = 0.0f;
  self->data[14]            = (-temp * zfar) * temp4;
  (*Mat4x4_get(self, 3, 3)) = 0.0f;

#if MATRIX_ROW_MAJOR
  Mat4x4_transpose(self);
#endif
}

// This is the matrix generated by calculating what happens as far => infinity
void Mat4x4_perspectiveInfinity(Mat4x4* const self, const float fovDeg, const float aspect, const float near)
{
  const float top      = near * tanf((fovDeg * 0.5f) * DEG_TO_RAD_f);
  const float bottom   = -top;
  const float right    = top * aspect;
  const float left     = -right;
  const float tminusb  = top - bottom;
  const float rminusl  = right - left;
  const float two_near = 2.0f * near;

  (*Mat4x4_get(self, 0, 0)) = (two_near) / rminusl;
  (*Mat4x4_get(self, 1, 0)) = 0.0f;
  (*Mat4x4_get(self, 2, 0)) = (right + left) / rminusl;
  (*Mat4x4_get(self, 3, 0)) = 0.0f;

  (*Mat4x4_get(self, 0, 1)) = 0.0f;
  (*Mat4x4_get(self, 1, 1)) = (two_near) / tminusb;
  (*Mat4x4_get(self, 2, 1)) = (top + bottom) / tminusb;
  (*Mat4x4_get(self, 3, 1)) = 0.0f;

  (*Mat4x4_get(self, 0, 2)) = 0.0f;
  (*Mat4x4_get(self, 1, 2)) = 0.0f;
  (*Mat4x4_get(self, 2, 2)) = -1.0f;
  (*Mat4x4_get(self, 3, 2)) = -two_near;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = -1.0f;
  (*Mat4x4_get(self, 3, 3)) = 0.0f;
}

void Mat4x4_initLookAt(Mat4x4* self, const Vec3f* position, const Vec3f* target, const Vec3f* inUp)
{
  Vec3f forward, left, up;

  Vec3f_copy(&forward, position);
  Vec3f_sub(&forward, target);
  Vec3f_normalize(&forward);

  Vec3f_cross(inUp, &forward, &left);
  Vec3f_normalize(&left);

  Vec3f_cross(&forward, &left, &up);
  Vec3f_normalize(&up);

  (*Mat4x4_get(self, 0, 0)) = left.x;
  (*Mat4x4_get(self, 1, 0)) = left.y;
  (*Mat4x4_get(self, 2, 0)) = left.z;

  (*Mat4x4_get(self, 0, 1)) = up.x;
  (*Mat4x4_get(self, 1, 1)) = up.y;
  (*Mat4x4_get(self, 2, 1)) = up.z;

  (*Mat4x4_get(self, 0, 2)) = forward.x;
  (*Mat4x4_get(self, 1, 2)) = forward.y;
  (*Mat4x4_get(self, 2, 2)) = forward.z;

  (*Mat4x4_get(self, 0, 3)) = 0.0f;
  (*Mat4x4_get(self, 1, 3)) = 0.0f;
  (*Mat4x4_get(self, 2, 3)) = 0.0f;

  (*Mat4x4_get(self, 3, 0)) = -left.x * position->x - left.y * position->y - left.z * position->z;
  (*Mat4x4_get(self, 3, 1)) = -up.x * position->x - up.y * position->y - up.z * position->z;
  (*Mat4x4_get(self, 3, 2)) = -forward.x * position->x - forward.y * position->y - forward.z * position->z;
  (*Mat4x4_get(self, 3, 3)) = 1.0f;
}

void Mat4x4_copy(const Mat4x4* self, Mat4x4* outCopy)
{
  memcpy(outCopy, self, sizeof(Mat4x4));
}

static void swap(float* x0, float* x1)
{
  const float temp = (*x0);
  (*x0)            = (*x1);
  (*x1)            = temp;
}

void Mat4x4_transpose(Mat4x4* self)
{
  // GCC 8.1 doesn't know how to unroll the loop but clang 3.5+ does.
  // The if statement doesn't matter since if the compiler can unroll
  // then the if is optimized out regardless.
  // ICC 18.0.0 produces the weirdest results when using the loop. (Also can't unroll)
  // GCC Produces the best code when using the optimized version (and -O2)
  // Then Clang, with ICC being the worst.
#if 0
  for (uint y = 0; y < 4; ++y)
  {
    for (uint x = (y + 1); x < 4; ++x)
    {
      /*if (x == y) continue;*/ 
      /* when unrolling the loop would the compiler optimize this condition? YES. */
      swap(Mat4x4_get(self, x, y), Mat4x4_get(self, y, x));
    }
  }
#else
  swap(&self->data[1], &self->data[4]);
  swap(&self->data[2], &self->data[8]);
  swap(&self->data[3], &self->data[12]);
  swap(&self->data[6], &self->data[9]);
  swap(&self->data[7], &self->data[13]);
  swap(&self->data[11], &self->data[14]);
#endif
}

int Mat4x4_inverse(const Mat4x4* self, Mat4x4* outInverse)
{
  const float* const m = self->data;
  float              inv[16];

  inv[0]  = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
  inv[4]  = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
  inv[8]  = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
  inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
  inv[1]  = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
  inv[5]  = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
  inv[9]  = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
  inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
  inv[2]  = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
  inv[6]  = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
  inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
  inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
  inv[3]  = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
  inv[7]  = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
  inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
  inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

  float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  if (det == 0)
    return 0;

  det = 1.0f / det;

  for (uint i = 0; i < 16; ++i)
    outInverse->data[i] = inv[i] * det;

  return 1;
}

static float det_2x2(float a, float b, float c, float d)
{
  return (a * d) - (b * c);
}

static float det_3x3(float a, float b, float c, float d, float e, float f, float g, float h, float i)
{
  return a * det_2x2(e, f, h, i) - b * det_2x2(d, f, g, i) + c * det_2x2(d, e, g, h);
}

float Mat4x4_det(const Mat4x4* self)
{
  const float det_a = Mat4x4_at(self, 0, 0) * det_3x3(Mat4x4_at(self, 1, 1), Mat4x4_at(self, 2, 1), Mat4x4_at(self, 3, 1), Mat4x4_at(self, 1, 2), Mat4x4_at(self, 2, 2), Mat4x4_at(self, 3, 2), Mat4x4_at(self, 1, 3), Mat4x4_at(self, 2, 3), Mat4x4_at(self, 3, 3));
  const float det_b = Mat4x4_at(self, 1, 0) * det_3x3(Mat4x4_at(self, 0, 1), Mat4x4_at(self, 2, 1), Mat4x4_at(self, 3, 1), Mat4x4_at(self, 0, 2), Mat4x4_at(self, 2, 2), Mat4x4_at(self, 3, 2), Mat4x4_at(self, 0, 3), Mat4x4_at(self, 2, 3), Mat4x4_at(self, 3, 3));
  const float det_c = Mat4x4_at(self, 2, 0) * det_3x3(Mat4x4_at(self, 0, 1), Mat4x4_at(self, 1, 1), Mat4x4_at(self, 3, 1), Mat4x4_at(self, 0, 2), Mat4x4_at(self, 1, 2), Mat4x4_at(self, 3, 2), Mat4x4_at(self, 0, 3), Mat4x4_at(self, 1, 3), Mat4x4_at(self, 3, 3));
  const float det_d = Mat4x4_at(self, 3, 0) * det_3x3(Mat4x4_at(self, 0, 1), Mat4x4_at(self, 1, 1), Mat4x4_at(self, 2, 1), Mat4x4_at(self, 0, 2), Mat4x4_at(self, 1, 2), Mat4x4_at(self, 2, 2), Mat4x4_at(self, 0, 3), Mat4x4_at(self, 1, 3), Mat4x4_at(self, 2, 3));

  return det_a - det_b + det_c - det_d;
}

float Mat4x4_trace(const Mat4x4* self)
{
  return Mat4x4_at(self, 0, 0) + Mat4x4_at(self, 1, 1) + Mat4x4_at(self, 2, 2) + Mat4x4_at(self, 3, 3);
}

void Mat4x4_mult(const Mat4x4* self, const Mat4x4* other, Mat4x4* out)
{
#if MATRIX_SSE
  // 12 Multiplications
  // 12 Additions
#if MATRIX_ROW_MAJOR
  const float* const lhs = self->data;
  const float* const rhs = other->data;
#else
  // Reverse The Multiplication for Column Major
  const float* const rhs       = self->data;
  const float* const lhs       = other->data;
#endif

  //  __m128 lhs_row_0 = _mm_loadu_ps(lhs + 0);
  //  __m128 lhs_row_1 = _mm_loadu_ps(lhs + 4);
  //  __m128 lhs_row_2 = _mm_loadu_ps(lhs + 8);
  //  __m128 lhs_row_3 = _mm_loadu_ps(lhs + 12);
  const __m128 rhs_row_0 = _mm_load_ps(rhs + 0);
  const __m128 rhs_row_1 = _mm_load_ps(rhs + 4);
  const __m128 rhs_row_2 = _mm_load_ps(rhs + 8);
  const __m128 rhs_row_3 = _mm_load_ps(rhs + 12);

  //  __m128 lhs_col_0 = _mm_setr_ps(lhs[0], lhs[4], lhs[8],  lhs[12]);
  //  __m128 lhs_col_1 = _mm_setr_ps(lhs[1], lhs[5], lhs[9],  lhs[13]);
  //  __m128 lhs_col_2 = _mm_setr_ps(lhs[2], lhs[6], lhs[10], lhs[14]);
  //  __m128 lhs_col_3 = _mm_setr_ps(lhs[3], lhs[7], lhs[11], lhs[15]);
  //  __m128 rhs_col_0 = _mm_setr_ps(rhs[0], rhs[4], rhs[8],  rhs[12]);
  //  __m128 rhs_col_1 = _mm_setr_ps(rhs[1], rhs[5], rhs[9],  rhs[13]);
  //  __m128 rhs_col_2 = _mm_setr_ps(rhs[2], rhs[6], rhs[10], rhs[14]);
  //  __m128 rhs_col_3 = _mm_setr_ps(rhs[3], rhs[7], rhs[11], rhs[15]);

  const float x1 = lhs[0];
  const float y1 = lhs[1];
  const float z1 = lhs[2];
  const float w1 = lhs[3];

  const float x2 = lhs[4];
  const float y2 = lhs[5];
  const float z2 = lhs[6];
  const float w2 = lhs[7];

  const float x3 = lhs[8];
  const float y3 = lhs[9];
  const float z3 = lhs[10];
  const float w3 = lhs[11];

  const float x4 = lhs[12];
  const float y4 = lhs[13];
  const float z4 = lhs[14];
  const float w4 = lhs[15];

  // (x1 * a1) + (y1 * a2) + (z1 * a3) + (w1 * a4)
  // (x1 * b1) + (y1 * b2) + (z1 * b3) + (w1 * b4)
  // (x1 * c1) + (y1 * c2) + (z1 * c3) + (w1 * c4)
  // (x1 * d1) + (y1 * d2) + (z1 * d3) + (w1 * d4)
  __m128 new_row_0 = _mm_mul_ps(rhs_row_0, _mm_set1_ps(x1));
  new_row_0        = _mm_add_ps(new_row_0, _mm_mul_ps(rhs_row_1, _mm_set1_ps(y1)));
  new_row_0        = _mm_add_ps(new_row_0, _mm_mul_ps(rhs_row_2, _mm_set1_ps(z1)));
  new_row_0        = _mm_add_ps(new_row_0, _mm_mul_ps(rhs_row_3, _mm_set1_ps(w1)));

  __m128 new_row_1 = _mm_mul_ps(rhs_row_0, _mm_set1_ps(x2));
  new_row_1        = _mm_add_ps(new_row_1, _mm_mul_ps(rhs_row_1, _mm_set1_ps(y2)));
  new_row_1        = _mm_add_ps(new_row_1, _mm_mul_ps(rhs_row_2, _mm_set1_ps(z2)));
  new_row_1        = _mm_add_ps(new_row_1, _mm_mul_ps(rhs_row_3, _mm_set1_ps(w2)));

  __m128 new_row_2 = _mm_mul_ps(rhs_row_0, _mm_set1_ps(x3));
  new_row_2        = _mm_add_ps(new_row_2, _mm_mul_ps(rhs_row_1, _mm_set1_ps(y3)));
  new_row_2        = _mm_add_ps(new_row_2, _mm_mul_ps(rhs_row_2, _mm_set1_ps(z3)));
  new_row_2        = _mm_add_ps(new_row_2, _mm_mul_ps(rhs_row_3, _mm_set1_ps(w3)));

  __m128 new_row_3 = _mm_mul_ps(rhs_row_0, _mm_set1_ps(x4));
  new_row_3        = _mm_add_ps(new_row_3, _mm_mul_ps(rhs_row_1, _mm_set1_ps(y4)));
  new_row_3        = _mm_add_ps(new_row_3, _mm_mul_ps(rhs_row_2, _mm_set1_ps(z4)));
  new_row_3        = _mm_add_ps(new_row_3, _mm_mul_ps(rhs_row_3, _mm_set1_ps(w4)));

  _mm_storeu_ps(out->data + 0, new_row_0);
  _mm_storeu_ps(out->data + 4, new_row_1);
  _mm_storeu_ps(out->data + 8, new_row_2);
  _mm_storeu_ps(out->data + 12, new_row_3);
#else
  // 64 Multiplications
  // 48 Additions

  Mat4x4 temp;

  for (uint cols = 0; cols < 4; ++cols)
  {
    const float col0 = Mat4x4_at(other, cols, 0);
    const float col1 = Mat4x4_at(other, cols, 1);
    const float col2 = Mat4x4_at(other, cols, 2);
    const float col3 = Mat4x4_at(other, cols, 3);

    for (uint rows = 0; rows < 4; ++rows)
    {
      const float row0 = Mat4x4_at(self, 0, rows);
      const float row1 = Mat4x4_at(self, 1, rows);
      const float row2 = Mat4x4_at(self, 2, rows);
      const float row3 = Mat4x4_at(self, 3, rows);

      (*Mat4x4_get(&temp, cols, rows)) = (col0 * row0) + (col1 * row1) + (col2 * row2) + (col3 * row3);
    }
  }

  Mat4x4_copy(&temp, out);
#endif
}

void Mat4x4_multVec(const Mat4x4* self, const Vec3f* vec, Vec3f* outVec)
{
#if MATRIX_SSE
  // 4 Multiplications
  // 3 Additions
  const float* const lhs = self->data;

#if MATRIX_ROW_MAJOR
  __m128 lhs_col_0 = _mm_setr_ps(lhs[0], lhs[4], lhs[8], lhs[12]);
  __m128 lhs_col_1 = _mm_setr_ps(lhs[1], lhs[5], lhs[9], lhs[13]);
  __m128 lhs_col_2 = _mm_setr_ps(lhs[2], lhs[6], lhs[10], lhs[14]);
  __m128 lhs_col_3 = _mm_setr_ps(lhs[3], lhs[7], lhs[11], lhs[15]);
#else
  __m128             lhs_col_0 = _mm_load_ps(lhs + 0);
  __m128             lhs_col_1 = _mm_load_ps(lhs + 4);
  __m128             lhs_col_2 = _mm_load_ps(lhs + 8);
  __m128             lhs_col_3 = _mm_load_ps(lhs + 12);
#endif

  // (x1 * x) + (y1 * y) + (z1 * z) + (w1 * w) = new_x
  // (x2 * x) + (y2 * y) + (z2 * z) + (w2 * w) = new_y
  // (x3 * x) + (y3 * y) + (z3 * z) + (w3 * w) = new_z
  // (x4 * x) + (y4 * y) + (z4 * z) + (w4 * w) = new_w
  __m128 new_row_0 = _mm_mul_ps(lhs_col_0, _mm_set1_ps(vec->x));
  new_row_0        = _mm_add_ps(new_row_0, _mm_mul_ps(lhs_col_1, _mm_set1_ps(vec->y)));
  new_row_0        = _mm_add_ps(new_row_0, _mm_mul_ps(lhs_col_2, _mm_set1_ps(vec->z)));
  new_row_0        = _mm_add_ps(new_row_0, _mm_mul_ps(lhs_col_3, _mm_set1_ps(vec->w)));

  _mm_storeu_ps((float*)outVec, new_row_0);
#else
  // 16 Multiplications
  // 12 Additions
  const float vx = vec->x;
  const float vy = vec->y;
  const float vz = vec->z;
  const float vw = vec->w;

  outVec->x = (Mat4x4_at(self, 0, 0) * vx) + (Mat4x4_at(self, 1, 0) * vy) + (Mat4x4_at(self, 2, 0) * vz) + (Mat4x4_at(self, 3, 0) * vw);
  outVec->y = (Mat4x4_at(self, 0, 1) * vx) + (Mat4x4_at(self, 1, 1) * vy) + (Mat4x4_at(self, 2, 1) * vz) + (Mat4x4_at(self, 3, 1) * vw);
  outVec->z = (Mat4x4_at(self, 0, 2) * vx) + (Mat4x4_at(self, 1, 2) * vy) + (Mat4x4_at(self, 2, 2) * vz) + (Mat4x4_at(self, 3, 2) * vw);
  outVec->w = (Mat4x4_at(self, 0, 3) * vx) + (Mat4x4_at(self, 1, 3) * vy) + (Mat4x4_at(self, 2, 3) * vz) + (Mat4x4_at(self, 3, 3) * vw);
#endif
}
