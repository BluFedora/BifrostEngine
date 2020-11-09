#ifndef SPATIAL_H
#define SPATIAL_H

#include "bifrost_prismtypes.h"

namespace prism
{
  // Quad / Oct Tree

  struct OctTreeNode
  {
    Vec3         position;
    OctTreeNode *child[8];

    uint getChildIndex(const Vec3 &object) const
    {
      uint index(0);

      if (object.x > position.x) index += 1;
      if (object.y > position.y) index += 2;
      if (object.z > position.z) index += 4;

      return index;
    }
  };
}  // namespace prism

#endif  // SPATIAL_H