/*!
 * @file   bifrost_base_component.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   The base class for all core engine components.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2019
 */
#include "bifrost/ecs/bifrost_base_component.hpp"

namespace bifrost
{
  BaseComponent::BaseComponent(Entity& owner):
    m_Owner{&owner}
  {
  }

  BaseComponent::BaseComponent() :
    m_Owner{nullptr}
  {
  }
}
