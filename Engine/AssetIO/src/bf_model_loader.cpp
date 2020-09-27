#include "bf/asset_io/bf_model_loader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cassert>  // assert

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

  static void aiVector3ToArray(const aiVector3D* from, float to[3])
  {
    to[0] = from->x;
    to[1] = from->y;
    to[2] = from->z;
  }

  static void aiColor4DToArray(const aiColor4D* from, float to[4])
  {
    // aiNodeAnim;
    to[0] = from->r;
    to[1] = from->g;
    to[2] = from->b;
    to[3] = from->a;
  }

  static void aiMat4x4ToMatrix4x4(const aiMatrix4x4* from, float* to)
  {
    static_assert(sizeof(*from) == sizeof(float) * 16, "Inccorect size of matrix from Assimp");

    memcpy(to, from, sizeof(*from));
  }

  static unsigned findAssetNode(ModelSkeleton& skeleton, const AssetNode** nodes, unsigned& num_nodes, const aiString& name)
  {
    for (unsigned i = 0; i < num_nodes; ++i)
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
        const unsigned id = num_nodes++;

        assert(num_nodes < k_MaxBones && "Too Many Bones");

        skeleton.nodes[i].model_to_bone_idx = id;
        nodes[id]                           = &skeleton.nodes[i];

        return id;
      }
    }

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
  static void recurseNodes(aiNode* parent_node, aiNode* node, F&& callback, int depth)
  {
#if 0 /* Depth */
    callback(parent_node, node, depth);

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
      recurseNodes(node, node->mChildren[i], std::forward<F>(callback), depth + 1);
      // callback(node, node->mChildren[i]);
    }
#else /* Level */
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
      callback(parent_node, node->mChildren[i], depth);
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
      recurseNodes(node, node->mChildren[i], std::forward<F>(callback), depth + 1);
    }
#endif
  }

  template<typename F>
  static void recurseNodes(aiNode* node, F&& callback)
  {
#if 0 /* Depth */
    recurseNodes(nullptr, node, std::forward<F>(callback), 0);
#else /* Level */
    callback(nullptr, node, 0);
    recurseNodes(nullptr, node, std::forward<F>(callback), 1);
#endif
  }

  static void PrintAssetNode(AssetNode* base_array, AssetNode& node, int depth)
  {
    for (unsigned int i = 0; i < node.num_children; ++i)
    {
      auto& child = base_array[i + node.first_child];

      for (int i = 0; i < depth * 2; ++i)
      {
        std::printf(" ");
      }

      std::printf("(depth:%i, %i) '%s'\n",
                  depth,
                  child.num_children,
                  child.name.data);
    }

    for (unsigned int i = 0; i < node.num_children; ++i)
    {
      PrintAssetNode(base_array, base_array[i + node.first_child], depth + 1);
    }
  }

  static void PrintAssetNode(AssetNode* base_array, AssetNode& node)
  {
    std::printf("(depth:%i, %i) '%s'\n",
                0,
                node.num_children,
                node.name.data);

    PrintAssetNode(base_array, node, 1);
  }

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

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
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
      unsigned         num_bone_to_node         = 0;

      // Process Nodes
      {
        unsigned int num_nodes = 0;

        recurseNodes(
         scene->mRootNode,
         [&num_nodes](const aiNode* parent_node, const aiNode* node, int depth) {
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

           ++num_nodes;
         });

        for (int i = 0; i < 80; ++i)
          std::printf("-");
        std::printf("\n");

        aiMat4x4ToMatrix4x4(&scene->mRootNode->mTransformation, result.skeleton.global_inv_transform);
        result.skeleton.num_nodes = num_nodes;
        result.skeleton.nodes     = memory.allocateArray<AssetNode>(num_nodes);

        recurseNodes(
         scene->mRootNode,
         [&skeleton = result.skeleton, node_idx = 0](const aiNode* parent_node, const aiNode* node, int depth) mutable {
           AssetNode& model_node = skeleton.nodes[node_idx++];

           aiMat4x4ToMatrix4x4(&node->mTransformation, model_node.transform);
           model_node.name.copyOverString(node->mName.data, node->mName.length);
           model_node.first_child       = node_idx;
           model_node.num_children      = node->mNumChildren;
           model_node.model_to_bone_idx = -1;
         });

        PrintAssetNode(result.skeleton.nodes, result.skeleton.nodes[0]);
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
        AssetIndexType index_offset  = 0;
        AssetIndexType vertex_offset = 0;

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

            output_vertex->uv[0] = uv->x;
            output_vertex->uv[1] = uv->y;
          }

          // Faces

          for (unsigned int f = 0; f < num_mesh_faces; ++f)
          {
            const aiFace* const face = mesh->mFaces + f;

            assert(face->mNumIndices == 3 && "Only triangles are supported.");

            for (unsigned int face_idx = 0; face_idx < face->mNumIndices; ++face_idx)
            {
              AssetIndexType* const output_index = result.indices->data + index_offset + face_idx;

              *output_index = face->mIndices[face_idx] + vertex_offset;
            }
          }

          // Bones

          for (unsigned int b = 0; b < mesh->mNumBones; ++b)
          {
            const aiBone* const bone       = mesh->mBones[b];
            const unsigned      bone_index = findAssetNode(result.skeleton, bone_to_node, num_bone_to_node, bone->mName);

            if (bone_index != -1)
            {
              aiMat4x4ToMatrix4x4(&bone->mOffsetMatrix, result.skeleton.bones[bone_index]);

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
              std::memcpy(str.data, tex_path.data, tex_path.length + 1);

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
