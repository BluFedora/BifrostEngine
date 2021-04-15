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

  for (uint32_t i = 0u; i < module.entry_point_count; ++i)
  {
    SpvReflectEntryPoint& entry_point = module.entry_points[i];

    std::printf("EntryPoint(%i) = \"%s\"\n", int(i), entry_point.name);
  }

  // Output variables, descriptor bindings, descriptor sets, and push constants
  // can be enumerated and extracted using a similar mechanism.

  // Destroy the reflection data when no longer required.
  spvReflectDestroyShaderModule(&module);

  return 0;
}

#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_cpp.hpp"

#include <iostream>

int main() 
{
  std::printf("Shader Reflection Demo\n");

  auto shader = loadFileIntoMemory("assets/imgui.vert.spv");

  SpirvReflectExample(shader.first, shader.second);
  
  spirv_cross::CompilerGLSL glsl((const uint32_t*)shader.first, shader.second / sizeof(uint32_t));
  spirv_cross::CompilerHLSL hlsl((const uint32_t*)shader.first, shader.second / sizeof(uint32_t));
  spirv_cross::CompilerCPP  cpp((const uint32_t*)shader.first, shader.second / sizeof(uint32_t));
  spirv_cross::CompilerMSL  msl((const uint32_t*)shader.first, shader.second / sizeof(uint32_t));

  // The SPIR-V is now parsed, and we can perform reflection on it.
  spirv_cross::ShaderResources resources = glsl.get_shader_resources();

  // Get all sampled images in the shader.
  for (auto& resource : resources.sampled_images)
  {
    unsigned set     = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
    unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
    //printf("Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);

    // Modify the decoration to prepare it for GLSL.
    glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

    // Some arbitrary remapping if we want.
    glsl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
  }

  // Set some options.
  spirv_cross::CompilerGLSL::Options options;
  options.version = 310;
  options.es      = true;
  glsl.set_common_options(options);

  
  std::cout << "GLSL(ES):\n\n" << glsl.compile() << "\n";

  options.version = 330;
  options.es      = false;
  glsl.set_common_options(options);

  std::cout << "GLSL(GL):\n\n" << glsl.compile() << "\n";

  options.version          = 450;
  options.vulkan_semantics = true;
  glsl.set_common_options(options);

  std::cout << "GLSL(VK):\n\n" << glsl.compile() << "\n";
  std::cout << "    HLSL:\n\n" << hlsl.compile() << "\n";
  std::cout << "     CPP:\n\n" << cpp.compile() << "\n";
  std::cout << "     MSL:\n\n" << msl.compile() << "\n";

  std::free(shader.first);

  std::printf("\n");

  shader = loadFileIntoMemory("assets/shaders/standard/compiled/gbuffer.vert.spv");
  
  SpirvReflectExample(shader.first, shader.second);
  std::free(shader.first);



  return 0;
}