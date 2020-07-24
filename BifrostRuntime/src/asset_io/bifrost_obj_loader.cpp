#include "bifrost/data_structures/bifrost_array.hpp"
#include "bifrost/math/bifrost_vec3.h"

#include "bifrost/graphics/bifrost_standard_renderer.hpp"
#include <cctype>  // isdigit
#include <cstdint>
#include <cstdlib>  // atoi

namespace bifrost
{
  static void skip_line(const char *data, std::size_t *pointer, std::size_t file_length)
  {
    while ((*pointer) < file_length && data[*pointer] != '\n')
    {
      ++(*pointer);
    }

    if ((*pointer) < file_length)
    {
      ++(*pointer);
    }
  }

  static void skip_whitespace(const char *data, std::size_t *pointer)
  {
    while (data[*pointer] == ' ')
    {
      ++(*pointer);
    }
  }

  static void skip_until_whitespace(const char *data, std::size_t *pointer)
  {
    while (data[*pointer] != ' ')
    {
      ++(*pointer);
    }
  }

  static void skip_non_digit(const char *data, std::size_t *pointer)
  {
    while (!isdigit(data[*pointer]) && data[*pointer] != '-')
    {
      ++(*pointer);
    }
  }

  static void skip_digit(const char *data, std::size_t *pointer)
  {
    while (isdigit(data[*pointer]) || data[*pointer] == '-')
    {
      ++(*pointer);
    }
  }

  static void extract_face(const char *data, std::size_t *pointer, int *v_index, int *uv_index, int *n_index)
  {
    (*v_index) = atoi(data + (*pointer));
    skip_digit(data, pointer);

    if (data[(*pointer)] == '/')
    {
      ++(*pointer);

      if (data[(*pointer)] == '/')
      {
        (*uv_index) = -1;
      }
      else if (isdigit(data[(*pointer)]))
      {
        (*uv_index) = atoi(data + (*pointer));
        skip_digit(data, pointer);
      }

      if (data[(*pointer)] == '/')
      {
        ++(*pointer);

        if (isdigit(data[(*pointer)]))
        {
          (*n_index) = atoi(data + (*pointer));
          skip_digit(data, pointer);
        }
        else
        {
          (*n_index) = -1;
        }
      }
    }
  }

  static void load_vec3f(Array<Vector3f> &arr, const char *obj_file_data, std::size_t *file_pointer, std::size_t obj_file_data_length, float default_w)
  {
    Vector3f &position = arr.emplace();

    skip_until_whitespace(obj_file_data, file_pointer);
    skip_whitespace(obj_file_data, file_pointer);
    position.x = (float)atof(obj_file_data + *file_pointer);
    skip_until_whitespace(obj_file_data, file_pointer);
    skip_whitespace(obj_file_data, file_pointer);
    position.y = (float)atof(obj_file_data + *file_pointer);
    skip_until_whitespace(obj_file_data, file_pointer);
    skip_whitespace(obj_file_data, file_pointer);
    position.z = (float)atof(obj_file_data + *file_pointer);
    skip_line(obj_file_data, file_pointer, obj_file_data_length);
    position.w = default_w;
  }

  static void load_position(Array<Vector3f> &arr, const char *obj_file_data, std::size_t *file_pointer, std::size_t obj_file_data_length, Vector3f &pos_min, Vector3f &pos_max)
  {
    load_vec3f(arr, obj_file_data, file_pointer, obj_file_data_length, 1.0f);

    Vector3f &position = arr.back();

    pos_min = vec::min(pos_min, position);
    pos_max = vec::max(pos_max, position);
  }

