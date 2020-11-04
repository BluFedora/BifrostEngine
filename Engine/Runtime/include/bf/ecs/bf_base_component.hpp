/******************************************************************************/
/*!
 * @file   bf_base_component.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   The base class for all core engine components.
 *   Look into "bf_component_list.hpp" for registering components.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_BASE_COMPONENT_HPP
#define BF_BASE_COMPONENT_HPP

#include <type_traits> /* false_type, void_t, declval, is_base_of_v */

class Engine;

namespace bf
{
  class Entity;
  class Scene;

  /*!
   * @brief
   *   The base class for all core engine components.
   */
  class BaseComponent
  {
   protected:
    Entity* m_Owner;  //!< The entity that this compoinent is attached to.

   public:
    explicit BaseComponent(Entity& owner);
    explicit BaseComponent();

    /*!
     * @brief
     *   Helper for grabbing Entity this component is attached to.
     * 
     * @return
     *   The Entity this component is attached to
     */
    Entity& owner() const;

    /*!
     * @brief
     *   Helper for grabbing Scene this component's owner is inside of.
     * 
     * @return
     *   The Scene this component's owner is inside of.
     */
    Scene& scene() const;

    /*!
     * @brief
     *   Helper for grabbing 'global' Engine.
     * 
     * @return
     *   The engine.
     */
    Engine& engine() const;
  };

  /*!
  * @brief
  *   The CRTP class to inherit from.
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
      static_assert(std::is_base_of_v<Base, TSelf>, "TSelf must be the same as the type deriving from this class.");
    }

   private:
    template<typename T, typename = void>
    struct HasOnEnable : std::false_type
    {};

    template<typename T>
    struct HasOnEnable<T, std::void_t<decltype(std::declval<T>().onEnable(std::declval<Engine&>()))>> : std::true_type
    {};

    template<typename T, typename = void>
    struct HasOnDisable : std::false_type
    {};

    template<typename T>
    struct HasOnDisable<T, std::void_t<decltype(std::declval<T>().onDisable(std::declval<Engine&>()))>> : std::true_type
    {};

    void privateOnEnable(Engine& engine)
    {
      if constexpr (HasOnEnable<TSelf>::value)
      {
        static_cast<TSelf*>(this)->onEnable(engine);
      }
    }

    void privateOnDisable(Engine& engine)
    {
      if constexpr (HasOnDisable<TSelf>::value)
      {
        static_cast<TSelf*>(this)->onDisable(engine);
      }
    }
  };
}  // namespace bf

#endif /* BF_BASE_COMPONENT_HPP */
