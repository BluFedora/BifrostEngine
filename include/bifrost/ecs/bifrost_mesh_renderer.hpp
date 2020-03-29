/*!
 * @file   bifrost_mesh_renderer.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *
 * @version 0.0.1
 * @date    2020-03-21
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_MESH_RENDERER_HPP
#define BIFROST_MESH_RENDERER_HPP

#include "bifrost/asset_io/bifrost_material.hpp" /* AssetMaterialHandle */
#include "bifrost_base_component.hpp"            /* BaseComponent       */

namespace bifrost
{
  class MeshRenderer : public Component<MeshRenderer>
  {
   private:
    AssetMaterialHandle m_Material;
    AssetModelHandle    m_Model;

   public:
    explicit MeshRenderer(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_Model{nullptr}
    {
    }

    AssetMaterialHandle& material() { return m_Material; }
    AssetModelHandle&    model() { return m_Model; }
  };
}  // namespace bifrost

#endif /* BIFROST_MESH_RENDERER_HPP */
