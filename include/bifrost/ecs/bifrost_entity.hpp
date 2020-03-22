/*!
 * @file   bifrost_entity.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   This engine's concept of a GameObject / Actor.
 *   An entity is a bag of components with a 'BifrostTransform' and a name.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_ENTITY_HPP
#define BIFROST_ENTITY_HPP

#include "bifrost/asset_io/bifrost_scene.hpp"                  // Scene
#include "bifrost/core/bifrost_base_object.hpp"                // BaseObject
#include "bifrost/data_structures/bifrost_intrusive_list.hpp"  // ListView
#include "bifrost/math/bifrost_transform.h"                    // BifrostTransform

namespace bifrost::meta
{
  template<>
  inline const auto& Meta::registerMembers<BifrostTransform>()
  {
    static auto member_ptrs = members(
     class_info<BifrostTransform>("Transform"),
     ctor<>(),
     field("origin", &BifrostTransform::origin),
     field("local_position", &BifrostTransform::local_position),
     field("local_rotation", &BifrostTransform::local_rotation),
     field("local_scale", &BifrostTransform::local_scale),
     field("world_position", &BifrostTransform::world_position),
     field("world_rotation", &BifrostTransform::world_rotation),
     field("world_scale", &BifrostTransform::world_scale),
     field("local_transform", &BifrostTransform::local_transform),
     field("world_transform", &BifrostTransform::world_transform));

    return member_ptrs;
  }
}  // namespace bifrost::meta

namespace bifrost
{
  class Entity;

  using EntityList = intrusive::ListView<Entity>;

  class Entity final : public BaseObject<Entity>
  {
    BIFROST_META_FRIEND;

   private:
    Scene&                  m_OwningScene;
    String                  m_Name;
    BifrostTransform        m_Transform;
    Entity*                 m_Parent;
    EntityList              m_Children;
    intrusive::Node<Entity> m_Hierarchy;
    ComponentHandleStorage  m_ComponentHandles;

   public:
    Entity(Scene& scene, const StringRange& name);

    [[nodiscard]] Scene&            scene() const { return m_OwningScene; }
    [[nodiscard]] const String&     name() const { return m_Name; }
    [[nodiscard]] BifrostTransform& transform() { return m_Transform; }
    [[nodiscard]] const EntityList& children() const { return m_Children; }

    Entity* addChild(const StringRange& name);
    void    setParent(Entity* new_parent);

    template<typename T>
    T& add()
    {
      T* const comp = get<T>();

      if (comp)
      {
        return *comp;
      }

      m_ComponentHandles.get<T>() = getComponentList<T>().add(*this);;
      return *get<T>();
    }

    template<typename T>
    T* get() const
    {
      const auto handle = m_ComponentHandles.get<T>();

      if (handle.isValid())
      {
        return &getComponentList<T>().find(handle);
      }

      return nullptr;
    }

    template<typename T>
    bool has() const
    {
      return get<T>() != nullptr;
    }

    template<typename T>
    void remove()
    {
      if (has<T>())
      {
        getComponentList<T>().remove(m_ComponentHandles.get<T>());
      }
    }

    void serialize(ISerializer& serializer);

    ~Entity();

   private:
    template<typename T>
    DenseMap<T>& getComponentList() const
    {
      return m_OwningScene.m_ActiveComponents.get<T>();
    }

    void removeChild(Entity* child);
  };
}  // namespace bifrost

BIFROST_META_REGISTER(bifrost::Entity)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<Entity>("Entity"),               //
     ctor<Scene&, const StringRange&>(),         //
     field("m_Name", &Entity::m_Name),           //
     field("m_Transform", &Entity::m_Transform)  //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_ENTITY_HPP */
