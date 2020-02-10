#include "bifrost/ecs/bifrost_entity.hpp"

#include "bifrost/asset_io/bifrost_scene.hpp"

namespace bifrost
{
  Entity::Entity(Scene& scene, const std::string_view name) :
    m_OwningScene{scene},
    m_Name{name},
    m_Transform{},
    m_Parent{nullptr},
    m_Children{&Entity::m_Hierarchy},
    m_Components{scene.m_Memory}
  {
    bfTransform_ctor(&m_Transform);
  }

  Entity* Entity::addChild(std::string_view name)
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

  Entity::~Entity()
  {
    if (m_Parent)
    {
      m_Parent->removeChild(this);
    }

    bfTransform_dtor(&m_Transform);
  }

  void Entity::removeChild(Entity* child)
  {
    m_Children.erase(m_Children.makeIterator(* child));
    child->m_Parent = nullptr;
  }
}  // namespace bifrost
