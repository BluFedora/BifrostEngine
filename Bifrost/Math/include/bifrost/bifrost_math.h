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

static inline uint32_t bfColor4u_toUint32(bfColor4u color)
{
  return (color.r << 0) | (color.g << 8) | (color.b << 16) | (color.a << 24);
}

static inline bfColor4u bfColor4u_fromUint32(uint32_t color)
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

static inline float bfMathInvLerpf(float min, float max, float value)
{
  return (value - min) / (max - value);
}

static inline float bfMathRemapf(float old_min, float old_max, float new_min, float new_max, float value)
{
  return bfMathLerpf(new_min, new_max, bfMathInvLerpf(old_min, old_max, value));
}
#if __cplusplus
}
#endif

#endif /* BIFROST_MATH_H */
