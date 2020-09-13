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
    to[0] = from->r;
    to[1] = from->g;
    to[2] = from->b;
    to[3] = from->a;
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
                                aiProcess_GenUVCoords |
                                aiProcess_CalcTangentSpace;

    import_flags |= load_settings.smooth_normals ? aiProcess_GenSmoothNormals : aiProcess_GenNormals;

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
          const aiMesh* const  mesh              = scene->mMeshes[i];
          const unsigned int   num_mesh_vertices = mesh->mNumVertices;
          const unsigned int   num_mesh_faces    = mesh->mNumFaces;
          const AssetIndexType num_mesh_indices  = num_mesh_faces * k_NumIndicesPerFace;

          for (unsigned int v = 0; v < num_mesh_vertices; ++v)
          {
            AssetModelVertex* const output_vertex = result.vertices->data + vertex_offset + v;

            assert(mesh->mVertices && "Mesh Is Missing Vertex Data");
            assert(mesh->mNormals && "Mesh Is Missing Vertex Data");
            assert(mesh->mTangents && "Mesh Is Missing Vertex Data");
            assert(mesh->mBitangents && "Mesh Is Missing Vertex Data");
            assert(mesh->mTextureCoords[0] && "Mesh Is Missing Vertex Data");

            const aiVector3D* const position  = mesh->mVertices + v;
            const aiVector3D* const normal    = mesh->mNormals + v;
            const aiVector3D* const tangent   = mesh->mTangents + v;
            const aiVector3D* const bitangent = mesh->mBitangents + v;
            const aiVector3D* const uv        = mesh->mTextureCoords[0] + v;

            aiVector3ToArray(position, output_vertex->position);
            aiVector3ToArray(normal, output_vertex->normal);
            aiVector3ToArray(tangent, output_vertex->tangent);
            aiVector3ToArray(bitangent, output_vertex->bitangent);

            if (mesh->HasVertexColors(0))
            {
              aiColor4DToArray(mesh->mColors[0] + v, output_vertex->color);
            }
            else
            {
              output_vertex->color[0] = 1.0f;
              output_vertex->color[1] = 1.0f;
              output_vertex->color[2] = 1.0f;
              output_vertex->color[3] = 1.0f;
            }

            output_vertex->uv[0] = uv->x;
            output_vertex->uv[1] = uv->y;
          }

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

          vertex_offset += num_mesh_vertices;
          index_offset += num_mesh_indices;
        }
      }

      // Process Materials
      {
        const unsigned int num_materials = scene->mNumMaterials;

        result.materials = detail::makeUniqueTempArray<AssetPBRMaterial>(memory, num_materials);

        const auto load_texture = [&memory](const aiMaterial* material, aiTextureType tex_type) -> AssetTempString {
          if (material->GetTextureCount(tex_type) > 0)
          {
            aiString       tex_path;
            const aiReturn res = material->GetTexture(tex_type, 0, &tex_path, nullptr, nullptr, nullptr, nullptr, nullptr);

            if (res == aiReturn_SUCCESS)
            {
              AssetTempString str;

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

      // Process Nodes
      {

      }
    }

    // Cleanup
  done:
    if (path_needs_clone)
    {
      memory.deallocate(const_cast<char*>(nul_terminated_path));
    }

    return result;
  }

  void AssetModelLoadResult::setError(const char* err_message) noexcept
  {
    const std::size_t buffer_len_minus_one = sizeof(error_buffer) - 1;

    error_buffer[buffer_len_minus_one] = '\0';

    error = bfMakeStringRangeC(std::strncpy(error_buffer.data(), err_message, buffer_len_minus_one));
  }
}  // namespace bf
