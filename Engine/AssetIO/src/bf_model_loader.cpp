#include "bf/asset_io/bf_model_loader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cassert>  // assert

#define DEBUG_PRINTOUT 0

namespace bf
{
  static char* stringRangeToString(IMemoryManager& memory, bfStringRange str) noexcept
  {
    const std::size_t str_length         = str.end - str.bgn;
    char*             nul_terminated_str = reinterpret_cast<char*>(memory.allocate(str_length + 1u));

    std::memcpy(nul_terminated_str, str.bgn, str_length);
    nul_terminated_str[str_length] = '\0';

    return nul_terminated_str;
  }

  static void aiVector3ToArray(const aiVector3D* from, Vector3f& to)
  {
    to.x = from->x;
    to.y = from->y;
    to.z = from->z;
  }

  static void aiColor4DToArray(const aiColor4D* from, bfColor4f& to)
  {
    to.r = from->r;
    to.g = from->g;
    to.b = from->b;
    to.a = from->a;
  }

  static void aiMat4x4ToMatrix4x4(const aiMatrix4x4* from_, Matrix4x4f& to, bool is_row_major)
  {
    static_assert(sizeof(aiMatrix4x4) == sizeof(Matrix4x4f), "Incorrect size of matrix from Assimp");

    aiMatrix4x4 from = *from_;

    if (!is_row_major)
    {
      from.Transpose();
    }

    std::memcpy(&to, &from, sizeof(from));
  }

  static unsigned findAssetNode(ModelSkeleton& skeleton, const AssetNode** nodes, const aiString& name)
  {
    auto& num_bones = skeleton.num_bones;

    for (std::uint8_t i = 0; i < num_bones; ++i)
    {
      if (std::strcmp(nodes[i]->name.data, name.data) == 0)
      {
        return i;
      }
    }

    for (unsigned i = 0; i < skeleton.num_nodes; ++i)
    {
      if (std::strcmp(skeleton.nodes[i].name.data, name.data) == 0)
      {
        const unsigned id = num_bones++;

        assert(num_bones < k_MaxBones && "Too Many Bones");

        skeleton.bones[id].first            = i;
        skeleton.nodes[i].model_to_bone_idx = id;
        nodes[id]                           = &skeleton.nodes[i];

        return id;
      }
    }

    assert(!"Could not find associated bone.");
    return -1;
  }

  static void addBoneDataToVertex(AssetModelVertex* vertex, float weight, int bone_index)
  {
    for (int i = 0; i < k_MaxVertexBones; ++i)
    {
      if (vertex->bone_weights[i] == 0.0f)
      {
        vertex->bone_weights[i] = weight;
        vertex->bone_indices[i] = bone_index;
        return;
      }
    }

    assert(!"No enough slots for bone data.");
  }

  template<typename F>
  static void recurseNodes(aiNode* parent_node, aiNode* node, F&& callback, int depth, unsigned int parent_index, int& num_nodes)
  {
#if 0 /* Depth Order */
    callback(parent_node, node, depth);

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
      recurseNodes(node, node->mChildren[i], std::forward<F>(callback), depth + 1);
    }
#else /* Level Order */
    const int base_index = num_nodes;

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
      callback(parent_node, node->mChildren[i], depth, parent_index);
      ++num_nodes;
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
      recurseNodes(node, node->mChildren[i], callback, depth + 1, base_index + i, num_nodes);
    }
#endif
  }

  template<typename F>
  static void recurseNodes(aiNode* node, F&& callback)
  {
#if 0 /* Depth */
    recurseNodes(nullptr, node, std::forward<F>(callback), 0);
#else /* Level */

    int num_nodes = 0;

    callback(nullptr, node, 0, -1);
    ++num_nodes;

    recurseNodes(nullptr, node, callback, 1, 0, num_nodes);
#endif
  }

