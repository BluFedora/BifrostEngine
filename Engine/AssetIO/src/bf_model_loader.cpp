#include "bf/asset_io/bf_model_loader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cassert>  // assert

namespace bf
{
  static char* stringRangeToString(IMemoryManager& memory, bfStringRange str) noexcept
  {
    const std::size_t str_length         = str.str_end - str.str_bgn;
    char*             nul_terminated_str = static_cast<char*>(memory.allocate(str_length + 1u));

    std::memcpy(nul_terminated_str, str.str_bgn, str_length);
    nul_terminated_str[str_length] = '\0';

    return nul_terminated_str;
  }

  static void aiVector3ToArray(const aiVector3D* from, Vector3f& to)
  {
    to.x = from->x;
    to.y = from->y;
    to.z = from->z;
    to.w = 1.0f;
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
      if (std::strcmp(skeleton.nodes[int(i)].name.data, name.data) == 0)
      {
        const unsigned id = num_bones++;

        assert(num_bones < k_MaxBones && "Too Many Bones");

        skeleton.bones[id].first                 = i;
        skeleton.nodes[int(i)].model_to_bone_idx = std::uint8_t(id);
        nodes[id]                                = &skeleton.nodes[int(i)];

        return id;
      }
    }

    assert(!"Could not find associated bone.");  // NOLINT(clang-diagnostic-string-conversion)
    return unsigned(-1);
  }

  static void addBoneDataToVertex(AssetModelVertex* vertex, float weight, std::uint8_t bone_index)
  {
    for (std::size_t i = 0; i < k_MaxVertexBones; ++i)
    {
      if (vertex->bone_weights[i] == 0.0f)
      {
        vertex->bone_weights[i] = weight;
        vertex->bone_indices[i] = bone_index;
        return;
      }
    }

    assert(!"No enough slots for bone data.");  // NOLINT(clang-diagnostic-string-conversion)
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
#if 0 /* Depth Order */
    recurseNodes(nullptr, node, std::forward<F>(callback), 0);
#else /* Level Order */

    int num_nodes = 0;

    callback(nullptr, node, 0, -1);
    ++num_nodes;

    recurseNodes(nullptr, node, callback, 1, 0, num_nodes);
#endif
  }

  AABB::AABB(const bfTransform& transform)
  {
    Vector3f coords[] = {
     {+0.5f, -0.5f, -0.5f, 1.0f},
     {-0.5f, +0.5f, -0.5f, 1.0f},
     {-0.5f, -0.5f, +0.5f, 1.0f},

     {+0.5f, +0.5f, -0.5f, 1.0f},
     {-0.5f, +0.5f, +0.5f, 1.0f},
     {+0.5f, -0.5f, +0.5f, 1.0f},

     {-0.5f, -0.5f, -0.5f, 1.0f},
     {+0.5f, +0.5f, +0.5f, 1.0f},
    };

    for (Vector3f& point : coords)
    {
      Vec3f_mulMat(&point, &transform.world_transform);
    }

    *this = aabb::fromPoints(coords, std::size(coords));
  }

  AABB aabb::transform(const AABB& aabb, const Mat4x4& matrix)
  {
#if 0
      Vector3f coords[] = {
       {aabb.max[0], aabb.min[1], aabb.min[2], 1.0f},
       {aabb.min[0], aabb.max[1], aabb.min[2], 1.0f},
       {aabb.min[0], aabb.min[1], aabb.max[2], 1.0f},
       {aabb.max[0], aabb.max[1], aabb.min[2], 1.0f},
       {aabb.min[0], aabb.max[1], aabb.max[2], 1.0f},
       {aabb.max[0], aabb.min[1], aabb.max[2], 1.0f},
       {aabb.min[0], aabb.min[1], aabb.min[2], 1.0f},
       {aabb.max[0], aabb.max[1], aabb.max[2], 1.0f},
      };

      for (Vector3f& point : coords)
      {
        Vec3f_mulMat(&point, &matrix);
      }

      return fromPoints(coords, std::size(coords));
#else
    Vector3f     new_center = aabb.center();
    Vector3f     new_extent = aabb.extents();
    const Mat4x4 abs_mat    = bfMat4x4f_abs(&matrix);

    Mat4x4_multVec(&matrix, &new_center, &new_center);
    Mat4x4_multVec(&abs_mat, &new_extent, &new_extent);

    return AABB{
     new_center - new_extent,
     new_center + new_extent,
    };
#endif
  }

  AssetModelLoadResult loadModel(const AssetModelLoadSettings& load_settings) noexcept
  {
#if _DEBUG
    static constexpr std::uint32_t k_MaxUint32 = std::numeric_limits<std::uint32_t>::max();
#endif

    AssetModelLoadResult result              = {};
    IMemoryManager&      memory              = *load_settings.memory;
    const bfStringRange  file_path           = load_settings.file_path;
    const bool           path_needs_clone    = *file_path.str_end != '\0';
    const char* const    nul_terminated_path = path_needs_clone ? stringRangeToString(memory, file_path) : file_path.str_bgn;

    Assimp::Importer importer;

    unsigned int import_flags = aiProcess_Triangulate |
                                aiProcess_SortByPType |
                                aiProcess_JoinIdenticalVertices |
                                aiProcess_LimitBoneWeights |
                                aiProcess_SplitByBoneCount |
                                aiProcess_GenUVCoords |
                                aiProcess_CalcTangentSpace |
                                aiProcess_OptimizeMeshes |
                                aiProcess_GlobalScale;

    import_flags |= load_settings.smooth_normals ? aiProcess_GenSmoothNormals : aiProcess_GenNormals;

    assert(importer.ValidateFlags(import_flags));

    const aiScene* const scene = importer.ReadFile(nul_terminated_path, import_flags);

    // Error Checking

    if (!scene /* || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE */)
    {
      result.setError(importer.GetErrorString());
      goto done;  // NOLINT(cppcoreguidelines-avoid-goto, hicpp-avoid-goto)
    }

    // Main Loading Routine
    {
      static constexpr AssetIndexType k_NumIndicesPerFace = 3;

      result.memory = &memory;

      const AssetNode* bone_to_node[k_MaxBones] = {nullptr};

      // Process Nodes
      {
        unsigned int num_nodes = 0;

        recurseNodes(
         scene->mRootNode,
         [&num_nodes](const aiNode*, const aiNode*, int, int) {
           ++num_nodes;
         });

        result.skeleton.num_nodes = num_nodes;
        result.skeleton.nodes     = memory.allocateArray<AssetNode>(num_nodes);

        recurseNodes(
         scene->mRootNode,
         [&load_settings, &skeleton = result.skeleton, node_idx = 0](const aiNode* parent_node, const aiNode* node, int depth, unsigned int parent_index) mutable {
           if (parent_index != static_cast<unsigned int>(-1) && skeleton.nodes[int(parent_index)].first_child == static_cast<unsigned int>(-1))
           {
             skeleton.nodes[int(parent_index)].first_child = node_idx;
           }

           AssetNode& model_node = skeleton.nodes[node_idx];

           aiMat4x4ToMatrix4x4(&node->mTransformation, model_node.transform, load_settings.row_major);
           model_node.name.copyOverString(node->mName.data, node->mName.length);
           model_node.first_child       = -1;
           model_node.num_children      = node->mNumChildren;
           model_node.model_to_bone_idx = -1;

           ++node_idx;
         });
      }

      const unsigned int num_meshes = scene->mNumMeshes;

      // Counting up Number of Vertices and Indices and set up Mesh Prototype
      {
        AssetIndexType num_vertices = 0;
        AssetIndexType num_indices  = 0;

        result.mesh_list = detail::makeUniqueTempArray<Mesh>(memory, num_meshes);

        for (unsigned int i = 0; i < num_meshes; ++i)
        {
          const aiMesh* const  mesh              = scene->mMeshes[i];
          Mesh* const          mesh_proto        = result.mesh_list->data + i;
          const unsigned int   num_mesh_vertices = mesh->mNumVertices;
          const AssetIndexType num_mesh_indices  = mesh->mNumFaces * k_NumIndicesPerFace;

          if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE))
          {
            continue;
          }

          assert((k_MaxUint32 - num_vertices) >= num_mesh_vertices && "Model has too many vertices.");
          assert((k_MaxUint32 - num_indices) >= num_mesh_indices && "Model has too many indices.");

          mesh_proto->index_offset = num_indices;
          mesh_proto->num_indices  = num_mesh_indices;
          mesh_proto->material_idx = mesh->mMaterialIndex;

          num_vertices += num_mesh_vertices;
          num_indices += num_mesh_indices;
        }

        result.vertices = detail::makeUniqueTempArray<AssetModelVertex>(memory, num_vertices);
        result.indices  = detail::makeUniqueTempArray<AssetIndexType>(memory, num_indices);
      }

      // Merging All Vertices Into One Buffer

      Vector3f bounds_min = {std::numeric_limits<float>::infinity()};
      Vector3f bounds_max = {-std::numeric_limits<float>::infinity()};
      {
        AssetIndexType  index_offset  = 0;
        AssetIndexType  vertex_offset = 0;
        AssetIndexType* output_index  = result.indices->data;

        for (unsigned int i = 0; i < num_meshes; ++i)
        {
          const aiMesh* const  mesh              = scene->mMeshes[i];
          const unsigned int   num_mesh_vertices = mesh->mNumVertices;
          const unsigned int   num_mesh_faces    = mesh->mNumFaces;
          const AssetIndexType num_mesh_indices  = num_mesh_faces * k_NumIndicesPerFace;

          if (!(mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) || num_mesh_vertices == 0)
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

            bounds_min = vec::min(bounds_min, output_vertex->position);
            bounds_max = vec::max(bounds_max, output_vertex->position);

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

            if (bone_index != unsigned(-1))
            {
              aiMat4x4ToMatrix4x4(&bone->mOffsetMatrix, result.skeleton.bones[bone_index].second, load_settings.row_major);

              for (unsigned int bw = 0; bw < bone->mNumWeights; ++bw)
              {
                const auto vertex_index = vertex_offset + bone->mWeights[bw].mVertexId;

                addBoneDataToVertex(
                 result.vertices->data + vertex_index,
                 bone->mWeights[bw].mWeight,
                 std::uint8_t(bone_index));
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

      // Scale the model to fit in Unit Cube.

      const AABB bounds          = {bounds_min, bounds_max};
      const auto global_inv      = scene->mRootNode->mTransformation;
      result.object_space_bounds = bounds;

      aiMat4x4ToMatrix4x4(&global_inv, result.skeleton.global_inv_transform, load_settings.row_major);
      Mat4x4_inverse(&result.skeleton.global_inv_transform, &result.skeleton.global_inv_transform);

      // Process Materials
      {
        const unsigned int num_materials = scene->mNumMaterials;

        result.materials = detail::makeUniqueTempArray<AssetPBRMaterial>(memory, num_materials);

        const auto load_texture = [](const aiMaterial* material, aiTextureType tex_type) -> AssetTempLargeString {
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
      memory.deallocate(const_cast<char*>(nul_terminated_path), (file_path.str_end - file_path.str_bgn) + 1);
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
