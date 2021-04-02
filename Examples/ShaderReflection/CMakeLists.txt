add_executable(
  ShaderReflection

  "shader_refl.cpp"
)

target_link_libraries(
  ShaderReflection
  PUBLIC
    spirv-reflect
)