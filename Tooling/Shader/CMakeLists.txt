
add_library(
  BFTooling_Shader STATIC

  "bf_shader_pipeline.hpp"
  "bf_shader_pipeline.cpp"
)

target_link_libraries(
  BFTooling_Shader

  PUBLIC
    BF_AssetIO
    BF_DataStructuresCxx
    BF_Graphics

    MachineIndependent
    OGLCompiler
    glslang
    SPIRV
)
