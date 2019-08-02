#include "bifrost/render/bifrost_shader_api.h"

#include <stdio.h>  /* fopen, fclose */
#include <stdlib.h> /* malloc, free  */
#include <assert.h> /* assert        */

// TODO(Shareef): Move this somewhere more public as this is a generic utility
static inline char* load_from_file(const char* const filename, long* out_size)
{
  FILE *f = fopen(filename, "rb");  // NOLINT(android-cloexec-fopen)
  char* buffer = NULL;

  if (f) {
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    buffer = malloc(fileSize);
    fread(buffer, fileSize, 1, f);
    fclose(f);

    (*out_size) = fileSize;
  }
  else
  {
    assert(0 && "failed to load a file");
  }

  return buffer;
}

void BifrostShaderProgram_loadFile(bfShaderProgram self, BifrostShaderTypes type, const char* filename)
{
  long shader_code_size;
  char* shader_code = load_from_file(filename, &shader_code_size);
  assert(shader_code_size && shader_code && "BifrostShaderProgram_loadFile failed to load the file correctly");
  BifrostShaderProgram_loadData(self, type, shader_code, (size_t)shader_code_size);
  free(shader_code);
}
