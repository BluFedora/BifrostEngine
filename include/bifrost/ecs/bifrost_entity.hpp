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
 * @copyright Copyright (c) 2019-2020
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
     field("Origin", &BifrostTransform::origin),            //
     field("Position", &BifrostTransform::local_position),  //
     field("Rotation", &BifrostTransform::local_rotation),  //
     field("Scale", &BifrostTransform::local_scale)         //
    );

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
    friend class Scene;

   private:
    Scene&                  m_OwningScene;
    String                  m_Name;
    BifrostTransformID      m_Transform;
    Entity*                 m_Parent;
    EntityList              m_Children;
    intrusive::Node<Entity> m_Hierarchy;
    ComponentHandleStorage  m_ComponentHandles;
    BVHNodeOffset           m_BHVNode;

   public:
    Entity(Scene& scene, const StringRange& name);

    [[nodiscard]] Scene&            scene() const { return m_OwningScene; }
    [[nodiscard]] const String&     name() const { return m_Name; }
    [[nodiscard]] BifrostTransform& transform() const;
    [[nodiscard]] const EntityList& children() const { return m_Children; }
    [[nodiscard]] BVHNodeOffset     bvhID() const { return m_BHVNode; }

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

      m_ComponentHandles.get<T>() = getComponentList<T>().add(*this);

      return *get<T>();
    }

    template<typename T>
    T* get() const
    {
      const auto& handle = m_ComponentHandles.get<T>();

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
      auto& handle = m_ComponentHandles.get<T>();

      if (handle.isValid())
      {
        getComponentList<T>().remove(handle);
        handle = {};
      }
    }

    void serialize(ISerializer& serializer);

    ~Entity();

   private:
    const BifrostTransform& metaGetTransform() const { return transform(); }
    void                    metaSetTransform(const BifrostTransform& value)
    {
      bfTransform_copyFrom(&transform(), &value);
    }

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
     class_info<Entity>("Entity"),                                                  //
     ctor<Scene&, const StringRange&>(),                                            //
     field("m_Name", &Entity::m_Name),                                              //
     property("m_Transform", &Entity::metaGetTransform, &Entity::metaSetTransform)  //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_ENTITY_HPP */
