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
#include "bifrost_component_handle_storage.hpp"
#include "bifrost_collision_system.hpp"

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
  class ISerializer;
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

    // Getters

    [[nodiscard]] Scene&            scene() const { return m_OwningScene; }
    [[nodiscard]] const String&     name() const { return m_Name; }
    [[nodiscard]] BifrostTransform& transform() const;
    [[nodiscard]] const EntityList& children() const { return m_Children; }
    [[nodiscard]] BVHNodeOffset     bvhID() const { return m_BHVNode; }

    // Child API
    //
    // This API needs to make it nearly impossible to leak children.
    // This is why there is no "removeChild" function publically available.
    // To removed a child you must destroy the object itself which
    // will enforce that it does not dangle outside of the hierarchy ;)

    Entity* addChild(const StringRange& name);
    void    setParent(Entity* new_parent);

    // Component API

    template<typename T>
    T& add()
    {
      if (!has<T>())
      {
        auto& handle = componentHandle<T>();

        handle.handle    = getComponentList<T>(true).add(*this);
        handle.is_active = true;
      }

      return *get<T>();
    }

    template<typename T>
    T* get() const
    {
      const auto& handle = componentHandle<T>();

      if (handle.handle.isValid())
      {
        return &getComponentList<T>(handle.is_active).find(handle.handle);
      }

      return nullptr;
    }

    template<typename T>
    bool has() const
    {
      return get<T>() != nullptr;
    }

    template<typename T>
    bool isComponentActive()
    {
      const auto& handle = componentHandle<T>();

      return handle.handle.isValid() && handle.is_active;
    }

    template<typename T>
    void setComponentActive(bool value)
    {
      if (has<T>())
      {
        if (value != isComponentActive<T>())
        {
          auto& handle     = componentHandle<T>();
          T&    old_data   = *get<T>();
          auto& new_list   = getComponentList<T>(value);
          auto  new_handle = new_list.add(*this);
          T&    new_data   = new_list.find(new_handle);

          new_data = std::move(old_data);

          remove<T>();

          handle.handle    = new_handle;
          handle.is_active = value;
        }
      }
      else
      {
        // TODO(SR): Error?
      }
    }

    template<typename T>
    void remove()
    {
      auto& handle = m_ComponentHandles.get<T>();

      if (handle.handle.isValid())
      {
        getComponentList<T>(handle.is_active).remove(handle.handle);
        handle.handle    = {};
        handle.is_active = false;
      }
      else
      {
        // TODO(SR): Error?
      }
    }

    // Meta

    void serialize(ISerializer& serializer);

    ~Entity();

   private:
    const BifrostTransform& metaGetTransform() const { return transform(); }
    // ReSharper disable once CppMemberFunctionMayBeConst
    void metaSetTransform(const BifrostTransform& value)
    {
      bfTransform_copyFrom(&transform(), &value);
    }

    template<typename T>
    DenseMap<T>& getComponentList(bool is_active) const
    {
      return is_active ? m_OwningScene.m_ActiveComponents.get<T>() : m_OwningScene.m_InactiveComponents.get<T>();
    }

    template<typename T>
    ComponentHandle<T>& componentHandle()
    {
      return m_ComponentHandles.get<T>();
    }

    template<typename T>
    const ComponentHandle<T>& componentHandle() const
    {
      return m_ComponentHandles.get<T>();
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