#if DEBUG_PRINTOUT
  static void PrintAssetNode(AssetNode* base_array, AssetNode& node, int depth, int& count)
  {
    for (unsigned int i = 0; i < node.num_children; ++i)
    {
      auto& child = base_array[i + node.first_child];

      for (int j = 0; j < depth * 2; ++j)
      {
        std::printf(" ");
      }

      std::printf("(%i depth: %i, fc: %i, numc: %i) '%s'\n",
                  count,
                  depth,
                  child.first_child,
                  child.num_children,
                  child.name.data);

      ++count;
    }

    for (unsigned int i = 0; i < node.num_children; ++i)
    {
      PrintAssetNode(base_array, base_array[node.first_child + i], depth + 1, count);
    }
  }

  static void PrintAssetNode(AssetNode* base_array, AssetNode& node)
  {
    int depth = 0;
    int count = 0;

    std::printf("(%i depth: %i, fc: %i, numc: %i) '%s'\n",
                count,
                depth,
                node.first_child,
                node.num_children,
                node.name.data);
    ++count;

    PrintAssetNode(base_array, node, depth + 1, count);
  }
#endif

  AssetModelLoadResult loadModel(const AssetModelLoadSettings& load_settings) noexcept
  {
    AssetModelLoadResult result              = {};
    IMemoryManager&      memory              = *load_settings.memory;
    const bfStringRange  file_path           = load_settings.file_path;
    const bool           path_needs_clone    = *file_path.end != '\0';
    const char* const    nul_terminated_path = path_needs_clone ? stringRangeToString(memory, file_path) : file_path.bgn;

    Assimp::Importer importer;

    unsigned int import_flags = aiProcess_Triangulate |
                                aiProcess_SortByPType |
                                aiProcess_JoinIdenticalVertices |
                                aiProcess_LimitBoneWeights |
                                aiProcess_SplitByBoneCount |
                                aiProcess_GenUVCoords |
                                aiProcess_CalcTangentSpace;

    import_flags |= load_settings.smooth_normals ? aiProcess_GenSmoothNormals : aiProcess_GenNormals;

    assert(importer.ValidateFlags(import_flags));

    const aiScene* const scene = importer.ReadFile(nul_terminated_path, import_flags);

    // Error Checking

    if (!scene /* || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE */)
    {
      result.setError(importer.GetErrorString());
      goto done;
    }

    // Main Loading Routine
    {
      static constexpr AssetIndexType k_MaxUint32         = std::numeric_limits<AssetIndexType>::max();
      static constexpr AssetIndexType k_NumIndicesPerFace = 3;

      result.memory = &memory;

      const AssetNode* bone_to_node[k_MaxBones] = {nullptr};

      // Process Nodes
      {
        unsigned int num_nodes = 0;

        recurseNodes(
         scene->mRootNode,
         [&num_nodes](const aiNode* parent_node, const aiNode* node, int depth, int parent_index) {

#if DEBUG_PRINTOUT
           for (int i = 0; i < depth * 2; ++i)
           {
             std::printf(" ");
           }

           std::printf("(depth:%i, #%i, mesh0: %i, %i) '%s'\n",
                       depth,
                       num_nodes,
                       node->mNumMeshes ? node->mMeshes[0] : -1,
                       node->mNumChildren,
                       node->mName.data);

#endif

           ++num_nodes;
         });

#if DEBUG_PRINTOUT
        for (int i = 0; i < 80; ++i)
          std::printf("-");
        std::printf("\n");
#endif

        auto global_inv = scene->mRootNode->mTransformation;
        global_inv.Inverse();

        aiMat4x4ToMatrix4x4(&global_inv, result.skeleton.global_inv_transform, load_settings.row_major);
        result.skeleton.num_nodes = num_nodes;
        result.skeleton.nodes     = memory.allocateArray<AssetNode>(num_nodes);

        recurseNodes(
         scene->mRootNode,
         [&load_settings, &skeleton = result.skeleton, node_idx = 0](const aiNode* parent_node, const aiNode* node, int depth, unsigned int parent_index) mutable {
           if (parent_index != static_cast<unsigned int>(-1) && skeleton.nodes[parent_index].first_child == static_cast<unsigned int>(-1))
           {
             skeleton.nodes[parent_index].first_child = node_idx;
           }

           AssetNode& model_node = skeleton.nodes[node_idx];

           aiMat4x4ToMatrix4x4(&node->mTransformation, model_node.transform, load_settings.row_major);
           model_node.name.copyOverString(node->mName.data, node->mName.length);
           model_node.first_child       = -1;
           model_node.num_children      = node->mNumChildren;
           model_node.model_to_bone_idx = -1;

           ++node_idx;
         });

#if DEBUG_PRINTOUT
        PrintAssetNode(result.skeleton.nodes, result.skeleton.nodes[0]);
#endif
      }

      const unsigned int num_meshes = scene->mNumMeshes;

      // Counting up Number of Vertices and Indices and set up Mesh Prototype
      {
        AssetIndexType num_vertices = 0;
        AssetIndexType num_indices  = 0;

        result.mesh_list = detail::makeUniqueTempArray<AssetMeshPrototype>(memory, num_meshes);

        for (unsigned int i = 0; i < num_meshes; ++i)
        {
          const aiMesh* const       mesh              = scene->mMeshes[i];
          AssetMeshPrototype* const mesh_proto        = result.mesh_list->data + i;
          const unsigned int        num_mesh_vertices = mesh->mNumVertices;
          const AssetIndexType      num_mesh_indices  = mesh->mNumFaces * k_NumIndicesPerFace;

          if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
          {
            continue;
          }

          assert((k_MaxUint32 - num_vertices) >= num_mesh_vertices && "Model has too many vertices.");
          assert((k_MaxUint32 - num_indices) >= num_mesh_indices && "Model has too many indices.");

          mesh_proto->index_offset   = num_indices;
          mesh_proto->num_indices    = num_mesh_indices;
          mesh_proto->material_index = mesh->mMaterialIndex;

          num_vertices += num_mesh_vertices;
          num_indices += num_mesh_indices;
        }

        result.vertices = detail::makeUniqueTempArray<AssetModelVertex>(memory, num_vertices);
        result.indices  = detail::makeUniqueTempArray<AssetIndexType>(memory, num_indices);
      }

      // Merging All Vertices Into One Buffer
      {
        AssetIndexType  index_offset  = 0;
        AssetIndexType  vertex_offset = 0;
        AssetIndexType* output_index  = result.indices->data;

        for (unsigned int i = 0; i < num_meshes; ++i)
        {
          const aiMesh* const       mesh              = scene->mMeshes[i];
          AssetMeshPrototype* const mesh_proto        = result.mesh_list->data + i;
          const unsigned int        num_mesh_vertices = mesh->mNumVertices;
          const unsigned int        num_mesh_faces    = mesh->mNumFaces;
          const AssetIndexType      num_mesh_indices  = num_mesh_faces * k_NumIndicesPerFace;

          if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
          {
            continue;
          }

          // Vertices

          for (unsigned int v = 0; v < num_mesh_vertices; ++v)
          {
            AssetModelVertex* const output_vertex = result.vertices->data + vertex_offset + v;

            assert(mesh->mVertices && "Mesh Is Missing Vertex Data");

            const aiVector3D        default_normal  = {1.0f, 0.0f, 0.0f};
            const aiVector3D        default_tangent = {0.0f, 1.0f, 0.0f};
            const aiVector3D        default_bitan   = {0.0f, 0.0f, 1.0f};
            const aiVector3D        default_uv      = {0.0f, 0.0f, 0.0f};
            const aiColor4D         default_color   = {1.0f, 1.0f, 1.0f, 1.0f};
            const aiVector3D* const position        = mesh->mVertices + v;
            const aiVector3D* const normal          = mesh->mNormals ? mesh->mNormals + v : &default_normal;
            const aiVector3D* const tangent         = mesh->mTangents ? mesh->mTangents + v : &default_tangent;
            const aiVector3D* const bitangent       = mesh->mBitangents ? mesh->mBitangents + v : &default_bitan;
            const aiVector3D* const uv              = mesh->mTextureCoords[0] ? mesh->mTextureCoords[0] + v : &default_uv;
            const aiColor4D* const  color           = mesh->mColors[0] ? mesh->mColors[0] + v : &default_color;

            aiVector3ToArray(position, output_vertex->position);
            aiVector3ToArray(normal, output_vertex->normal);
            aiVector3ToArray(tangent, output_vertex->tangent);
            aiVector3ToArray(bitangent, output_vertex->bitangent);
            aiColor4DToArray(color, output_vertex->color);

            output_vertex->uv.x = uv->x;
            output_vertex->uv.y = uv->y;
          }

          // Faces

          for (unsigned int f = 0; f < num_mesh_faces; ++f)
          {
            const aiFace* const face = mesh->mFaces + f;

            assert(face->mNumIndices == 3 && "Only triangles are supported.");

            for (unsigned int face_idx = 0; face_idx < face->mNumIndices; ++face_idx)
            {
              *output_index = face->mIndices[face_idx] + vertex_offset;
              ++output_index;
            }
          }

          // Bones

          for (unsigned int b = 0; b < mesh->mNumBones; ++b)
          {
            const aiBone* const bone       = mesh->mBones[b];
            const unsigned      bone_index = findAssetNode(result.skeleton, bone_to_node, bone->mName);

            if (bone_index != -1)
            {
              aiMat4x4ToMatrix4x4(&bone->mOffsetMatrix, result.skeleton.bones[bone_index].second, load_settings.row_major);

              for (unsigned int bw = 0; bw < bone->mNumWeights; ++bw)
              {
                const auto vertex_index = vertex_offset + bone->mWeights[bw].mVertexId;

                addBoneDataToVertex(
                 result.vertices->data + vertex_index,
                 bone->mWeights[bw].mWeight,
                 bone_index);
              }
            }
            else
            {
              std::printf("WARNING: Missing Model Node For Bone '%s'\n", bone->mName.C_Str());
            }
          }

          vertex_offset += num_mesh_vertices;
          index_offset += num_mesh_indices;
        }
      }

      // Process Materials
      {
        const unsigned int num_materials = scene->mNumMaterials;

        result.materials = detail::makeUniqueTempArray<AssetPBRMaterial>(memory, num_materials);

        const auto load_texture = [&memory](const aiMaterial* material, aiTextureType tex_type) -> AssetTempLargeString {
          if (material->GetTextureCount(tex_type) > 0)
          {
            aiString       tex_path;
            const aiReturn res = material->GetTexture(tex_type, 0, &tex_path, nullptr, nullptr, nullptr, nullptr, nullptr);

            if (res == aiReturn_SUCCESS)
            {
              AssetTempLargeString str;

              str.length = tex_path.length;
              std::memcpy(str.data, tex_path.data, std::size_t(tex_path.length) + 1);

              return str;
            }
          }

          return {0u, ""};
        };

        for (unsigned int i = 0; i < num_materials; ++i)
        {
          const aiMaterial* const material     = scene->mMaterials[i];
          AssetPBRMaterial*       out_material = result.materials->data + i;

          out_material->textures[PBRTextureType::DIFFUSE] = load_texture(material, aiTextureType_BASE_COLOR);

          if (!out_material->textures[PBRTextureType::DIFFUSE])
          {
            out_material->textures[PBRTextureType::DIFFUSE] = load_texture(material, aiTextureType_DIFFUSE);
          }

          if (!out_material->textures[PBRTextureType::DIFFUSE])
          {
            out_material->textures[PBRTextureType::DIFFUSE] = load_texture(material, aiTextureType_AMBIENT);
          }

          out_material->textures[PBRTextureType::NORMAL] = load_texture(material, aiTextureType_NORMAL_CAMERA);

          if (!out_material->textures[PBRTextureType::NORMAL])
          {
            out_material->textures[PBRTextureType::NORMAL] = load_texture(material, aiTextureType_NORMALS);
          }

          if (!out_material->textures[PBRTextureType::NORMAL])
          {
            // WTF Assimp
            out_material->textures[PBRTextureType::NORMAL] = load_texture(material, aiTextureType_HEIGHT);
          }

          out_material->textures[PBRTextureType::METALLIC] = load_texture(material, aiTextureType_METALNESS);

          if (!out_material->textures[PBRTextureType::METALLIC])
          {
            out_material->textures[PBRTextureType::METALLIC] = load_texture(material, aiTextureType_SHININESS);
          }

          out_material->textures[PBRTextureType::ROUGHNESS] = load_texture(material, aiTextureType_DIFFUSE_ROUGHNESS);
          out_material->textures[PBRTextureType::AO]        = load_texture(material, aiTextureType_AMBIENT_OCCLUSION);
        }
      }

      // Animations
      {
        const unsigned int num_animations = scene->mNumAnimations;

        result.animations = detail::makeUniqueTempArray<ModelAnimation>(memory, num_animations);

        for (unsigned int i = 0; i < num_animations; ++i)
        {
          const aiAnimation* const src_animation = scene->mAnimations[i];
          ModelAnimation&          dst_animation = result.animations->data[i];

          dst_animation.name.copyOverString(src_animation->mName.data, src_animation->mName.length);
          dst_animation.duration         = src_animation->mDuration;
          dst_animation.ticks_per_second = src_animation->mTicksPerSecond;
          dst_animation.channels         = allocateTempArray<ModelAnimationChannel>(memory, src_animation->mNumChannels);

          for (unsigned int c = 0; c < src_animation->mNumChannels; ++c)
          {
            const aiNodeAnim* const src_channel    = src_animation->mChannels[c];
            ModelAnimationChannel&  dst_channel    = dst_animation.channels->data[c];
            const std::size_t       total_num_keys = std::size_t(src_channel->mNumPositionKeys) +
                                               std::size_t(src_channel->mNumRotationKeys) +
                                               std::size_t(src_channel->mNumScalingKeys);

            dst_channel.name.copyOverString(src_channel->mNodeName.data, src_channel->mNodeName.length);
            dst_channel.all_keys            = allocateTempArray<AnimationKey>(memory, total_num_keys);
            dst_channel.rotation_key_offset = src_channel->mNumPositionKeys;
            dst_channel.scale_key_offset    = dst_channel.rotation_key_offset + src_channel->mNumRotationKeys;
            dst_channel.num_position_keys   = src_channel->mNumPositionKeys;
            dst_channel.num_rotation_keys   = src_channel->mNumRotationKeys;
            dst_channel.num_scale_keys      = src_channel->mNumScalingKeys;

            unsigned int dst_key_idx = 0;

            for (unsigned int k = 0; k < src_channel->mNumPositionKeys; ++k)
            {
              const aiVectorKey* const src_key = src_channel->mPositionKeys + k;
              AnimationKey&            dst_key = dst_channel.all_keys->data[dst_key_idx++];

              dst_key.time    = src_key->mTime;
              dst_key.data[0] = src_key->mValue.x;
              dst_key.data[1] = src_key->mValue.y;
              dst_key.data[2] = src_key->mValue.z;
              dst_key.data[3] = 1.0f;
            }

            for (unsigned int k = 0; k < src_channel->mNumRotationKeys; ++k)
            {
              const aiQuatKey* const src_key = src_channel->mRotationKeys + k;
              AnimationKey&          dst_key = dst_channel.all_keys->data[dst_key_idx++];

              dst_key.time    = src_key->mTime;
              dst_key.data[0] = src_key->mValue.x;
              dst_key.data[1] = src_key->mValue.y;
              dst_key.data[2] = src_key->mValue.z;
              dst_key.data[3] = src_key->mValue.w;
            }

            for (unsigned int k = 0; k < src_channel->mNumScalingKeys; ++k)
            {
              const aiVectorKey* const src_key = src_channel->mScalingKeys + k;
              AnimationKey&            dst_key = dst_channel.all_keys->data[dst_key_idx++];

              dst_key.time    = src_key->mTime;
              dst_key.data[0] = src_key->mValue.x;
              dst_key.data[1] = src_key->mValue.y;
              dst_key.data[2] = src_key->mValue.z;
              dst_key.data[3] = 0.0f;
            }
          }
        }
      }
    }

    // Cleanup
  done:
    if (path_needs_clone)
    {
      memory.deallocate(const_cast<char*>(nul_terminated_path), (file_path.end - file_path.bgn) + 1);
    }

    return result;
  }

  void AssetModelLoadResult::setError(const char* err_message) noexcept
  {
    constexpr std::size_t k_BufferSizeMinusOne = sizeof(error_buffer) - 1;

    error_buffer[k_BufferSizeMinusOne] = '\0';

    error = bfMakeStringRangeC(std::strncpy(error_buffer.data(), err_message, k_BufferSizeMinusOne));
  }
}  // namespace bf
