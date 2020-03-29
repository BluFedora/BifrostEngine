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
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_COMPONENT_STORAGE_HPP
#define BIFROST_COMPONENT_STORAGE_HPP

#include "bifrost/data_structures/bifrost_container_tuple.hpp" /* ContainerTuple<T> */
#include "bifrost/data_structures/bifrost_dense_map.hpp"       /* DenseMap<T>       */
#include "bifrost_component_list.hpp"                          /* ComponentPack     */

//
// @EngineComponentRegister
// The Array datatype needs to know the sizeof(T) in it's constructor. (This required should be removed in later versions of this engine.)
//
#include "bifrost/ecs/bifrost_light.hpp"         /* Light        */
#include "bifrost/ecs/bifrost_mesh_renderer.hpp" /* MeshRenderer */

namespace bifrost
{
  template<typename... Args>
  using DenseMapTuple    = ContainerTuple<DenseMap, Args...>;
  using ComponentStorage = ComponentPack::apply<DenseMapTuple>;

  template<typename... Args>
  using DenseMapHandleTuple    = ContainerTuple<DenseMapHandle, Args...>;
  using ComponentHandleStorage = ComponentPack::apply<DenseMapHandleTuple>;
}  // namespace bifrost

#endif /* BIFROST_COMPONENT_STORAGE_HPP */
