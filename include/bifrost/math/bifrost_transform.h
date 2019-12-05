  //
  //  transform.h
  //  OpenGL
  //
  //  Created by Shareef Raheem on 12/21/17.
  //  Copyright © 2017 Shareef Raheem. All rights reserved.
  //
#ifndef transform_h
#define transform_h

#include "bifrost_vec3.h"
#include "bifrost_mat4x4.h"

#if __cplusplus
extern "C"
{
#endif
  typedef struct Transform_t
  {
    Mat4x4              _cachedMatrix;
    Vec3f               position;
    Vec3f               rotation;
    Vec3f               scale;
    Vec3f               origin;
    struct Transform_t* parent;

  } Transform;

  void    TransformCtor(Transform* transform);
  Mat4x4* Transform_update(Transform* transform);
#if __cplusplus
}
#endif

#endif /* transform_h */
