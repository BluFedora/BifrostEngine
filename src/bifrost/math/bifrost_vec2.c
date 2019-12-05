#include "bifrost/math/bifrost_vec2.h"

#include <math.h> /* sqrtf */

void Vec2f_set(Vec2f* self, const float x, const float y)
{
  self->x = x;
  self->y = y;
}

void Vec2f_copy(Vec2f* const self, const Vec2f* const src)
{
  self->x = src->x;
  self->y = src->y;
}

void Vec2f_addScaled(Vec2f* self, const Vec2f* other, const float dt)
{
  self->x += other->x * dt;
  self->y += other->y * dt;
}

void Vec2f_add(Vec2f* self, const Vec2f* other)
{
  self->x += other->x;
  self->y += other->y;
}

void Vec2f_sub(Vec2f* self, const Vec2f* other)
{
  self->x -= other->x;
  self->y -= other->y;
}

void  Vec2f_subScaled(Vec2f* self, const Vec2f* other, const float dt)
{
  self->x -= other->x * dt;
  self->y -= other->y * dt;
}

void Vec2f_normalize(Vec2f *self)
{
  const float len = Vec2f_len(self);

  if (len != 1.0f && len != 0.0f) {
    Vec2f_multScalar(self, 1.0f / len);
  }
}

void Vec2f_multScalar(Vec2f *self, const float factor)
{
  self->x *= factor;
  self->y *= factor;
}

float Vec2f_dot(const Vec2f *v1, const Vec2f *v2)
{
  return (v1->x * v2->x) + (v1->y * v2->y);
}

float Vec2f_lenSq(const Vec2f* const v1)
{
  return Vec2f_dot(v1, v1);
}

float Vec2f_len(const Vec2f *v1)
{
  return sqrtf(Vec2f_lenSq(v1));
}

float Vec2f_cross(const Vec2f* lhs, const Vec2f* rhs)
{
  return (lhs->x * rhs->y) - (lhs->y * rhs->x);
}

Vec2f vec2f(const float x, const float y)
{
  return (Vec2f) { x, y };
}

Vec2f vec2f_sub(const Vec2f* const self, const Vec2f* const other)
{
  return (Vec2f) { self->x - other->x, self->y - other->y };
}

Vec2f vec2f_add(const Vec2f* const self, const Vec2f* const other)
{
  return (Vec2f) { self->x + other->x, self->y + other->y };
}
