#ifndef BIFROST_SCENE_HPP
#define BIFROST_SCENE_HPP

#include "bifrost/bifrost_math.h"               /* Vec3f, Mat4x4 */
#include "bifrost/core/bifrost_base_object.hpp" /* BaseObject<T> */

namespace bifrost
{
  class Entity;

  struct SceneNodeRef
  {
    static constexpr int INVALID_INDEX = -1;

    int index;

    SceneNodeRef(int index = INVALID_INDEX) :
      index{index}
    {
    }

    [[nodiscard]] bool isValid() const
    {
      return index != INVALID_INDEX;
    }
  };

  struct SceneNode
  {
    Vec3f        position;
    Vec3f        rotation;
    Vec3f        scale;
    Mat4x4       local_transform;
    Mat4x4       world_transform;
    SceneNodeRef parent;
    SceneNodeRef first_child;
    SceneNodeRef next_sibling;
    SceneNodeRef prev_sibling;

    void addChild(SceneNodeRef this_id, SceneNodeRef child, Array<SceneNode>& data)
    {
      data[child.index].parent = this_id;

      if (first_child.isValid())
      {
        SceneNodeRef child_it = first_child;

        while (data[child_it.index].next_sibling.isValid())
        {
          child_it = data[child_it.index].next_sibling;
        }

        data[child_it.index].next_sibling = child;
      }
      else
      {
        first_child = child;
      }
    }

    void removeChild()
    {
    }
  };

  class Scene final : public BaseObject<Scene>
  {
    BIFROST_META_FRIEND;

   private:
    Array<Entity*>   m_Entities;
    Array<SceneNode> m_SceneNodes;

   public:
    explicit Scene(IMemoryManager& memory) :
      m_Entities{memory},
      m_SceneNodes{memory}
    {
    }

    SceneNodeRef addNode(SceneNodeRef parent = {SceneNodeRef::INVALID_INDEX})
    {
      const SceneNodeRef id{int(m_SceneNodes.size())};
      SceneNode&         node = m_SceneNodes.emplace();

      node.parent       = parent;
      node.first_child  = SceneNodeRef::INVALID_INDEX;
      node.next_sibling = SceneNodeRef::INVALID_INDEX;
      node.prev_sibling = SceneNodeRef::INVALID_INDEX;

      if (parent.isValid())
      {
      }

      return id;
    }
  };

  BIFROST_META_REGISTER(Scene)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<Scene>("Scene"),                                                          //
       ctor<IMemoryManager&>(),                                                             //
       field_readonly("m_Entities", &Scene::m_Entities, offsetof(Scene, m_Entities)),       //
       field_readonly("m_SceneNodes", &Scene::m_SceneNodes, offsetof(Scene, m_SceneNodes))  //
      )
    BIFROST_META_END()
  }
}  // namespace bifrost

#endif /* BIFROST_SCENE_HPP */
