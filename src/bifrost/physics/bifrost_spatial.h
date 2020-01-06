#ifndef SPATIAL_H
#define SPATIAL_H

#include "bifrost_prismtypes.h"
#include <vector>

namespace prism
{
  enum class SpacialType
  {
    BINARY_SPACE  = 0,
    OCT_QUAD_TREE = 1,
    GRID          = 2,
    MULTIRES_MAP  = 3
  };

  // Binary Space

  struct SpatialPlane
  {
    Vec3 position;
    Vec3 direction;
  };

  struct BSPNode
  {
    SpatialPlane plane;
    BSPNode *    front;
    BSPNode *    back;
  };

  using BSPObjectSet = std::vector<BSPNode *>;

  enum class BSChildType
  {
    NODE,
    OBJECTS
  };

  struct BSPChild
  {
    BSChildType type;

    union
    {
      BSPNode *     node;
      BSPObjectSet *objects;
    };
  };

  // Quad / Oct Tree

  struct QuadTreeNode
  {
    Vec3          position;
    QuadTreeNode *child[4];

    uint getChildIndex(const Vec3 &object)
    {
      uint index(0);

      if (object.x > position.x) index += 1;
      if (object.z > position.z) index += 2;

      return index;
    }
  };

  struct OctTreeNode
  {
    Vec3         position;
    OctTreeNode *child[8];

    uint getChildIndex(const Vec3 &object)
    {
      uint index(0);

      if (object.x > position.x) index += 1;
      if (object.y > position.y) index += 2;
      if (object.z > position.z) index += 4;

      return index;
    }
  };

  class Spatial
  {
   private:
    SpacialType m_Type;

   public:
    Spatial() :
      m_Type(SpacialType::BINARY_SPACE)
    {
      //TODO(BluFedora):: Implement Spacial screen particioning.
    }
  };
}  // namespace prism

#endif  // SPATIAL_H