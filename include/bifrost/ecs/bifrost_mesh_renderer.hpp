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

// #include "bifrost_entity.hpp" // TEMP

namespace bifrost
{
  class MeshRenderer : public Component<MeshRenderer>
  {
    BIFROST_META_FRIEND;

   private:
    AssetMaterialHandle m_Material;
    Ref<Entity>         m_EntityRef;  // TEMP CODE
    AssetModelHandle    m_Model;

   public:
    explicit MeshRenderer(Entity& owner) :
      Base(owner),
      m_Material{nullptr},
      m_EntityRef{},
      m_Model{nullptr}
    {
    }

    AssetMaterialHandle& material() { return m_Material; }
    AssetModelHandle&    model() { return m_Model; }
  };
}  // namespace bifrost

BIFROST_META_REGISTER(bifrost::MeshRenderer)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<MeshRenderer>("MeshRenderer"),                         //
     field<BaseAssetHandle>("m_Material", &MeshRenderer::m_Material),  //
     field<BaseRef>("m_EntityRef", &MeshRenderer::m_EntityRef),        //
     field<BaseAssetHandle>("m_Model", &MeshRenderer::m_Model)         //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_MESH_RENDERER_HPP */
