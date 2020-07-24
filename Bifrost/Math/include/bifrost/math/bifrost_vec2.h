#pragma once

#include "bifrost_math_export.h"

#if __cplusplus
extern "C" {
#endif
typedef struct Vec2f_t
{
  float x;
  float y;

} Vec2f;

typedef struct Vec2i_t
{
  int x;
  int y;

} Vec2i;

BIFROST_MATH_API void  Vec2f_set(Vec2f* self, const float x, const float y);
BIFROST_MATH_API void  Vec2f_copy(Vec2f* const self, const Vec2f* const src);
BIFROST_MATH_API void  Vec2f_addScaled(Vec2f* self, const Vec2f* other, const float dt);
BIFROST_MATH_API void  Vec2f_add(Vec2f* self, const Vec2f* other);
BIFROST_MATH_API void  Vec2f_sub(Vec2f* self, const Vec2f* other);
BIFROST_MATH_API void  Vec2f_subScaled(Vec2f* self, const Vec2f* other, const float dt);
BIFROST_MATH_API void  Vec2f_normalize(Vec2f* self);
BIFROST_MATH_API void  Vec2f_multScalar(Vec2f* self, const float factor);
BIFROST_MATH_API float Vec2f_dot(const Vec2f* v1, const Vec2f* v2);
BIFROST_MATH_API float Vec2f_lenSq(const Vec2f* const v1);
BIFROST_MATH_API float Vec2f_len(const Vec2f* v1);
BIFROST_MATH_API float Vec2f_cross(const Vec2f* lhs, const Vec2f* rhs);

// 'static' functions
BIFROST_MATH_API Vec2f vec2f(const float x, const float y);
BIFROST_MATH_API Vec2f vec2f_sub(const Vec2f* const self, const Vec2f* const other);
BIFROST_MATH_API Vec2f vec2f_add(const Vec2f* const self, const Vec2f* const other);
#if __cplusplus
}
#endif
