/*!
 * @file bifrost_entity.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   This engine's concept of a GameObject / Actor.
 *   An entity is a bag of components with a 'BifrostTransform' and name.
 *   
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_ENTITY_HPP
#define BIFROST_ENTITY_HPP

#include "bifrost/asset_io/bifrost_scene.hpp"    // Scene
#include "bifrost/core/bifrost_base_object.hpp"  // BaseObject
#include "bifrost/data_structures/bifrost_intrusive_list.hpp"
#include "bifrost/math/bifrost_transform.h"  // BifrostTransform

namespace bifrost
{
  // class Scene;

  class Entity;

  using EntityList = intrusive::ListView<Entity>;

  class Entity final : public BaseObject<Entity>
  {
    BIFROST_META_FRIEND;

   private:
    Scene&                                           m_OwningScene;
    std::string                                      m_Name;
    BifrostTransform                                 m_Transform;
    Entity*                                          m_Parent;
    EntityList                                       m_Children;
    intrusive::Node<Entity>                          m_Hierarchy;
    Array<std::pair<std::uint32_t, dense_map::ID_t>> m_Components;  // <type, id>

   public:
    Entity(Scene& scene, std::string_view name);

    [[nodiscard]] Scene&             scene() const { return m_OwningScene; }
    [[nodiscard]] const std::string& name() const { return m_Name; }
    [[nodiscard]] BifrostTransform&  transform() { return m_Transform; }
    [[nodiscard]] const EntityList&  children() const { return m_Children; }

    template<typename F>
    void forEachComp(F&& f)
    {
      for (const auto& c_data : m_Components)
      {
        f(m_OwningScene.m_ComponentStorage[c_data.first]->getComponent(c_data.second));
      }
    }

    Entity* addChild(std::string_view name);
    void    setParent(Entity* new_parent);

    template<typename T>
    T* add()
    {
      const dense_map::ID_t index = m_OwningScene.addComponent<T>(*this);
      m_Components.emplace(T::s_ComponentID, index);
      return (T*)m_OwningScene.m_ComponentStorage[T::s_ComponentID]->getComponent(index);
    }

    ~Entity();

   private:
    void removeChild(Entity* child);
  };

}  // namespace bifrost

#endif /* BIFROST_ENTITY_HPP */
