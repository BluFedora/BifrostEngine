/*!
 * @file   bifrost_component_list.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Registry for all built in engine components.
 *   For registering a new component search for '@EngineComponentRegister'.
 * 
 *   1) First add a forward declaration of you type to the top of this file.
 *   2) Add yor type to the list of types contained in 'ComponentPack'.
 *   3) Add to 'g_EngineComponentInfo' your component type info.
 *      ** Make Sure The order in the 'ComponentPack' is the same. **
 *   4) Include your type in the 'bifrost_component_storage.hpp' header.
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_COMPONENT_LIST_HPP
#define BIFROST_COMPONENT_LIST_HPP

#include "bifrost/data_structures/bifrost_string.hpp"    /* StringRange   */
#include "bifrost/meta/bifrost_meta_function_traits.hpp" /* ParameterPack */

namespace bifrost
{
  //
  // @EngineComponentRegister
  // Forward Declarations go here.
  //

  class MeshRenderer;

  struct ComponentTypeInfo final
  {
    const StringRange name;
  };

  //
  // @EngineComponentRegister
  // All Engine Components must be listed here.
  //
  using ComponentPack = meta::ParameterPack<
    MeshRenderer
   >;

  namespace component
  {
    //
    // @EngineComponentRegister
    // Some type information for components for editor and serialization.
    //
    static constexpr ComponentTypeInfo g_EngineComponentInfo[ComponentPack::size] =
     {
      {"MeshRenderer"},
    };
  }  // namespace component

}  // namespace bifrost

#endif /* BIFROST_COMPONENT_LIST_HPP */
