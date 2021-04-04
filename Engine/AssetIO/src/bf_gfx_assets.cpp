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
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#include "bf/asset_io/bf_gfx_assets.hpp"

#include "bf/asset_io/bf_document.hpp"
#include "bf/asset_io/bf_iasset_importer.hpp"
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
  static const bfTextureSamplerProperties k_SamplerNearestRepeat = bfTextureSamplerProperties_init(BF_SFM_NEAREST, BF_SAM_CLAMP_TO_EDGE);

  // TODO(SR): We dont want these. \/
  LinearAllocator&  ENGINE_TEMP_MEM(Engine& engine);
  bfGfxDeviceHandle ENGINE_GFX_DEVICE(Engine& engine);
  // TODO(SR): We dont want these. /\


  TextureAsset* TextureAsset::load(IMemoryManager& memory, StringRange path, Engine& engine)
  {
    TextureAsset* const         asset             = memory.allocateT<TextureAsset>();
    auto                        device            = ENGINE_GFX_DEVICE(engine);
    const bfTextureCreateParams tex_create_params = bfTextureCreateParams_init2D(
     BF_IMAGE_FORMAT_R8G8B8A8_UNORM,
     k_bfTextureUnknownSize,
     k_bfTextureUnknownSize);

    asset->m_TextureHandle = bfGfxDevice_newTexture(device, &tex_create_params);

    if (bfTexture_loadFile(asset->m_TextureHandle, path.begin()))
    {
      bfTexture_setSampler(asset->m_TextureHandle, &k_SamplerNearestRepeat);
    }

    return asset;
  }

  void TextureAsset::unload(IMemoryManager& memory, TextureAsset* asset, Engine& engine)
  {
    if (asset->m_TextureHandle)
    {
      auto device = ENGINE_GFX_DEVICE(engine);

      // TODO(SR): This will not scale well.
      bfGfxDevice_flush(device);
      bfGfxDevice_release(device, asset->m_TextureHandle);

      memory.deallocateT(asset);
    }
  }

  TextureAsset::TextureAsset() :
    Base(),
    m_TextureHandle{nullptr}
  {
  }

  AssetStatus TextureDocument::onLoad()
  {
    const String&               full_path         = fullPath();
    const bfTextureCreateParams tex_create_params = bfTextureCreateParams_init2D(
     BF_IMAGE_FORMAT_R8G8B8A8_UNORM,
     k_bfTextureUnknownSize,
     k_bfTextureUnknownSize);

    auto device = ENGINE_GFX_DEVICE(assets().engine());

    TextureAsset* const asset = addAsset<TextureAsset>(ResourceID{1u}, relativePath());

    m_TextureAsset = asset;

    asset->m_TextureHandle = bfGfxDevice_newTexture(device, &tex_create_params);

    if (bfTexture_loadFile(asset->m_TextureHandle, full_path.c_str()))
    {
      bfTexture_setSampler(asset->m_TextureHandle, &k_SamplerNearestRepeat);

      return AssetStatus::LOADED;
    }

    return AssetStatus::FAILED;
  }

  void TextureDocument::onUnload()
  {
    TextureAsset* const asset = m_TextureAsset;

    if (asset->m_TextureHandle)
    {
      auto device = ENGINE_GFX_DEVICE(assets().engine());

      // TODO(SR): This will not scale well.
      bfGfxDevice_flush(device);
      bfGfxDevice_release(device, asset->m_TextureHandle);
    }

    m_TextureAsset = nullptr;
  }

  void TextureDocument::onSaveMeta(ISerializer& serializer)
  {
  }

  AssetStatus MaterialDocument::onLoad()
  {
    File file_in{fullPath(), file::FILE_MODE_READ};

    if (file_in)
    {
      LinearAllocator&     allocator  = ENGINE_TEMP_MEM(assets().engine());
      LinearAllocatorScope mem_scope  = allocator;
      const BufferRange      buffer     = file_in.readEntireFile(allocator);
      json::Value          json_value = json::parse(buffer.buffer, buffer.length);
      JsonSerializerReader reader     = {assets(), allocator, json_value};

      if (reader.beginDocument())
      {
        m_MaterialAsset = addAsset<MaterialAsset>(ResourceID{1u}, relativePath());

        m_MaterialAsset->reflect(reader);
        reader.endDocument();

        return AssetStatus::LOADED;
      }
    }

    return AssetStatus::FAILED;
  }

  void MaterialDocument::onUnload()
  {
    if (m_MaterialAsset)
    {
      m_MaterialAsset->clear();
      m_MaterialAsset = nullptr;
    }
  }

  void MaterialDocument::onSaveAsset()
  {
    defaultSave([this](ISerializer& serialzier) {
      m_MaterialAsset->reflect(serialzier);
    });
  }

  ModelAsset::ModelAsset(IMemoryManager& memory) :
    m_GraphicsDevice{nullptr},
    m_VertexBuffer{nullptr},
    m_IndexBuffer{nullptr},
    m_VertexBoneData{nullptr},
    m_Meshes{memory},
    m_Nodes{memory},
    m_BoneToModel{memory},
    m_Materials{memory},
    m_GlobalInvTransform{},
    m_ObjectSpaceBounds{},
    m_Triangles{memory},
    m_Vertices{memory}
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
       NodeIDBone dst_bone;  // NOLINT(cppcoreguidelines-pro-type-member-init)

       dst_bone.node_idx  = src_bone.first;
       dst_bone.transform = src_bone.second;

       return dst_bone;
     });
  }

  static bool makeTexturePath(char (&abs_texture_path)[path::k_MaxLength], StringRange root_dir, const AssetPBRMaterial& src_mat, int type)
  {
    const auto& texture = src_mat.textures[type];

    if (texture)
    {
      return !path::append(abs_texture_path, bfCArraySize(abs_texture_path), root_dir, StringRange(texture)).is_truncated;
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
    std::for_each_n(array.data, array.length, [index = std::size_t(0u), &callback](const T& element) mutable {
      callback(element, index);
      ++index;
    });
  }

  void ModelAsset::unload()
  {
    // TODO(SR): This will not scale well.
    bfGfxDevice_flush(m_GraphicsDevice);

    m_Materials.clear();

    bfGfxDevice_release(m_GraphicsDevice, m_VertexBuffer);
    bfGfxDevice_release(m_GraphicsDevice, m_IndexBuffer);
    bfGfxDevice_release(m_GraphicsDevice, m_VertexBoneData);
  }

  AssetStatus AssimpDocument::onLoad()
  {
    Engine&              engine       = assets().engine();
    LinearAllocatorScope mem_scope0   = ENGINE_TEMP_MEM(engine);
    const String&        full_path    = fullPath();
    const StringRange    file_dir     = path::directory(full_path);
    AssetModelLoadResult model_result = loadModel(AssetModelLoadSettings(full_path, ENGINE_TEMP_MEM(engine)));

    if (model_result)
    {
      auto* model = addAsset<ModelAsset>(ResourceID{1u}, relativePath(), assetMemory());

      m_ModelAsset = model;

      model->m_GraphicsDevice = ENGINE_GFX_DEVICE(assets().engine());

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

      model->m_ObjectSpaceBounds = model_result.object_space_bounds;

      // Load Skeleton
      model->loadSkeleton(model_result.skeleton);

      // Load Materials
      forEachWithIndex(*model_result.materials, [&](const AssetPBRMaterial& src_mat, std::size_t index) {
        string_utils::fmtBuffer(name_buffer, sizeof(name_buffer), &name_length, "Material_#%i.material", int(index));

        ResourceID material_id = {k_AssetIDMaterialFlag | std::uint32_t(index)};
        auto*      material    = addAsset<MaterialAsset>(material_id, {name_buffer, name_length});

        assign_texture(material->m_AlbedoTexture, src_mat, PBRTextureType::DIFFUSE);
        assign_texture(material->m_NormalTexture, src_mat, PBRTextureType::NORMAL);
        assign_texture(material->m_MetallicTexture, src_mat, PBRTextureType::METALLIC);
        assign_texture(material->m_RoughnessTexture, src_mat, PBRTextureType::ROUGHNESS);
        assign_texture(material->m_AmbientOcclusionTexture, src_mat, PBRTextureType::AO);

        model->m_Materials.push(material);
      });

      // Load Animations
      forEachWithIndex(*model_result.animations, [&](const ModelAnimation& src_animation, std::size_t anim_index) {
        string_utils::fmtBuffer(name_buffer, sizeof(name_buffer), &name_length, "%s#_%i.anim", src_animation.name.data, int(anim_index));

        ResourceID  animation_id = {k_AssetIDAnimationFlag | std::uint32_t(anim_index)};
        auto* const animation    = addAsset<Anim3DAsset>(animation_id, {name_buffer, name_length}, assetMemory());

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
       [model](const Mesh& mesh_proto) {
         model->m_Meshes.push(Mesh{mesh_proto.index_offset, mesh_proto.num_indices, mesh_proto.material_idx});
       });

      /// Vertex / Index Buffer Marshalling

      const uint32_t         num_vertices = uint32_t(model_result.vertices->length);
      Array<StandardVertex>& vertices     = model->m_Vertices;
      Array<VertexBoneData>  bone_vertices{ENGINE_TEMP_MEM(engine)};

      model->m_Vertices.clear();
      model->m_Triangles.clear();

      vertices.resize(num_vertices);
      bone_vertices.resize(num_vertices);
      model->m_Triangles.resize(model_result.indices->length);
      std::memcpy(model->m_Triangles.data(), model_result.indices->data, sizeof(AssetIndexType) * model_result.indices->length);

      std::transform(
       model_result.vertices->begin(),
       model_result.vertices->end(),
       vertices.begin(),
       [write_bone_data = bone_vertices.data()](const AssetModelVertex& vertex) mutable -> StandardVertex {
         StandardVertex r /* = {}*/;

         r.pos     = vertex.position;
         r.normal  = vertex.normal;
         r.tangent = vertex.tangent;
         r.color   = bfColor4u_fromColor4f(vertex.color);
         r.uv      = vertex.uv;

         VertexBoneData& out_bone_data = *write_bone_data;

         for (int i = 0; i < int(k_GfxMaxVertexBones); ++i)
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

      model->m_VertexBuffer = bfGfxDevice_newBuffer(model->m_GraphicsDevice, &buffer_params);

      void* const vertex_buffer_ptr = bfBuffer_map(model->m_VertexBuffer, 0, k_bfBufferWholeSize);

      std::memcpy(vertex_buffer_ptr, vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(model->m_VertexBuffer, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(model->m_VertexBuffer);

      ///////

      buffer_params.allocation.size = sizeof(AssetIndexType) * model_result.indices->length;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

      model->m_IndexBuffer = bfGfxDevice_newBuffer(model->m_GraphicsDevice, &buffer_params);

      void* const index_buffer_ptr = bfBuffer_map(model->m_IndexBuffer, 0, k_bfBufferWholeSize);

      std::memcpy(index_buffer_ptr, model_result.indices->data, buffer_params.allocation.size);

      bfBuffer_flushRange(model->m_IndexBuffer, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(model->m_IndexBuffer);

      ///////

      buffer_params.allocation.size = sizeof(VertexBoneData) * num_vertices;
      buffer_params.usage           = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

      model->m_VertexBoneData = bfGfxDevice_newBuffer(model->m_GraphicsDevice, &buffer_params);

      void* const bone_buffer_ptr = bfBuffer_map(model->m_VertexBoneData, 0, k_bfBufferWholeSize);

      std::memcpy(bone_buffer_ptr, bone_vertices.data(), buffer_params.allocation.size);

      bfBuffer_flushRange(model->m_VertexBoneData, 0, k_bfBufferWholeSize);
      bfBuffer_unMap(model->m_VertexBoneData);

      return AssetStatus::LOADED;
    }

    return AssetStatus::FAILED;
  }

  void AssimpDocument::onUnload()
  {
    m_ModelAsset->unload();
  }

  void assetImportTexture(AssetImportCtx& ctx)
  {
    ctx.document = ctx.asset_memory->allocateT<TextureDocument>();
  }

  void assetImportMaterial(AssetImportCtx& ctx)
  {
    ctx.document = ctx.asset_memory->allocateT<MaterialDocument>();
  }

  void assetImportModel(AssetImportCtx& ctx)
  {
    ctx.document = ctx.asset_memory->allocateT<AssimpDocument>();
  }
}  // namespace bf

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2020-2021 Shareef Abdoul-Raheem

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
