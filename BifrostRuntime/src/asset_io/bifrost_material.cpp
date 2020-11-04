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
    return true;  // TODO: File doesn't exist on create.
  }

  bool AssetMaterialInfo::save(Engine& engine, ISerializer& serializer)
  {
    (void)engine;

    serializer.serialize(*payload());

    return true;
  }

  bool AssetAnimation3DInfo::load(Engine& engine)
  {
    if (!m_Payload.is<Animation3D>()) 
    {
      m_Payload.set<Animation3D>(engine.assets().memory());
    }

    return true;
  }

  void loadObj(IMemoryManager& temp_allocator, Array<StandardVertex>& out, const char* obj_file_data, std::size_t obj_file_data_length);

  Model::Model(IMemoryManager& memory, bfGfxDeviceHandle device) :
    Base(device),
    m_EmbeddedMaterials{memory},
    m_Meshes{memory},
    m_Nodes{memory},
    m_BoneToModel{memory},
    m_IndexBuffer{nullptr},
    m_VertexBoneData{nullptr},
    m_GlobalInvTransform{}
  {
  }

  void Model::loadAssetSkeleton(const ModelSkeleton& skeleton)
  {
    std::memcpy(&m_GlobalInvTransform, &skeleton.global_inv_transform, sizeof(Matrix4x4));

    m_Nodes.reserve(skeleton.num_nodes);
    m_BoneToModel.reserve(skeleton.num_bones);

    Node* const       dst_nodes = m_Nodes.emplaceN(skeleton.num_nodes);
    NodeIDBone* const dst_bones = m_BoneToModel.emplaceN(skeleton.num_bones);

    std::transform(
     &*skeleton.nodes,
     &*skeleton.nodes + skeleton.num_nodes,
     dst_nodes,
     [](const AssetNode& src_node) -> Node {
       Node dst_node;

       dst_node.name = src_node.name.data;
       std::memcpy(&dst_node.transform, &src_node.transform, sizeof(Matrix4x4));
       dst_node.bone_idx     = src_node.model_to_bone_idx;
       dst_node.first_child  = src_node.first_child;
       dst_node.num_children = src_node.num_children;

       return dst_node;
     });

    std::transform(
     &*skeleton.bones,
     &*skeleton.bones + skeleton.num_bones,
     dst_bones,
     [](const auto& src_bone) -> NodeIDBone {
       NodeIDBone dst_bone;

       dst_bone.node_idx = src_bone.first;

       std::memcpy(&dst_bone.transform, &src_bone.second, sizeof(Matrix4x4));

       return dst_bone;
     });
  }

  void Model::draw(bfGfxCommandListHandle cmd_list)
  {
    uint64_t       buffer_offsets[2] = {0, 0};
    bfBufferHandle buffer_handles[2] = {m_Handle, m_VertexBoneData};

    bfGfxCmdList_bindVertexBuffers(cmd_list, 0, buffer_handles, uint32_t(bfCArraySize(buffer_offsets)), buffer_offsets);
    bfGfxCmdList_bindIndexBuffer(cmd_list, m_IndexBuffer, 0u, BIFROST_INDEX_TYPE_UINT32);

    for (const Mesh& mesh : m_Meshes)
    {
      // TODO(SR): Support binding of various materials per mesh.

      bfGfxCmdList_drawIndexed(cmd_list, mesh.num_indices, mesh.index_offset, 0);
    }
  }

  // TOOD(SR): Make this function also return the length so that the call to 'getTetxureAssetHandle' doesnt have to recalc length.
  static bool makeTexturePath(char (&abs_texture_path)[path::k_MaxLength], StringRange root_dir, const AssetPBRMaterial& src_mat, int type)
  {
    const auto& texture = src_mat.textures[type];

    if (texture)
    {
      const auto [path_length, is_truncated] = path::append(abs_texture_path, bfCArraySize(abs_texture_path), root_dir, StringRange(texture));

      return !is_truncated;
    }

    return false;
  }

  static AssetTextureHandle getTextureAssetHandle(Assets& assets, StringRange file_path)
  {
    const auto [asset_info, is_new] = assets.indexAsset<AssetTextureInfo>(file_path);

    if (asset_info)
    {
      return assets.makeHandleT<AssetTextureHandle>(*asset_info);
    }

    return nullptr;
  }

  template<typename T, typename F>
  static void forEachWithIndex(const AssetTempArray<T>& array, F&& callback)
  {
    std::for_each_n(
     array.data,
     array.length,
     [index = std::size_t(0u), &callback](const T& element) mutable {
       callback(element, index);
       ++index;
     });
  }

  bool AssetModelInfo::load(Engine& engine)
  {
    LinearAllocatorScope    mem_scope0    = engine.tempMemory();
    auto&                   no_free_alloc = engine.tempMemoryNoFree();
    const bfGfxDeviceHandle device        = bfGfxContext_device(engine.renderer().context());
    Model&                  model         = m_Payload.set<Model>(engine.mainMemory(), device);
    const String&           full_path     = filePathAbs();
    const StringRange       file_dir      = path::directory(full_path);
    AssetModelLoadResult    model_result  = loadModel(AssetModelLoadSettings(full_path, no_free_alloc));

    if (model_result)
    {
      Assets& assets                              = engine.assets();
      char    abs_texture_path[path::k_MaxLength] = {'\0'};

      std::size_t name_length;
      char        name_buffer[128];

      const auto assign_texture = [&file_dir, &assets](
                                   AssetTextureHandle& tex_handle,
                                   char(&abs_texture_path)[path::k_MaxLength],
                                   StringRange             root_dir,
                                   const AssetPBRMaterial& src_mat,
                                   int                     type) {
        if (makeTexturePath(abs_texture_path, file_dir, src_mat, PBRTextureType::DIFFUSE))
        {
          tex_handle = getTextureAssetHandle(assets, abs_texture_path);
        }
      };

      model.loadAssetSkeleton(model_result.skeleton);

      forEachWithIndex(*model_result.materials, [&](const AssetPBRMaterial& src_mat, std::size_t index) {
        string_utils::fmtBuffer(name_buffer, sizeof(name_buffer), &name_length, "Material_#%i", int(index));

        StringRange         name          = {name_buffer, name_length};
        const auto          material_info = assets.indexAsset<AssetMaterialInfo>(this, name).info;
        AssetMaterialHandle material      = assets.makeHandleT<AssetMaterialHandle>(*material_info);

        assign_texture(material->m_AlbedoTexture, abs_texture_path, file_dir, src_mat, PBRTextureType::DIFFUSE);
        assign_texture(material->m_NormalTexture, abs_texture_path, file_dir, src_mat, PBRTextureType::NORMAL);
        assign_texture(material->m_MetallicTexture, abs_texture_path, file_dir, src_mat, PBRTextureType::METALLIC);
        assign_texture(material->m_RoughnessTexture, abs_texture_path, file_dir, src_mat, PBRTextureType::ROUGHNESS);
        assign_texture(material->m_AmbientOcclusionTexture, abs_texture_path, file_dir, src_mat, PBRTextureType::AO);
      });

      forEachWithIndex(*model_result.animations, [&](const ModelAnimation& src_animation, std::size_t index) {
        string_utils::fmtBuffer(name_buffer, sizeof(name_buffer), &name_length, "%s", src_animation.name.data);

        StringRange            name           = {name_buffer, name_length};
        const auto             animation_info = assets.indexAsset<AssetAnimation3DInfo>(this, name).info;
        AssetAnimation3DHandle animation      = assets.makeHandleT<AssetAnimation3DHandle>(*animation_info);
        Animation3D&           anim           = *animation_info->payloadT();

        anim.m_Duration       = src_animation.duration;
        anim.m_TicksPerSecond = src_animation.ticks_per_second != 0.0f ? src_animation.ticks_per_second : 25.0f;
        anim.create(std::uint8_t(src_animation.channels->length));

        forEachWithIndex(
         *src_animation.channels,
         [&anim](const ModelAnimationChannel& channel, std::size_t index) {
           auto& dst_channel = anim.m_Channels[index];

           dst_channel.create(
            anim.m_Memory,
            channel.num_rotation_keys,
            channel.num_position_keys,
            channel.num_position_keys,
            channel.num_position_keys,
            channel.num_scale_keys,
            channel.num_scale_keys,
            channel.num_scale_keys);

           std::for_each_n(
            channel.all_keys->data,
            channel.num_position_keys,
            [&dst_channel, i = 0](const AnimationKey& key) mutable {
              dst_channel.translation.x.keys[i] = {key.time, key.data[0]};
              dst_channel.translation.y.keys[i] = {key.time, key.data[1]};
              dst_channel.translation.z.keys[i] = {key.time, key.data[2]};

              ++i;
            });

           std::for_each_n(
            channel.all_keys->data + channel.rotation_key_offset,
            channel.num_rotation_keys,
            [&dst_channel, i = 0](const AnimationKey& key) mutable {
              dst_channel.rotation.keys[i] = {key.time, {key.data[0], key.data[1], key.data[2], key.data[3]}};

              ++i;
            });

           std::for_each_n(
            channel.all_keys->data + channel.scale_key_offset,
            channel.num_scale_keys,
            [&dst_channel, i = 0](const AnimationKey& key) mutable {
              dst_channel.scale.x.keys[i] = {key.time, key.data[0]};
              dst_channel.scale.y.keys[i] = {key.time, key.data[1]};
              dst_channel.scale.z.keys[i] = {key.time, key.data[2]};

              ++i;
            });

           anim.m_NameToChannel.insert(bfStringRange(channel.name), std::uint8_t(index));
         }

        );
      });

      std::for_each(
       model_result.mesh_list->begin(),
       model_result.mesh_list->end(),
       [&model](const AssetMeshPrototype& mesh_proto) {
         model.m_Meshes.push(
          Model::Mesh{mesh_proto.index_offset, mesh_proto.num_indices, nullptr}  // TODO(SR): Material
         );
       });

      /// Vertex / Index Buffer Marshalling

      const auto            num_vertices = (uint32_t)model_result.vertices->length;
      Array<StandardVertex> vertices{no_free_alloc};
      Array<VertexBoneData> bone_vertices{no_free_alloc};

      vertices.resize(num_vertices);
      bone_vertices.resize(num_vertices);

      std::transform(
       model_result.vertices->begin(),
       model_result.vertices->end(),
       vertices.begin(),
       [write_bone_data = bone_vertices.data()](const AssetModelVertex& vertex) mutable -> StandardVertex {
         StandardVertex r /* = {}*/;

         r.pos     = Vector3f{vertex.position[0], vertex.position[1], vertex.position[2], 1.0f};
         r.normal  = Vector3f{vertex.normal[0], vertex.normal[1], vertex.normal[2], 0.0f};
         r.tangent = Vector3f{vertex.tangent[0], vertex.tangent[1], vertex.tangent[2], 0.0f};
         r.color   = bfColor4u_fromUint32(Vec3f_toColor(Vector3f{vertex.color[0], vertex.color[1], vertex.color[2], vertex.color[3]}));
         r.uv      = Vec2f{vertex.uv[0], vertex.uv[1]};

         VertexBoneData& out_bone_data = *write_bone_data;

         for (int i = 0; i < k_GfxMaxVertexBones; ++i)
         {
           out_bone_data.bone_idx[i]     = vertex.bone_indices[i];
           out_bone_data.bone_weights[i] = vertex.bone_weights[i];
         }

         ++write_bone_data;

         return r;
       });

      // TODO(SR): Staging buffer should be used here.

      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE;

      ////////

      buffer_params.allocation.size = sizeof(StandardVertex) * num_vertices;
      buffer_params.usage           = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

      model.m_Handle = bfGfxDevice_newBuffer(model.m_GraphicsDevice, &buffer_params);

      void* const vertex_buffer_ptr = bfBuffer_map(model.m_Handle, 0, BIFROST_BUFFER_WHOLE_SIZE);

      std::memcpy(vertex_buffer_ptr, vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(model.m_Handle, 0, BIFROST_BUFFER_WHOLE_SIZE);
      bfBuffer_unMap(model.m_Handle);

      ///////

      buffer_params.allocation.size = sizeof(std::uint32_t) * model_result.indices->length;
      buffer_params.usage           = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_INDEX_BUFFER;

      model.m_IndexBuffer = bfGfxDevice_newBuffer(model.m_GraphicsDevice, &buffer_params);

      void* const index_buffer_ptr = bfBuffer_map(model.m_IndexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);

      std::memcpy(index_buffer_ptr, model_result.indices->data, buffer_params.allocation.size);

      bfBuffer_flushRange(model.m_IndexBuffer, 0, BIFROST_BUFFER_WHOLE_SIZE);
      bfBuffer_unMap(model.m_IndexBuffer);

      ///////

      buffer_params.allocation.size = sizeof(VertexBoneData) * num_vertices;
      buffer_params.usage           = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

      model.m_VertexBoneData = bfGfxDevice_newBuffer(model.m_GraphicsDevice, &buffer_params);

      void* const bone_buffer_ptr = bfBuffer_map(model.m_VertexBoneData, 0, BIFROST_BUFFER_WHOLE_SIZE);

      std::memcpy(bone_buffer_ptr, bone_vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(model.m_VertexBoneData, 0, BIFROST_BUFFER_WHOLE_SIZE);
      bfBuffer_unMap(model.m_VertexBoneData);

      return true;
    }

    return false;
  }
}  // namespace bf
