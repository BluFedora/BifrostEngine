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
#include "bf/asset_io/bifrost_material.hpp"

#include "bf/asset_io/bf_model_loader.hpp"
#include "bf/asset_io/bf_path_manip.hpp"
#include "bf/asset_io/bifrost_file.hpp"
#include "bf/asset_io/bifrost_json_serializer.hpp"
#include "bf/core/bifrost_engine.hpp"
#include "bf/utility/bifrost_json.hpp"

namespace bf
{
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
    m_GraphicsDevice{device},
    m_Handle{nullptr},
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
    m_GlobalInvTransform = skeleton.global_inv_transform;

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

       dst_node.name         = src_node.name.data;
       dst_node.transform    = src_node.transform;
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

       dst_bone.node_idx  = src_bone.first;
       dst_bone.transform = src_bone.second;

       return dst_bone;
     });
  }

  void Model::draw(bfGfxCommandListHandle cmd_list)
  {
    uint64_t       buffer_offsets[2] = {0, 0};
    bfBufferHandle buffer_handles[2] = {m_Handle, m_VertexBoneData};

    bfGfxCmdList_bindVertexBuffers(cmd_list, 0, buffer_handles, uint32_t(bfCArraySize(buffer_offsets)), buffer_offsets);
    bfGfxCmdList_bindIndexBuffer(cmd_list, m_IndexBuffer, 0u, BF_INDEX_TYPE_UINT32);

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

  static ARC<TextureAsset> getTextureAssetHandle(Assets& assets, StringRange file_path)
  {
    return assets.findAssetOfType<TextureAsset>(AbsPath(file_path));
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
                                   ARC<TextureAsset>& tex_handle,
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

         r.pos     = vertex.position;
         r.normal  = vertex.normal;
         r.tangent = vertex.tangent;
         r.color   = bfColor4u_fromUint32(Vec3f_toColor(Vector3f{vertex.color.r, vertex.color.g, vertex.color.b, vertex.color.a}));
         r.uv      = vertex.uv;

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
      buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE;

      ////////

      buffer_params.allocation.size = sizeof(StandardVertex) * num_vertices;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      model.m_Handle = bfGfxDevice_newBuffer(model.m_GraphicsDevice, &buffer_params);

      void* const vertex_buffer_ptr = bfBuffer_map(model.m_Handle, 0, k_bfBufferWholeSize);

      std::memcpy(vertex_buffer_ptr, vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(model.m_Handle, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(model.m_Handle);

      ///////

      buffer_params.allocation.size = sizeof(std::uint32_t) * model_result.indices->length;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

      model.m_IndexBuffer = bfGfxDevice_newBuffer(model.m_GraphicsDevice, &buffer_params);

      void* const index_buffer_ptr = bfBuffer_map(model.m_IndexBuffer, 0, k_bfBufferWholeSize);

      std::memcpy(index_buffer_ptr, model_result.indices->data, buffer_params.allocation.size);

      bfBuffer_flushRange(model.m_IndexBuffer, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(model.m_IndexBuffer);

      ///////

      buffer_params.allocation.size = sizeof(VertexBoneData) * num_vertices;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      model.m_VertexBoneData = bfGfxDevice_newBuffer(model.m_GraphicsDevice, &buffer_params);

      void* const bone_buffer_ptr = bfBuffer_map(model.m_VertexBoneData, 0, k_bfBufferWholeSize);

      std::memcpy(bone_buffer_ptr, bone_vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(model.m_VertexBoneData, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(model.m_VertexBoneData);

      return true;
    }

    return false;
  }
}  // namespace bf
