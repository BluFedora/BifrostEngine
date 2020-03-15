/******************************************************************************/
/*!
* @file   bifrost_material.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*
* @version 0.0.1
* @date    2020-03-10
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#include "bifrost/asset_io/bifrost_material.hpp"

namespace bifrost
{
  bool Material::load(Engine& engine)
  {
    if (m_ShaderProgram && m_DiffuseTexture)
    {
      m_Handle = bfShaderProgram_createDescriptorSet(*m_ShaderProgram, 0);

      if (m_Handle)
      {
        bfTextureHandle diffuse = *m_DiffuseTexture;

        bfDescriptorSet_setCombinedSamplerTextures(m_Handle, 0, 0, &diffuse, 1);
        bfDescriptorSet_flushWrites(m_Handle);

        return true;
      }
    }

    return false;
  }
}
