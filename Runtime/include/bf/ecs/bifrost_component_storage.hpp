/*!
 * @file   bifrost_component_storage.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Contains the type that should be used to store
 *   engine components in a cache friendly manner.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
#ifndef BIFROST_COMPONENT_STORAGE_HPP
#define BIFROST_COMPONENT_STORAGE_HPP

#include "bf/data_structures/bifrost_container_tuple.hpp" /* ContainerTuple<T> */
#include "bf/data_structures/bifrost_dense_map.hpp"       /* DenseMap<T>       */
#include "bifrost_component_list.hpp"                     /* ComponentPack     */

//
// @EngineComponentRegister
// The Array datatype needs to know the sizeof(T) in it's constructor. (This requirement should be removed in later versions of this engine.)
//
#include "bf/ecs/bifrost_light.hpp"              /* Light        */
#include "bf/ecs/bifrost_renderer_component.hpp" /* MeshRenderer */

namespace bf
{
  template<typename... Args>
  using DenseMapTuple    = ContainerTuple<DenseMap, Args...>;
  using ComponentStorage = ComponentPack::apply<DenseMapTuple>;
}  // namespace bf

#endif /* BIFROST_COMPONENT_STORAGE_HPP */
