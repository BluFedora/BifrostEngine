
#include "bifrost/asset_io/test.h"
#include "bifrost/data_structures/bifrost_array.hpp"
#include "bifrost/math/bifrost_vec3.h"

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

  static void load_vec3f(Array<Vec3f> &arr, const char *obj_file_data, std::size_t *file_pointer, std::size_t obj_file_data_length, float default_w)
  {
    Vec3f &position = arr.emplace();

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

  void loadObj(IMemoryManager &temp_allocator, Array<BasicVertex> &out, const char *obj_file_data, std::size_t obj_file_data_length)
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

    Array<Vec3f>       positions{temp_allocator};
    Array<Vec2f>       uvs{temp_allocator};
    Array<Vec3f>       normals{temp_allocator};
    Array<Face>        faces{temp_allocator};
    Array<FaceElement> face_elements{temp_allocator};
    std::size_t        file_pointer{0};

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
              load_vec3f(positions, obj_file_data, &file_pointer, obj_file_data_length, 1.0f);
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

    for (std::size_t i = 0; i < faces.size(); ++i)
    {
      const Face *face = faces.data() + i;

      for (std::size_t j = 0; j < 3; ++j)
      {
        BasicVertex *vertex = &out.emplace();

        const Vec3f *const pos    = face->position[j] == -1 ? NULL : positions.data() + face->position[j] - 1;
        const Vec3f *const normal = face->normal[j] == -1 ? NULL : normals.data() + face->normal[j] - 1;
        const Vec2f *const uv     = face->uv[j] == -1 ? NULL : uvs.data() + face->uv[j] - 1;

        if (pos)
        {
          Vec3f_copy(&vertex->pos, pos);
        }

        if (normal)
        {
          Vec3f_copy(&vertex->normal, normal);
        }

        if (uv)
        {
          vertex->uv = (*uv);
        }

        *reinterpret_cast<std::uint32_t *>(vertex->color) = /*COLOR_AQUA * */ Vec3f_toColor(pos);
      }
    }
  }
}  // namespace bifrost
