/******************************************************************************/
/*!
 * @file   bf_gfx_assets.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Defines some built in Asset Types used mainly by the graphics subsystem(s).
 *
 * @version 0.0.1
 * @date    2020-12-19
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#include "bf/asset_io/bf_gfx_assets.hpp"

// #include "../../Runtime/include/bf/Engine.hpp"
#include "bf/asset_io/bf_model_loader.hpp"
#include "bf/asset_io/bf_path_manip.hpp"
#include "bf/asset_io/bifrost_assets.hpp"
#include "bf/asset_io/bifrost_file.hpp"
#include "bf/asset_io/bifrost_json_serializer.hpp"
#include "bf/graphics/bifrost_standard_renderer.hpp"
#include "bf/utility/bifrost_json.hpp"

namespace bf
{
  // TODO(SR): This is copied from "bifrost_standard_renderer.cpp"
  static const bfTextureSamplerProperties k_SamplerNearestRepeat = bfTextureSamplerProperties_init(BF_SFM_NEAREST, BF_SAM_REPEAT);

    // TODO(SR): We dont want this.
  LinearAllocator& ENGINE_TEMP_MEM(Engine& engine);
  IMemoryManager&  ENGINE_TEMP_MEM_NO_FREE(Engine& engine);

  TextureAsset::TextureAsset(bfGfxDeviceHandle gfx_device) :
    Base(),
    m_ParentDevice{gfx_device},
    m_TextureHandle{nullptr}
  {
  }

  void TextureAsset::onLoad()
  {
    if (loadImpl())
    {
      markIsLoaded();
    }
    else
    {
      markFailedToLoad();
    }
  }

  void TextureAsset::onUnload()
  {
    if (m_TextureHandle)
    {
      // TODO(SR): This will not scale well.
      bfGfxDevice_flush(m_ParentDevice);

      bfGfxDevice_release(m_ParentDevice, m_TextureHandle);
    }
  }

  bool TextureAsset::loadImpl()
  {
    const String&               full_path         = fullPath();
    const bfTextureCreateParams tex_create_params = bfTextureCreateParams_init2D(
     BF_IMAGE_FORMAT_R8G8B8A8_UNORM,
     k_bfTextureUnknownSize,
     k_bfTextureUnknownSize);

    m_TextureHandle = bfGfxDevice_newTexture(m_ParentDevice, &tex_create_params);

    if (bfTexture_loadFile(m_TextureHandle, full_path.c_str()))
    {
      bfTexture_setSampler(m_TextureHandle, &k_SamplerNearestRepeat);

      return true;
    }

    return false;
  }

  MaterialAsset::MaterialAsset() :
    Base(),
    m_AlbedoTexture{nullptr},
    m_NormalTexture{nullptr},
    m_MetallicTexture{nullptr},
    m_RoughnessTexture{nullptr},
    m_AmbientOcclusionTexture{nullptr}
  {
    markAsEngineAsset();
  }

  void MaterialAsset::onLoad()
  {
    File file_in{fullPath(), file::FILE_MODE_READ};

    if (file_in)
    {
      Engine&              engine     = assets().engine();
      LinearAllocator&     allocator  = ENGINE_TEMP_MEM(engine);
      LinearAllocatorScope mem_scope  = allocator;
      const BufferLen      buffer     = file_in.readEntireFile(allocator);
      json::Value          json_value = json::fromString(buffer.buffer, buffer.length);
      JsonSerializerReader reader     = {assets(), ENGINE_TEMP_MEM_NO_FREE(engine), json_value};

      if (reader.beginDocument(false))
      {
        reflect(reader);
        reader.endDocument();
      }
      else
      {
        markFailedToLoad();
      }
    }
    else
    {
      markFailedToLoad();
    }
  }

  void MaterialAsset::onUnload()
  {
    m_AlbedoTexture           = nullptr;
    m_NormalTexture           = nullptr;
    m_MetallicTexture         = nullptr;
    m_RoughnessTexture        = nullptr;
    m_AmbientOcclusionTexture = nullptr;
  }

  ModelAsset::ModelAsset(IMemoryManager& memory, bfGfxDeviceHandle device) :
    m_GraphicsDevice{device},
    m_VertexBuffer{nullptr},
    m_IndexBuffer{nullptr},
    m_VertexBoneData{nullptr},
    m_Meshes{memory},
    m_Nodes{memory},
    m_BoneToModel{memory},
    m_GlobalInvTransform{}
  {
  }

  void ModelAsset::loadSkeleton(const ModelSkeleton& skeleton)
  {
    m_GlobalInvTransform = skeleton.global_inv_transform;

    m_Nodes.clear();
    m_Nodes.reserve(skeleton.num_nodes);

    m_BoneToModel.clear();
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

  // TOOD(SR): Make this function also return the length so that the call to 'getTextureAssetHandle' doesnt have to recalc length.
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

  void ModelAsset::onLoad()
  {
    Engine&                 engine        = assets().engine();
    LinearAllocatorScope    mem_scope0    = ENGINE_TEMP_MEM(engine);
    auto&                   no_free_alloc = ENGINE_TEMP_MEM_NO_FREE(engine);
    const String&           full_path     = fullPath();
    const StringRange       file_dir      = path::directory(full_path);
    AssetModelLoadResult    model_result  = loadModel(AssetModelLoadSettings(full_path, no_free_alloc));

    if (model_result)
    {
      Assets& assets                              = this->assets();
      char    abs_texture_path[path::k_MaxLength] = {'\0'};

      std::size_t name_length;
      char        name_buffer[128];

      const auto assign_texture = [&file_dir, &assets, &abs_texture_path](
                                   ARC<TextureAsset>&      tex_handle,
                                   const AssetPBRMaterial& src_mat,
                                   int                     type) {
        if (makeTexturePath(abs_texture_path, file_dir, src_mat, type))
        {
          tex_handle = getTextureAssetHandle(assets, abs_texture_path);
        }
      };

      // Load Skeleton

      loadSkeleton(model_result.skeleton);

      // Load Materials

      forEachWithIndex(*model_result.materials, [&](const AssetPBRMaterial& src_mat, std::size_t index) {
        string_utils::fmtBuffer(name_buffer, sizeof(name_buffer), &name_length, "Material_#%i.material", int(index));

        const StringRange    name     = {name_buffer, name_length};
        MaterialAsset* const material = static_cast<MaterialAsset*>(findOrCreateSubAsset(name));

        assign_texture(material->m_AlbedoTexture, src_mat, PBRTextureType::DIFFUSE);
        assign_texture(material->m_NormalTexture, src_mat, PBRTextureType::NORMAL);
        assign_texture(material->m_MetallicTexture, src_mat, PBRTextureType::METALLIC);
        assign_texture(material->m_RoughnessTexture, src_mat, PBRTextureType::ROUGHNESS);
        assign_texture(material->m_AmbientOcclusionTexture, src_mat, PBRTextureType::AO);
      });

      // Load Animations

      forEachWithIndex(*model_result.animations, [&](const ModelAnimation& src_animation, std::size_t anim_index) {
        string_utils::fmtBuffer(name_buffer, sizeof(name_buffer), &name_length, "ANIM_%s#%i.anim", src_animation.name.data, int(anim_index));

        const StringRange  name      = {name_buffer, name_length};
        Anim3DAsset* const animation = static_cast<Anim3DAsset*>(findOrCreateSubAsset(name));

        animation->m_Duration       = src_animation.duration;
        animation->m_TicksPerSecond = src_animation.ticks_per_second != 0.0 ? src_animation.ticks_per_second : 25.0;
        animation->create(std::uint8_t(src_animation.channels->length));

        forEachWithIndex(
         *src_animation.channels,
         [animation](const ModelAnimationChannel& channel, std::size_t anim_channel_index) {
           auto& dst_channel = animation->m_Channels[anim_channel_index];

           dst_channel.create(
            animation->m_Memory,
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

           animation->m_NameToChannel.insert(bfStringRange(channel.name), std::uint8_t(anim_channel_index));
         });
      });

      // Load Meshes

      std::for_each(
       model_result.mesh_list->begin(),
       model_result.mesh_list->end(),
       [this](const Mesh& mesh_proto) {
         m_Meshes.push(
          Mesh{mesh_proto.index_offset, mesh_proto.num_indices, mesh_proto.material_idx}
         );
       });

      /// Vertex / Index Buffer Marshalling

      const uint32_t        num_vertices = uint32_t(model_result.vertices->length);
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

      m_VertexBuffer = bfGfxDevice_newBuffer(m_GraphicsDevice, &buffer_params);

      void* const vertex_buffer_ptr = bfBuffer_map(m_VertexBuffer, 0, k_bfBufferWholeSize);

      std::memcpy(vertex_buffer_ptr, vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(m_VertexBuffer, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(m_VertexBuffer);

      ///////

      buffer_params.allocation.size = sizeof(std::uint32_t) * model_result.indices->length;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

      m_IndexBuffer = bfGfxDevice_newBuffer(m_GraphicsDevice, &buffer_params);

      void* const index_buffer_ptr = bfBuffer_map(m_IndexBuffer, 0, k_bfBufferWholeSize);

      std::memcpy(index_buffer_ptr, model_result.indices->data, buffer_params.allocation.size);

      bfBuffer_flushRange(m_IndexBuffer, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(m_IndexBuffer);

      ///////

      buffer_params.allocation.size = sizeof(VertexBoneData) * num_vertices;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      m_VertexBoneData = bfGfxDevice_newBuffer(m_GraphicsDevice, &buffer_params);

      void* const bone_buffer_ptr = bfBuffer_map(m_VertexBoneData, 0, k_bfBufferWholeSize);

      std::memcpy(bone_buffer_ptr, bone_vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(m_VertexBoneData, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(m_VertexBoneData);
    }
    else
    {
      markFailedToLoad();
    }
  }

  void ModelAsset::onUnload()
  {
    // TODO(SR): This will not scale well.
    bfGfxDevice_flush(m_GraphicsDevice);

    bfGfxDevice_release(m_GraphicsDevice, m_VertexBuffer);
    bfGfxDevice_release(m_GraphicsDevice, m_IndexBuffer);
    bfGfxDevice_release(m_GraphicsDevice, m_VertexBoneData);
  }
}  // namespace bf

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
