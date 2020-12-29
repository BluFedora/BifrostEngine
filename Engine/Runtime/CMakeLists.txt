################################################################################
###  CMakeList.txt : The 'game shipping' high-level API.                     ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostRuntime VERSION 1.0.0 DESCRIPTION "")

add_library(BifrostRuntime INTERFACE)

target_include_directories(
  BifrostRuntime
  INTERFACE 
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/../../Runtime/include # TODO(SR): This is a hack to bootstrap the new project layout.
    ${PROJECT_SOURCE_DIR}/../../Engine/Graphics2D/include
    ${PROJECT_SOURCE_DIR}/../../Experiments/include
)

target_link_libraries(
  BifrostRuntime
  INTERFACE
    BF_AssetIO_static
    BF_Core_static
    # BF_Platform_shared
    BF_Text_static
    BF_Memory_interface

    BF_DataStructuresCxx
    BifrostScript_shared
    BF_Math_shared
    bfAnimation2D_shared
    # BifrostGraphics_shared
)

if (WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
     set(BIFROST_RUNTIME_LIB ${PROJECT_SOURCE_DIR}/lib/win64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
     set(BIFROST_RUNTIME_LIB ${PROJECT_SOURCE_DIR}/lib/win32)
  endif()
elseif(APPLE)
  set(BIFROST_RUNTIME_LIB ${PROJECT_SOURCE_DIR}/lib/macOS)
endif()

if (WIN32)
  # target_link_libraries(BifrostEngine PRIVATE "opengl32.lib")

  set(BIFROST_RUNTIME_LIB_PREFIX "")
  
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BIFROST_RUNTIME_LIB_SUFFIX "d")
  else()
    set(BIFROST_RUNTIME_LIB_SUFFIX "")
  endif()

  set(BIFROST_RUNTIME_LIB_EXT "lib")

elseif (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit")

  set(BIFROST_RUNTIME_LIB_PREFIX "lib")
  set(BIFROST_RUNTIME_LIB_SUFFIX "")
  set(BIFROST_RUNTIME_LIB_EXT    "a")
endif()

# Libraries for glslang

target_link_libraries(
  BifrostRuntime 
  INTERFACE 
    "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}glslang${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"
    "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}HLSL${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"
    "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}OGLCompiler${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"
    "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}OSDependent${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"
    "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}SPIRV${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"

    # These libraries are not needed.

    # "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}SPIRV-Tools${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"
    # "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}SPIRV-Tools-opt${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"
    # "${BIFROST_RUNTIME_LIB}/${BIFROST_RUNTIME_LIB_PREFIX}SPIRV${BIFROST_RUNTIME_LIB_SUFFIX}.${BIFROST_RUNTIME_LIB_EXT}"
)

set(THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED "${PROJECT_SOURCE_DIR}/../../Runtime/src/")

target_sources(
   BifrostRuntime
   INTERFACE

   # Asset IO

   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}asset_io/bifrost_asset_handle.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}asset_io/bifrost_assets.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}asset_io/bifrost_file.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}asset_io/bifrost_json_serializer.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}asset_io/bifrost_scene.cpp"

   # Core

   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}core/bifrost_engine.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}core/bifrost_game_state_machine.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}core/bifrost_igame_state_layer.cpp"

   # ECS

   "${PROJECT_SOURCE_DIR}/src/ecs/bf_base_component.cpp"

   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}ecs/bifrost_behavior.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}ecs/bifrost_behavior_system.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}ecs/bifrost_collision_system.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}ecs/bifrost_entity.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}ecs/bifrost_entity_ref.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}ecs/bifrost_iecs_system.cpp"

   # Graphics

   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}graphics/bifrost_component_renderer.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}graphics/bifrost_debug_renderer.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}graphics/bifrost_glsl_compiler.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}graphics/bifrost_standard_renderer.cpp"

   # Meta

   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}meta/bifrost_meta_runtime.cpp"

   # Utility

   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}utility/bifrost_json.cpp"
   "${THIS_IS_A_HACK_THAT_NEEDS_TO_BE_FIXED}utility/bifrost_uuid.c"

   

   # HAcky Hacks

   "${PROJECT_SOURCE_DIR}/../../Experiments/src/bifrost_json.c"

   "${PROJECT_SOURCE_DIR}/../../Engine/Graphics2D/src/bf_painter.cpp"

   # Mostly Correct

   "${PROJECT_SOURCE_DIR}/src/anim2D/bf_animation_system.cpp"
   "${PROJECT_SOURCE_DIR}/src/asset_io/bf_path_manip.cpp"
   "${PROJECT_SOURCE_DIR}/src/asset_io/bf_spritesheet_asset.cpp"
)