  void loadObj(IMemoryManager &temp_allocator, Array<StandardVertex> &out, const char *obj_file_data, std::size_t obj_file_data_length)
  {
    struct FaceElement final
    {
      int position;
      int uv;
      int normal;
    };

    struct Face final
    {
      int position[3];
      int uv[3];
      int normal[3];
    };

    Array<Vector3f>    positions{temp_allocator};
    Array<Vector3f>    normals{temp_allocator};
    Array<Vector2f>    uvs{temp_allocator};
    Array<Face>        faces{temp_allocator};
    Array<FaceElement> face_elements{temp_allocator};
    std::size_t        file_pointer{0};
    Vector3f           min_bounds = {FLT_MAX};
    Vector3f           max_bounds = {FLT_MIN};

    while (file_pointer < obj_file_data_length)
    {
      const char *line = (obj_file_data + file_pointer);

      switch (line[0])
      {
        case '#':
        {
          skip_line(obj_file_data, &file_pointer, obj_file_data_length);
          break;
        }
        case 'v':
        {
          switch (line[1])
          {
            case ' ':
            {
              load_position(positions, obj_file_data, &file_pointer, obj_file_data_length, min_bounds, max_bounds);
              break;
            }
            case 't':
            {
              Vec2f *uvsCoords = (Vec2f *)&uvs.emplace();

              skip_until_whitespace(obj_file_data, &file_pointer);
              skip_whitespace(obj_file_data, &file_pointer);
              uvsCoords->x = (float)atof(obj_file_data + file_pointer);
              skip_until_whitespace(obj_file_data, &file_pointer);
              uvsCoords->y = (float)atof(obj_file_data + file_pointer);
              skip_line(obj_file_data, &file_pointer, obj_file_data_length);
              break;
            }
            case 'n':
            {
              load_vec3f(normals, obj_file_data, &file_pointer, obj_file_data_length, 0.0f);
              break;
            }
            default:
              ++file_pointer;
              break;
          }
          break;
        }
        case 'f':
        {
          skip_non_digit(obj_file_data, &file_pointer);

          int num_face_elements = 0;

          face_elements.clear();

          while (file_pointer < obj_file_data_length)
          {
            FaceElement face_element{-1, -1, -1};

            extract_face(obj_file_data, &file_pointer, &face_element.position, &face_element.uv, &face_element.normal);

            face_elements.emplace(face_element);

            ++num_face_elements;

            if (obj_file_data[file_pointer] != ' ')
            {
              break;
            }

            skip_until_whitespace(obj_file_data, &file_pointer);
            skip_whitespace(obj_file_data, &file_pointer);
          }

          skip_line(obj_file_data, &file_pointer, obj_file_data_length);

          for (int i = 1; i < num_face_elements - 1; ++i)
          {
            FaceElement *const fe = face_elements.data() + i;

            Face face =
             {
              {face_elements[0].position, fe[0].position, fe[1].position},
              {face_elements[0].uv, fe[0].uv, fe[1].uv},
              {face_elements[0].normal, fe[0].normal, fe[1].normal},
             };

            faces.emplace(face);
          }

          break;
        }
        default:
        {
          skip_line(obj_file_data, &file_pointer, obj_file_data_length);
          break;
        }
      }
    }

    out.reserve(faces.size() * 3);

    const Vector3f scale         = max_bounds - min_bounds;
    const Vector3f center        = (max_bounds + min_bounds) * 0.5f;
    const float    max_scale     = std::max({scale.x, scale.y, scale.z});
    const float    inv_max_scale = 1.0f / std::max(max_scale, k_Epsilon);

    for (std::size_t i = 0; i < faces.size(); ++i)
    {
      const Face *face = faces.data() + i;

      const Vector3f *const p0 = face->position[0] == -1 ? nullptr : positions.data() + face->position[0] - 1;
      const Vector3f *const p1 = face->position[1] == -1 ? nullptr : positions.data() + face->position[1] - 1;
      const Vector3f *const p2 = face->position[2] == -1 ? nullptr : positions.data() + face->position[2] - 1;

      if (!p0 || !p1 || !p2)
      {
        continue;
      }

      const Vector3f u           = (*p1) - (*p0);
      const Vector3f v           = (*p2) - (*p0);
      const Vector3f face_normal = vec::faceNormal(*p0, *p1, *p2);

      // [http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/]

      const Vector2f *const uv0 = face->uv[0] == -1 ? nullptr : uvs.data() + face->uv[0] - 1;
      const Vector2f *const uv1 = face->uv[1] == -1 ? nullptr : uvs.data() + face->uv[1] - 1;
      const Vector2f *const uv2 = face->uv[2] == -1 ? nullptr : uvs.data() + face->uv[2] - 1;

      Vector3f face_tangent = {1.0f, 0.0f, 0.0, 0.0f};

      if (uv0 && uv1 && uv2)
      {
        const Vector2f delta_uv0 = (*uv1) - (*uv0);
        const Vector2f delta_uv1 = (*uv2) - (*uv0);
        const float    r         = 1.0f / std::max(delta_uv0.x * delta_uv1.y - delta_uv0.y * delta_uv1.x, 0.001f);

        face_tangent = (u * delta_uv1.y - v * delta_uv0.y) * r;
        // face_bitangent = (v * delta_uv1.x - u * delta_uv0.x) * r;

        /*
        if (glm::dot(glm::cross(n, t), b) < 0.0f){
          t = t * -1.0f;
        }
        */
      }

      for (std::size_t j = 0; j < 3; ++j)
      {
        StandardVertex *const vertex = &out.emplace();

        const Vector3f *const pos    = positions.data() + face->position[j] - 1;
        const Vector3f *const normal = face->normal[j] == -1 ? nullptr : normals.data() + face->normal[j] - 1;
        const Vector2f *const uv     = face->uv[j] == -1 ? nullptr : uvs.data() + face->uv[j] - 1;

        if (pos)
        {
          vertex->pos = (*pos - center) * inv_max_scale;
        }

        vertex->normal  = normal ? *normal : face_normal;
        vertex->tangent = face_tangent;

        if (uv)
        {
          vertex->uv = *uv;
        }

        vertex->color = bfColor4u_fromUint32(0xFFFFFFFF);
      }
    }
  }
}  // namespace bifrost
