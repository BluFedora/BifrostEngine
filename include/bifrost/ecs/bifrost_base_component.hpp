/*!
 * @file   bifrost_base_component.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   The base class for all core engine components.
 *   Look into "bifrost_component_list.hpp" for registering
 *   components.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2019-2020
 */
#ifndef BIFROST_BASE_COMPONENT_HPP
#define BIFROST_BASE_COMPONENT_HPP

#include <type_traits> /* false_type, void_t, declval */

class BifrostEngine;

namespace bifrost
{
  using Engine = ::BifrostEngine;

  class Entity;

  /*
    Planned Component Types;

      * AudioSource
      * ParticleEmitter
      * AIAgent
      * Sprite
      * Animator2D (SpriteAnimator)
        > Needs Sprite
      * Mesh
      * Animator3D (MeshAnimator)
        > Needs Mesh
      * TimelineAnimator
      * Tilemap
      * Collider2D
        > Needs: RigidBody2D
      * Collider3D
        > Needs: RigidBody3D
      * RigidBody2D
      * RigidBody3D
      * LightSource
      * GUI?
        > Text, Button, Etc
      * Combat/Health?

    Built In Scripts:

      * ParallaxBackground
      * CameraTarget
  */

  /*!
   * @brief
   *   The base class for all core engine components.
   */
  class BaseComponent
  {
   protected:
    Entity* m_Owner;

   public:
    explicit BaseComponent(Entity& owner);

    Entity& owner() const { return *m_Owner; }
  };

  /*!
  * @brief
  *   The class to inherit from.
  */
  template<typename TSelf>
  class Component : public BaseComponent
  {
    friend class Entity;

   public:
    using Base = Component<TSelf>;

    explicit Component(Entity& owner) :
      BaseComponent(owner)
    {
    }

   private:
    template<typename T, typename = void>
    struct HasOnEnable : std::false_type
    {};

    template<typename T>
    struct HasOnEnable<T, std::void_t<decltype(std::declval<T>().onEnable())>> : std::true_type
    {};

    template<typename T, typename = void>
    struct HasOnDisable : std::false_type
    {};

    template<typename T>
    struct HasOnDisable<T, std::void_t<decltype(std::declval<T>().onDisable())>> : std::true_type
    {};

    void privateOnEnable(Engine& engine)
    {
      if constexpr (HasOnEnable<TSelf>::value)
      {
        TSelf::onEnable(engine);
      }
    }

    void privateOnDisable(Engine& engine)
    {
      if constexpr (HasOnDisable<TSelf>::value)
      {
        TSelf::onDisable(engine);
      }
    }
  };
}  // namespace bifrost

#endif /* BIFROST_BASE_COMPONENT_HPP */
