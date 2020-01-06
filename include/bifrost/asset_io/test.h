#ifndef ASSETIO_TEST
#define ASSETIO_TEST

#include "bifrost/bifrost_math.h"

struct BasicVertex final
{
  Vec3f pos;
  Vec3f normal;
  uchar color[4];
  Vec2f uv;
};

struct TEST_ModelData final
{
  BasicVertex* vertices = nullptr;
  void destroy();
};

TEST_ModelData TEST_AssetIO_loadObj(const char* obj_file_data, uint obj_file_data_length);

#endif
