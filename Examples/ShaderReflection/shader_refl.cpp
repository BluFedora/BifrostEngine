#include "spirv_reflect.h"

#include <cstdio>
#include <cassert>

std::pair<unsigned char*, long> loadFileIntoMemory(const char* const filename)
{
  FILE*          f      = fopen(filename, "rb");  // NOLINT(android-cloexec-fopen)
  unsigned char* buffer = NULL;
  long           file_size;

  if (f)
  {
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);
    buffer = static_cast<unsigned char*>(std::malloc(file_size + std::size_t(1)));

    if (buffer)
    {
      fread(buffer, sizeof(buffer[0]), file_size, f);
      buffer[file_size] = '\0';
    }
    else
    {
      file_size = 0;
    }

    fclose(f);
  }
  else
  {
    file_size = 0;
  }

  return {buffer, file_size};
}

int SpirvReflectExample(const void* spirv_code, size_t spirv_nbytes)
{
  // Generate reflection data for a shader
  SpvReflectShaderModule module;
  SpvReflectResult       result = spvReflectCreateShaderModule(spirv_nbytes, spirv_code, &module);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  // Enumerate and extract shader's input variables
  uint32_t var_count = 0;
  result             = spvReflectEnumerateInputVariables(&module, &var_count, NULL);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);
  SpvReflectInterfaceVariable** input_vars =
   (SpvReflectInterfaceVariable**)malloc(var_count * sizeof(SpvReflectInterfaceVariable*));
  result = spvReflectEnumerateInputVariables(&module, &var_count, input_vars);
  assert(result == SPV_REFLECT_RESULT_SUCCESS);

  for (uint32_t i = 0u; i < var_count; ++i) 
  {
    SpvReflectInterfaceVariable* input_variable = input_vars[i];

    std::printf("InputVar[%i] = \"%s\"(%s)\n", int(i), input_variable->name, input_variable->semantic);
  }

  // Output variables, descriptor bindings, descriptor sets, and push constants
  // can be enumerated and extracted using a similar mechanism.

  // Destroy the reflection data when no longer required.
  spvReflectDestroyShaderModule(&module);

  return 0;
}

int main() 
{
  std::printf("Shader Reflection Demo\n");

  auto shader = loadFileIntoMemory("assets/shaders/standard/compiled/gbuffer.frag.spv");

  SpirvReflectExample(shader.first, shader.second);

  std::printf("\n");

  shader = loadFileIntoMemory("assets/shaders/standard/compiled/gbuffer.vert.spv");
  SpirvReflectExample(shader.first, shader.second);


  std::free(shader.first);

  return 0;
}