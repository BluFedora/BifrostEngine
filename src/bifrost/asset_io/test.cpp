#include "bifrost/asset_io/test.h"

#include "bifrost/data_structures/bifrost_array_t.h"
#include "bifrost/data_structures/bifrost_dynamic_string.h"

#include <cctype>   // isdigit
#include <cstdlib>  // atoi

static void skip_line(const char *data, uint *pointer, uint file_length)
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

static void skip_whitespace(const char *data, uint *pointer)
{
  while (data[*pointer] == ' ') ++(*pointer);
}

static void skip_until_whitespace(const char *data, uint *pointer)
{
  while (data[*pointer] != ' ') ++(*pointer);
}

static void skip_non_digit(const char *data, uint *pointer)
{
  while (!isdigit(data[*pointer]) && data[*pointer] != '-') ++(*pointer);
}

static void skip_digit(const char *data, uint *pointer)
{
  while (isdigit(data[*pointer]) || data[*pointer] == '-') ++(*pointer);
}

static void skip_comma(const char *data, uint *pointer)
{
  if (data[*pointer] == ',') ++(*pointer);
}

static void extract_face(const char *data, uint *pointer, int *v_index, int *uv_index, int *n_index)
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

TEST_ModelData TEST_AssetIO_loadObj(const char *obj_file_data, uint obj_file_data_length)
{
  typedef struct
  {
    int vertex[3];
    int uv[3];
    int normal[3];

  } Face;

  TEST_ModelData ret;

  ret.vertices        = OLD_bfArray_newT(BasicVertex, 10);
  Vec3f *positions    = OLD_bfArray_newT(Vec3f, 10);
  Vec2f *uvs          = OLD_bfArray_newT(Vec2f, 10);
  Vec3f *normals      = OLD_bfArray_newT(Vec3f, 10);
  Face * faces        = OLD_bfArray_newT(Face, 10);
  uint   file_pointer = 0;

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
            Vec3f *position = (Vec3f *)Array_emplace(&positions);

            skip_until_whitespace(obj_file_data, &file_pointer);
            skip_whitespace(obj_file_data, &file_pointer);
            position->x = (float)atof(obj_file_data + file_pointer);
            skip_until_whitespace(obj_file_data, &file_pointer);
            skip_whitespace(obj_file_data, &file_pointer);
            position->y = (float)atof(obj_file_data + file_pointer);
            skip_until_whitespace(obj_file_data, &file_pointer);
            skip_whitespace(obj_file_data, &file_pointer);
            position->z = (float)atof(obj_file_data + file_pointer);
            skip_line(obj_file_data, &file_pointer, obj_file_data_length);
            position->w = 1.0f;
            break;
          }
          case 't':
          {
            Vec2f *uvsCoords = (Vec2f *)Array_emplace(&uvs);

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
            Vec3f *normal = (Vec3f *)Array_emplace(&normals);

            skip_until_whitespace(obj_file_data, &file_pointer);
            skip_whitespace(obj_file_data, &file_pointer);
            normal->x = (float)atof(obj_file_data + file_pointer);
            skip_until_whitespace(obj_file_data, &file_pointer);
            skip_whitespace(obj_file_data, &file_pointer);
            normal->y = (float)atof(obj_file_data + file_pointer);
            skip_until_whitespace(obj_file_data, &file_pointer);
            skip_whitespace(obj_file_data, &file_pointer);
            normal->z = (float)atof(obj_file_data + file_pointer);
            skip_line(obj_file_data, &file_pointer, obj_file_data_length);
            normal->w = 0.0f;
            break;
          }
        }
        break;
      }
      case 'f':
      {
        int v1, v2, v3, uv1, uv2, uv3, n1, n2, n3;

        skip_non_digit(obj_file_data, &file_pointer);
        extract_face(obj_file_data, &file_pointer, &v1, &uv1, &n1);
        skip_until_whitespace(obj_file_data, &file_pointer);
        skip_whitespace(obj_file_data, &file_pointer);
        extract_face(obj_file_data, &file_pointer, &v2, &uv2, &n2);
        skip_until_whitespace(obj_file_data, &file_pointer);
        skip_whitespace(obj_file_data, &file_pointer);
        extract_face(obj_file_data, &file_pointer, &v3, &uv3, &n3);
        skip_line(obj_file_data, &file_pointer, obj_file_data_length);

        Face face =
         {
          {v1, v2, v3},
          {uv1, uv2, uv3},
          {n1, n2, n3},
         };

        Array_push(&faces, &face);
        break;
      }
      default:
      {
        skip_line(obj_file_data, &file_pointer, obj_file_data_length);
        break;
      }
    }
  }

  for (uint i = 0; i < Array_size(&faces); ++i)
  {
    const Face *face = faces + i;

    for (uint j = 0; j < 3; ++j)
    {
      BasicVertex *vertex = (BasicVertex *)Array_emplace(&ret.vertices);

      const Vec3f *const pos    = face->vertex[j] == -1 ? NULL : positions + face->vertex[j] - 1;
      const Vec3f *const normal = face->normal[j] == -1 ? NULL : normals + face->normal[j] - 1;
      const Vec2f *const uv     = face->uv[j] == -1 ? NULL : uvs + face->uv[j] - 1;

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

      *(uint32_t *)vertex->color = /*COLOR_AQUA * */ Vec3f_toColor(pos);
    }
  }

  Array_delete(&positions);
  Array_delete(&uvs);
  Array_delete(&normals);
  Array_delete(&faces);

  return ret;
}

void TEST_ModelData::destroy()
{
  Array_delete(&vertices);
  vertices = nullptr;
}
