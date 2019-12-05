//
//  transform.c
//  OpenGL
//
//  Created by Shareef Raheem on 12/21/17.
//  Copyright © 2017 Shareef Raheem. All rights reserved.
//
#include "bifrost/math/bifrost_transform.h"

#include "bifrost/math/bifrost_mat4x4.h"

#ifndef NULL
#define NULL ((void*)(0))
#endif

static const Vec3f move_zero  = {0, 0, 0, 1};
static const Vec3f rot_zero   = {0, 0, 0, 0};
static const Vec3f scl_zero   = {1, 1, 1, 1};
static const Vec3f pivot_zero = {0, 0, 0, 1};

void TransformCtor(Transform* transform)
{
  Mat4x4_identity(&transform->_cachedMatrix);
  Vec3f_copy(&transform->position, &move_zero);
  Vec3f_copy(&transform->rotation, &rot_zero);
  Vec3f_copy(&transform->scale, &scl_zero);
  Vec3f_copy(&transform->origin, &pivot_zero);
  transform->parent = NULL;
}

Mat4x4* Transform_update(Transform* transform)
{
  const int     has_moved     = !Vec3f_isEqual(&transform->position, &move_zero);
  const int     has_rotated   = !Vec3f_isEqual(&transform->rotation, &rot_zero);
  const int     has_scaled    = !Vec3f_isEqual(&transform->scale, &scl_zero);
  const int     has_pivoted   = !Vec3f_isEqual(&transform->origin, &pivot_zero);
  Mat4x4* const cached_matrix = &transform->_cachedMatrix;

  if (has_moved || has_rotated || has_scaled || has_pivoted)
  {
    Mat4x4 neg_origin, scale, rotation, translation, pos_origin;

    if (has_pivoted)
    {
      Mat4x4_initTranslatef(&neg_origin, -transform->origin.x, -transform->origin.y, -transform->origin.z);
      Mat4x4_initTranslatef(&pos_origin, transform->origin.x, transform->origin.y, transform->origin.z);
    }

    if (has_scaled) Mat4x4_initScalef(&scale, transform->scale.x, transform->scale.y, transform->scale.z);
    if (has_rotated) Mat4x4_initRotationf(&rotation, transform->rotation.x, transform->rotation.y, transform->rotation.z);
    if (has_moved) Mat4x4_initTranslatef(&translation, transform->position.x, transform->position.y, transform->position.z);

    if (transform->parent)
    {
      Mat4x4* parent_matrix = Transform_update(transform->parent);
      Mat4x4_mult(cached_matrix, parent_matrix, cached_matrix);
    }
    else
    {
      Mat4x4_identity(cached_matrix);
    }

    /* parent => neg_origin => scale => rotate => translate => pos_origin */
    if (has_pivoted) Mat4x4_mult(&neg_origin, cached_matrix, cached_matrix);
    if (has_scaled) Mat4x4_mult(&scale, cached_matrix, cached_matrix);
    if (has_rotated) Mat4x4_mult(&rotation, cached_matrix, cached_matrix);
    if (has_pivoted) Mat4x4_mult(&pos_origin, cached_matrix, cached_matrix);
    if (has_moved) Mat4x4_mult(&translation, cached_matrix, cached_matrix);
  }

  return cached_matrix;
}
