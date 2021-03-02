/******************************************************************************/
/*!
 * @file   bf_base_component.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   The base class for all core engine components.
 *   Look into "bf_component_list.hpp" for registering components.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_BASE_COMPONENT_HPP
#define BF_BASE_COMPONENT_HPP

#include <type_traits> /* false_type, void_t, declval, is_base_of_v */

namespace bf
{
  // Forward Declarations

  class Engine;
  class Entity;
  class Scene;

  /*!
   * @brief
   *   The base class for all core engine components.
   */
  class BaseComponent
  {
   protected:
    Entity* m_Owner;  //!< The entity that this component is attached to.

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
    [[nodiscard]] Entity& owner() const;

    /*!
     * @brief
     *   Helper for grabbing Scene this component's owner is inside of.
     * 
     * @return
     *   The Scene this component's owner is inside of.
     */
    [[nodiscard]] Scene& scene() const;

    /*!
     * @brief
     *   Helper for grabbing 'global' Engine.
     * 
     * @return
     *   A reference to the engine.
     */
    [[nodiscard]] Engine& engine() const;
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
    /*
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

    template<typename T, typename = void>
    struct HasOnDestroy : std::false_type
    {};

    template<typename T>
    struct HasOnDestroy<T, std::void_t<decltype(std::declval<T>().onDestroy(std::declval<Engine&>()))>> : std::true_type
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

    void privateOnDestroy(Engine& engine)
    {
      if constexpr (HasOnDestroy<TSelf>::value)
      {
        static_cast<TSelf*>(this)->onDestroy(engine);
      }
    }
    */
  };

  //
  // Allows for customizing the behavior of a component on certain
  // events without introducing significant runtime overhead.
  //
  namespace ComponentTraits
  {
    // clang-format off

    template<typename T> void onCreate(T& component, Engine& engine)  { (void)component; (void)engine; }
    template<typename T> void onEnable(T& component, Engine& engine)  { (void)component; (void)engine; }
    template<typename T> void onDisable(T& component, Engine& engine) { (void)component; (void)engine; }
    template<typename T> void onDestroy(T& component, Engine& engine) { (void)component; (void)engine; }

    // clang-format on
  }  // namespace ComponentTraits
}  // namespace bf

#endif /* BF_BASE_COMPONENT_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2019-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
