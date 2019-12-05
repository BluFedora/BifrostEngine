#pragma once

#if __cplusplus
extern "C"
{
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

  void  Vec2f_set(Vec2f* self, const float x, const float y);
  void  Vec2f_copy(Vec2f* const self, const Vec2f* const src);
  void  Vec2f_addScaled(Vec2f* self, const Vec2f* other, const float dt);
  void  Vec2f_add(Vec2f* self, const Vec2f* other);
  void  Vec2f_sub(Vec2f* self, const Vec2f* other);
  void  Vec2f_subScaled(Vec2f* self, const Vec2f* other, const float dt);
  void  Vec2f_normalize(Vec2f *self);
  void  Vec2f_multScalar(Vec2f *self, const float factor);
  float Vec2f_dot(const Vec2f *v1, const Vec2f *v2);
  float Vec2f_lenSq(const Vec2f* const v1);
  float Vec2f_len(const Vec2f *v1);
  float Vec2f_cross(const Vec2f* lhs, const Vec2f* rhs);

    // 'static' functions
  Vec2f vec2f(const float x, const float y);
  Vec2f vec2f_sub(const Vec2f* const self, const Vec2f* const other);
  Vec2f vec2f_add(const Vec2f* const self, const Vec2f* const other);
#if __cplusplus
}
#endif
