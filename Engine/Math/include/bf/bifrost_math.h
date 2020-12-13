#ifndef BIFROST_MATH_H
#define BIFROST_MATH_H

#include "math/bifrost_camera.h"
#include "math/bifrost_mat4x4.h"
#include "math/bifrost_math.h"
#include "math/bifrost_transform.h"
#include "math/bifrost_vec2.h"
#include "math/bifrost_vec3.h"

#include <stdint.h> /* uint8_t */

#if __cplusplus
extern "C" {
#endif
typedef struct bfColor4f_t
{
  float r;
  float g;
  float b;
  float a;

} bfColor4f;

typedef struct bfColor4u_t
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

} bfColor4u;

typedef uint32_t bfColor32u;

static inline bfColor32u bfColor4u_toUint32(bfColor4u color)
{
  return (color.r << 0) | (color.g << 8) | (color.b << 16) | (color.a << 24);
}

static inline bfColor4u bfColor4u_fromUint32(bfColor32u color)
{
  bfColor4u ret;

  ret.r = Color_r(color);
  ret.g = Color_g(color);
  ret.b = Color_b(color);
  ret.a = Color_a(color);

  return ret;
}

static inline float bfMathAlignf(float value, float size)
{
  return floorf(value / size) * size;
}

static inline float bfMathLerpf(float a, float b, float t)
{
  return (1.0f - t) * a + t * b;
}

static inline bfColor4f bfMathLerpColor4f(bfColor4f a, bfColor4f b, float t)
{
  bfColor4f result;

  result.r = bfMathLerpf(a.r, b.r, t);
  result.g = bfMathLerpf(a.g, b.g, t);
  result.b = bfMathLerpf(a.b, b.b, t);
  result.a = bfMathLerpf(a.a, b.a, t);

  return result;
}

static inline bfColor4u bfMathLerpColor4u(bfColor4u a, bfColor4u b, float t)
{
  bfColor4u result;

  result.r = (uint8_t)bfMathLerpf((float)a.r, (float)b.r, t);
  result.g = (uint8_t)bfMathLerpf((float)a.g, (float)b.g, t);
  result.b = (uint8_t)bfMathLerpf((float)a.b, (float)b.b, t);
  result.a = (uint8_t)bfMathLerpf((float)a.a, (float)b.a, t);

  return result;
}

static inline float bfMathInvLerpf(float min, float max, float value)
{
  return (value - min) / (max - min);
}

static inline float bfMathRemapf(float old_min, float old_max, float new_min, float new_max, float value)
{
  return bfMathLerpf(new_min, new_max, bfMathInvLerpf(old_min, old_max, value));
}
#if __cplusplus
}
#endif

#endif /* BIFROST_MATH_H */
