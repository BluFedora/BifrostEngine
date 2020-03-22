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

#include "bifrost/asset_io/bifrost_file.hpp"
#include "bifrost/asset_io/bifrost_json_serializer.hpp"
#include "bifrost/core/bifrost_engine.hpp"
#include "bifrost/utility/bifrost_json.hpp"

namespace bifrost
{
  bool AssetTextureInfo::load(Engine& engine)
  {
    const bfGfxDeviceHandle device = bfGfxContext_device(engine.renderer().context());

    const bfTextureCreateParams params = bfTextureCreateParams_init2D(
     BIFROST_TEXTURE_UNKNOWN_SIZE,
     BIFROST_TEXTURE_UNKNOWN_SIZE,
     BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM);

    Texture& texture = m_Payload.set<Texture>(device);

    texture.m_Handle = bfGfxDevice_newTexture(device, &params);

    const String full_path = engine.assets().fullPath(*this);

    return bfTexture_loadFile(texture.m_Handle, full_path.cstr());
  }

  ShaderModule::ShaderModule(bfGfxDeviceHandle device) :
    BaseT(device)
  {
  }

  bool AssetShaderModuleInfo::load(Engine& engine)
  {
    const bfGfxDeviceHandle device        = bfGfxContext_device(engine.renderer().context());
    ShaderModule&           shader_module = m_Payload.set<ShaderModule>(device);
    const String&           full_path     = engine.assets().fullPath(*this);
    shader_module.m_Handle                = bfGfxDevice_newShaderModule(device, m_Type);

    return bfShaderModule_loadFile(shader_module.m_Handle, full_path.cstr());
  }

  void AssetShaderModuleInfo::serialize(Engine& engine, ISerializer& serializer)
  {
    serializer.serializeT("m_Type", &m_Type);
  }

  ShaderProgram::ShaderProgram(bfGfxDeviceHandle device) :
    BaseT(device),
    m_VertexShader{nullptr},
    m_FragmentShader{nullptr},
    m_NumDescriptorSets{1}
  {
  }

  void ShaderProgram::setNumDescriptorSets(std::uint32_t value)
  {
    if (m_NumDescriptorSets != value)
    {
      m_NumDescriptorSets = value;

      destroyHandle();
      createImpl();
    }
  }

  void ShaderProgram::createImpl()
  {
    if (m_VertexShader && m_FragmentShader)
    {
      bfShaderProgramCreateParams create_params;

      create_params.debug_name    = "__SHADER__";
      create_params.num_desc_sets = m_NumDescriptorSets;

      m_Handle = bfGfxDevice_newShaderProgram(m_GraphicsDevice, &create_params);

      // TODO: This isnt correct.
      bfShaderProgram_addModule(m_Handle, m_VertexShader->handle());
      bfShaderProgram_addModule(m_Handle, m_FragmentShader->handle());

      if (m_NumDescriptorSets)
      {
        bfShaderProgram_addImageSampler(m_Handle, "u_DiffuseTexture", 0, 0, 1, BIFROST_SHADER_STAGE_FRAGMENT);
        bfShaderProgram_addUniformBuffer(m_Handle, "u_ModelTransform", 0, 1, 1, BIFROST_SHADER_STAGE_VERTEX);
      }
    }
  }

  bool AssetShaderProgramInfo::load(Engine& engine)
  {
    const bfGfxDeviceHandle device = bfGfxContext_device(engine.renderer().context());

    auto& shader = m_Payload.set<ShaderProgram>(device);

    if (defaultLoad(engine))
    {
      shader.createImpl();
    }

    return true; // TODO: File doesn't exit on create.
  }

  bool AssetShaderProgramInfo::save(Engine& engine, ISerializer& serializer)
  {
    (void)engine;

    serializer.serialize(*payload());

    return true;
  }

  bool AssetMaterialInfo::load(Engine& engine)
  {
    m_Payload.set<Material>();
    defaultLoad(engine);
    return true; // TODO: File doesn't exit on create.
  }

  bool AssetMaterialInfo::save(Engine& engine, ISerializer& serializer)
  {
    (void)engine;

    serializer.serialize(*payload());

    return true;
  }
}  // namespace bifrost
