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

#include "bf/asset_io/bf_model_loader.hpp"
#include "bf/asset_io/bf_path_manip.hpp"

namespace bf
{
  ShaderModule::ShaderModule(bfGfxDeviceHandle device) :
    BaseT(device)
  {
  }

  bool AssetShaderModuleInfo::load(Engine& engine)
  {
    const bfGfxDeviceHandle device        = bfGfxContext_device(engine.renderer().context());
    ShaderModule&           shader_module = m_Payload.set<ShaderModule>(device);
    const String&           full_path     = filePathAbs();
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

    return true;  // TODO: File doesn't exit on create.
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
    return true;  // TODO: File doesn't exit on create.
  }

  bool AssetMaterialInfo::save(Engine& engine, ISerializer& serializer)
  {
    (void)engine;

    serializer.serialize(*payload());

    return true;
  }

  void loadObj(IMemoryManager& temp_allocator, Array<StandardVertex>& out, const char* obj_file_data, std::size_t obj_file_data_length);

  Model::Model(IMemoryManager& memory, bfGfxDeviceHandle device) :
    BaseT(device),
    m_NumVertices{0u},
    m_EmbeddedMaterials{memory}
  {
  }

  void Model::draw(bfGfxCommandListHandle cmd_list)
  {
    uint64_t buffer_offset = 0;
    bfGfxCmdList_bindVertexBuffers(cmd_list, 0, &m_Handle, 1, &buffer_offset);
    bfGfxCmdList_draw(cmd_list, 0, m_NumVertices);
  }

  static bool makeTexturePath(char (&abs_texture_path)[path::k_MaxLength], StringRange root_dir, AssetPBRMaterial& src_mat, int type)
  {
    const auto& texture = src_mat.textures[type];

    if (texture)
    {
      const auto [path_length, is_truncated] = path::append(abs_texture_path, bfCArraySize(abs_texture_path), root_dir, StringRange(texture));

      return !is_truncated;
    }

    return false;
  }

  static AssetTextureHandle getTetxureAssetHandle(Assets& assets, StringRange file_path)
  {
    const auto           uuid       = assets.indexAsset<AssetTextureInfo>(file_path);
    BaseAssetInfo* const asset_info = assets.findAssetInfo(uuid);

    if (asset_info)
    {
      return assets.makeHandleT<AssetTextureHandle>(*asset_info);
    }

    return nullptr;
  }

  bool AssetModelInfo::load(Engine& engine)
  {
    LinearAllocatorScope    mem_scope0    = engine.tempMemory();
    auto&                   no_free_alloc = engine.tempMemoryNoFree();
    const bfGfxDeviceHandle device        = bfGfxContext_device(engine.renderer().context());
    Model&                  model         = m_Payload.set<Model>(engine.mainMemory(), device);
    const String&           full_path     = filePathAbs();
    const StringRange       file_dir      = path::directory(full_path);

    {
      LinearAllocatorScope mem_scope1 = engine.tempMemory();

      AssetModelLoadResult model_result = loadModel(AssetModelLoadSettings(full_path, no_free_alloc));

      if (model_result)
      {
        char abs_texture_path[path::k_MaxLength] = {'\0'};

        for (AssetPBRMaterial& src_mat : *model_result.materials)
        {
          // TOOD(SR): Make this function also return the length so that the call to 'getTetxureAssetHandle' doesnt have to recalc length.
          if (makeTexturePath(abs_texture_path, file_dir, src_mat, PBRTextureType::DIFFUSE))
          {
            //auto texture_handle = getTetxureAssetHandle(engine.assets(), abs_texture_path);

            //__debugbreak();
          }
        }
      }
    }

    long        file_data_size;
    char* const file_data = LoadFileIntoMemory(full_path.cstr(), &file_data_size);

    // TODO: Check file_data for null lol

    Array<StandardVertex> vertices{no_free_alloc};
    loadObj(no_free_alloc, vertices, file_data, file_data_size);

    free(file_data);

    model.m_NumVertices = (uint32_t)vertices.size();

    bfBufferCreateParams buffer_params;
    buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE;
    buffer_params.allocation.size       = sizeof(StandardVertex) * model.m_NumVertices;
    buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

    model.m_Handle = bfGfxDevice_newBuffer(model.m_GraphicsDevice, &buffer_params);

    void* const vertex_buffer_ptr = bfBuffer_map(model.m_Handle, 0, BIFROST_BUFFER_WHOLE_SIZE);

    std::memcpy(vertex_buffer_ptr, vertices.data(), (size_t)buffer_params.allocation.size);

    bfBuffer_flushRange(model.m_Handle, 0, BIFROST_BUFFER_WHOLE_SIZE);
    bfBuffer_unMap(model.m_Handle);

    return true;
  }
}  // namespace bf
