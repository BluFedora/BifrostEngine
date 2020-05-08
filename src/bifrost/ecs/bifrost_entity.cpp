/*!
* @file   bifrost_entity.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   This engine's concept of a GameObject / Actor.
*   An entity is a bag of components with a 'BifrostTransform' and a name.
*
* @version 0.0.1
* @date    2019-12-22
*
* @copyright Copyright (c) 2019-2020
*/
#include "bifrost/ecs/bifrost_entity.hpp"

#include "bifrost/asset_io/bifrost_asset_handle.hpp"
#include "bifrost/asset_io/bifrost_scene.hpp"

namespace bifrost
{
  Entity::Entity(Scene& scene, const StringRange& name) :
    m_OwningScene{scene},
    m_Name{name},
    m_Transform{scene.m_TransformSystem.createTransform()},
    m_Parent{nullptr},
    m_Children{&Entity::m_Hierarchy},
    m_ComponentHandles{},
    m_BHVNode{k_BVHNodeInvalidOffset}
  {
  }

  BifrostTransform& Entity::transform() const
  {
    IBifrostTransformSystem& transform_system = scene().m_TransformSystem;

    return *transform_system.transformFromID(&transform_system, m_Transform);
  }

  Entity* Entity::addChild(const StringRange& name)
  {
    Entity* child   = m_OwningScene.createEntity(name);
    child->m_Parent = this;
    m_Children.pushBack(*child);
    return child;
  }

  void Entity::setParent(Entity* new_parent)
  {
    if (m_Parent != new_parent)
    {
      if (m_Parent)
      {
        m_Parent->removeChild(this);
      }

      m_Parent = new_parent;

      if (new_parent)
      {
        new_parent->m_Children.pushBack(*this);
      }
      else
      {
        m_OwningScene.m_RootEntities.push(this);
      }
    }
  }

  void Entity::serialize(ISerializer& serializer)
  {
    serializer.serializeT(this);

    if (serializer.mode() != SerializerMode::INSPECTING)
    {
      if (serializer.pushObject("m_Components"))
      {
        ComponentStorage::forEachType([&serializer, this](auto t) {
          using T = bfForEachTemplateT(t);

          const StringRange name = g_EngineComponentInfo[bfForEachTemplateIndex(t)].name;

          T* component;

          if (serializer.mode() == SerializerMode::LOADING && serializer.hasKey(name))
          {
            component = &add<T>();
          }
          else
          {
            component = get<T>();
          }

          if (component && serializer.pushObject(name))
          {
            serializer.serializeT(component);
            serializer.popObject();
          }
        });

        serializer.popObject();
      }
    }
  }

  Entity::~Entity()
  {
    for (Entity& child : m_Children)
    {
      m_OwningScene.destroyEntity(&child);
    }

    scene().m_TransformSystem.destroyTransform(m_Transform);

    if (m_Parent)
    {
      m_Parent->removeChild(this);
    }
  }

  void Entity::removeChild(Entity* child)
  {
    m_Children.erase(m_Children.makeIterator(*child));
    child->m_Parent = nullptr;
  }
}  // namespace bifrost
