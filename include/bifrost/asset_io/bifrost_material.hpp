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
  class ShaderProgram final : public BaseObject<ShaderProgram>
  {
   private:
    AssetShaderModuleHandle m_VertexShader;
    AssetShaderModuleHandle m_FragmentShader;

   public:
    ShaderProgram() :
      BaseObject<ShaderProgram>(),
      m_VertexShader{nullptr},
      m_FragmentShader{nullptr}
    {
    }
  };

  // using AssetShaderProgramHandle = AssetHandle<ShaderProgram>;

  class Material final : public BaseObject<Material>
  {
   private:
    AssetShaderProgramHandle m_ShaderProgram;
    AssetTextureHandle       m_DiffuseTexture;

   public:
    explicit Material() :
      BaseObject<Material>(),
      m_ShaderProgram{nullptr},
      m_DiffuseTexture{nullptr}
    {
    }

    const AssetShaderProgramHandle& shaderProgram() const { return m_ShaderProgram; }
    const AssetTextureHandle&       diffuseTexture() const { return m_DiffuseTexture; }
  };

  using AssetMaterialHandle = AssetHandle<Material>;
}  // namespace bifrost

#endif /* BIFROST_MATERIAL_HPP */
