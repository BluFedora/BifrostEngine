add_executable(
  ShaderReflection

  "shader_refl.cpp"
)

target_link_libraries(
  ShaderReflection
  PUBLIC
    spirv-cross-glsl
    spirv-cross-hlsl
    spirv-cross-cpp
    spirv-cross-reflect
    spirv-cross-msl
    spirv-cross-util
    spirv-cross-core

    spirv-reflect
)