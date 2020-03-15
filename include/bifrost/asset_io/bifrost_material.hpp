/******************************************************************************/
/*!
* @file   bifrost_material.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-10
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_MATERIAL_HPP
#define BIFROST_MATERIAL_HPP

#include "bifrost_asset_handle.hpp"  // AssetInfo<T>

namespace bifrost
{
  class ShaderProgram final : public detail::GfxHandle<bfShaderProgramHandle>
  {
   private:
    AssetShaderModuleHandle m_VertexShader;
    AssetShaderModuleHandle m_FragmentShader;

   public:
    ShaderProgram(const StringRange& path, const BifrostUUID& uuid) :
      GfxHandle<bfShaderProgramHandle>(path, uuid),
      m_VertexShader{nullptr},
      m_FragmentShader{nullptr}
    {
      m_Payload = this;
    }
  };

  class Material final : public detail::GfxHandle<bfDescriptorSetHandle>
  {
   private:
    AssetShaderProgramHandle m_ShaderProgram;
    AssetTextureHandle       m_DiffuseTexture;

   public:
    Material(const StringRange& path, const BifrostUUID& uuid) :
      GfxHandle<bfDescriptorSetHandle>(path, uuid),
      m_ShaderProgram{nullptr},
      m_DiffuseTexture{nullptr}
    {
      m_Payload = this;
    }

    bool                load(Engine& engine) override;
    AssetTextureHandle& diffuseTexture() { return m_DiffuseTexture; }
  };

  using AssetMaterialHandle = AssetHandle<Material, Material>;
}  // namespace bifrost

#endif /* BIFROST_MATERIAL_HPP */
