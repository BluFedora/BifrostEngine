################################################################################
#                                                                              #
#                               BF CMAKE PROJECT                               #
#                                                                              #
################################################################################
###  CMakeList.txt : Top-level CMake project file, for global configuration  ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BluFedoraEngine VERSION 1.0.0)

set(BF_ENGINE_VERSION_MAJOR 1)
set(BF_ENGINE_VERSION_MINOR 3)
set(BF_ENGINE_VERSION_PATCH 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # This will generate a header file with the version info.
  # The file will be made in the build directory hence why the build
  # dir needs to be added to the includes.
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/bf_version.h.in"
  "${PROJECT_BINARY_DIR}/bf/bf_version.h"
)
# NOTE(Shareef): Print out some CMake Info.
message(STATUS "Generated headers located at ${PROJECT_BINARY_DIR}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
    message(STATUS "We are building a DEBUG 64bit build...")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
    message(STATUS "We are building a DEBUG 32bit build...")
  endif()
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
    message(STATUS "We are building a RELEASE 64bit build...")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
   message(STATUS "We are building a RELEASE 32bit build...")
  endif()
endif()

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_GENERATOR_PLATFORM x64)

  # NOTE(Shareef): Sets the output directory to be in a reasonable place.
set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/bin/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

  # GCC / Clang
  #   "/GR-"                      - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "/D_***_SECURE_NO_WARNINGS" - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "-std=c++17"                = "/std:c++17"
  #   "-fno-rtti"                 = "/GR-"
  #   TODO(Shareef): /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj
 if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "-Wno-error=deprecated-declarations -std=c++17 -lstdc++fs -fno-rtti -Wall -g")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # NOTE(Shareef): Setting the compiler flags we will be using.
  #
  #   "/Zc:__cplusplus" - Report the correct '__cplusplus' macro on msvc.
  #   "/WX"   - treat warnings as errors.
  #   "/EHsc" - is for enabling C++ exceptions which we will NOT be using.
  #     Reason being the C++ STL uses exceptions like a mother f*cker and with warnings as
  #     errors on this would not build other wise.
  #   (alternative is to define: _HAS_EXCEPTIONS but that has some other implications.)
  #   "/GR-"  - Disables RTTI, trying to see if we can do without it for this engine.
  #   "/D_***_SECURE_NO_WARNINGS" - Disable the warnings for not using the "secure"
  #      versions of certain functions.
  #   "/std:c++17" - I needed some new C++17 features for some tEmPlAtE mAgIc
  #   "/GF"        - I want stable addresses. ("Eliminate Duplicate Strings")
  #   "/MP[N]"     - N is optional, will just use all the cores if omitted. Allows for
  #                  compilation on mutiple cores. Should give us a big speed-up on compile times.
  #
  # [https://docs.microsoft.com/en-us/cpp/build/reference/vmb-vmg-representation-method?redirectedfrom=MSDN&view=vs-2019]
  #
  set(CMAKE_CXX_FLAGS "/wd4996 /Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812 /vmg")
  set(CMAKE_C_FLAGS "/wd4996 /Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812 /vmg")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
  set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")
endif()

  # NOTE(Shareef): Automatically generate a list of ALL source (C/++) files based of off what is in the folders.
file(GLOB_RECURSE BIFROST_ENGINE_SOURCE_FILES
  "Engine/Runtime/lib/include/nativefiledialog/nfd_common.c"
  "Engine/Runtime/src/asset_io/*.cpp"
  "Engine/Runtime/src/core/bifrost_game_state_machine.cpp"
  "Engine/Runtime/src/core/bifrost_igame_state_layer.cpp"
  "Engine/Runtime/src/debug/bifrost_dbg_logger.c"
  "Engine/Runtime/src/ecs/bifrost_entity.cpp"
  "Engine/Runtime/src/ecs/bf_base_component.cpp"
  "Engine/Runtime/src/ecs/bifrost_iecs_system.cpp"
  "Engine/Runtime/src/bifrost_imgui_glfw.cpp"
  "Engine/Runtime/src/graphics/bifrost_standard_renderer.cpp"
  "Engine/Runtime/src/graphics/bifrost_glsl_compiler.cpp"
  "Engine/Runtime/gfx/src/bf_draw_2d.cpp"
  "Engine/Math/src/bifrost_rect2.cpp"
  "Engine/Runtime/src/meta/bifrost_meta_runtime.cpp"
  "Engine/Runtime/include/utility/bifrost_uuid.h"
  "Engine/Runtime/src/utility/bifrost_hash.cpp"
  "Engine/Runtime/src/utility/bifrost_json.cpp"
  "Engine/Runtime/src/utility/bifrost_json.c"
  "Engine/Runtime/src/utility/bifrost_uuid.c"
 )

if (WIN32)
  set(BIFROST_ENGINE_SOURCE_FILES ${BIFROST_ENGINE_SOURCE_FILES} "Engine/Runtime/lib/include/nativefiledialog/nfd_win.cpp")
elseif (APPLE)
  set(BIFROST_ENGINE_SOURCE_FILES ${BIFROST_ENGINE_SOURCE_FILES} "Engine/Runtime/lib/include/nativefiledialog/nfd_cocoa.m")
endif()

# NOTE(Shareef): Add the libraries we are linking against to the target.
# find_package(OpenGL REQUIRED)
find_package(OpenCL)

add_executable(
  BifrostEngineOLD
  ${BIFROST_ENGINE_SOURCE_FILES}
  
  "Examples/SceneEditor/main.cpp"
  "Engine/Runtime/src/graphics/bifrost_debug_renderer.cpp"
  "Engine/Runtime/src/core/bifrost_engine.cpp"
  
  "Engine/Runtime/src/ecs/bifrost_behavior.cpp"
  "Engine/Runtime/src/ecs/bifrost_behavior_system.cpp"
  "Engine/Runtime/src/ecs/bifrost_entity_ref.cpp"

  # Runtime

  "Engine/Editor/lib/include/ImGuizmo/ImGuizmo.cpp"

  # 'Game' Code
 "Examples/IK/camera_controller.cpp"

 "Engine/Runtime/src/graphics/bifrost_component_renderer.cpp"

 "Engine/Runtime/src/anim2D/bf_animation_system.cpp" 
 "Engine/Runtime/src/asset_io/bf_spritesheet_asset.cpp" 
  
  "Engine/AssetIO/include/bf/asset_io/bf_path_manip.hpp" 
  "Engine/Runtime/src/asset_io/bf_path_manip.cpp"
  "Engine/Runtime/include/bf/gfx/bf_render_queue.hpp" 
  "Engine/Runtime/src/bf_render_queue.cpp" 
  "Engine/Runtime/include/bf/ecs/bf_entity_storage.hpp" 
  "Engine/Runtime/src/core/bf_class_id.cpp"
   "Examples/SceneEditor/main_tests.cpp"
  "Engine/Runtime/src/editor/bifrost_editor_build.cpp"
)

add_executable(
  StandaloneRuntime
  "Examples/PhysicsSandbox/bf_runtime_driver.cpp"

  ${BIFROST_ENGINE_SOURCE_FILES}
  
  "Engine/Runtime/src/graphics/bifrost_debug_renderer.cpp"
  "Engine/Runtime/src/core/bifrost_engine.cpp"
  "Engine/Runtime/src/ecs/bifrost_behavior.cpp"
  "Engine/Runtime/src/ecs/bifrost_behavior_system.cpp"
  "Engine/Runtime/src/ecs/bifrost_entity_ref.cpp"

  # Runtime

  "${PROJECT_SOURCE_DIR}/Engine/Editor/lib/include/ImGuizmo/ImGuizmo.cpp"

  # 'Game' Code
 "Examples/IK/camera_controller.cpp"

 "Engine/Runtime/src/graphics/bifrost_component_renderer.cpp"

 "Engine/Runtime/src/anim2D/bf_animation_system.cpp" 
 "Engine/Runtime/src/asset_io/bf_spritesheet_asset.cpp" 
  
 "Engine/AssetIO/include/bf/asset_io/bf_path_manip.hpp" 
 "Engine/Runtime/src/asset_io/bf_path_manip.cpp"
  "Engine/Runtime/include/bf/gfx/bf_render_queue.hpp" 
  "Engine/Runtime/src/bf_render_queue.cpp" 
  "Engine/Runtime/include/bf/ecs/bf_entity_storage.hpp" 
  "Engine/Runtime/src/core/bf_class_id.cpp"
)


# if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#   target_compile_options(BifrostEngineOLD PRIVATE "/ZI")
#   target_link_options(BifrostEngineOLD PRIVATE "/SAFESEH:NO")
# endif()

set_target_properties(
  BifrostEngineOLD

  PROPERTIES
    LINK_FLAGS "/ignore:4099"
)

target_include_directories(
  BifrostEngineOLD
  PRIVATE
    ${PROJECT_SOURCE_DIR}/Engine/Editor/lib/include
    ${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/include
    ${PROJECT_SOURCE_DIR}/Engine/Runtime/include
    ${PROJECT_SOURCE_DIR}/Engine/Platform/lib/include
    ${PROJECT_BINARY_DIR} # For the cmake version file
    ${PROJECT_SOURCE_DIR}/Engine/Graphics2D/include 
)

target_link_libraries(
  BifrostEngineOLD
  PRIVATE
    BF_AssetIO
    BF_Core
    BF_Graphics

    BF_Platform_shared
    BF_Memory_interface
    BF_Text_static
    BF_TMPUtils
    BF_DataStructuresCxx
    BF_Math_shared
    bfAnimation2D_shared
    BifrostScript_shared
    BF_UI_shared
    BF_RuntimeGraphics

    BF_Job_static

    DearImGUI
)

set_target_properties(
  StandaloneRuntime

  PROPERTIES
    LINK_FLAGS "/ignore:4099"
)

target_include_directories(
  StandaloneRuntime
  PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/include
    ${PROJECT_SOURCE_DIR}/Runtime/include
    ${PROJECT_SOURCE_DIR}/Engine/Editor/lib/include
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/Engine/Runtime/include
    ${PROJECT_SOURCE_DIR}/Engine/Platform/lib/include
    ${PROJECT_BINARY_DIR} # For the cmake version file
    ${PROJECT_SOURCE_DIR}/Engine/Graphics2D/include 
    ${PROJECT_SOURCE_DIR}/Engine/Runtime/include

    "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/include"
)

target_link_libraries(
  StandaloneRuntime
  PRIVATE
    BF_AssetIO
    BF_Core
    BF_Graphics

    BF_Platform_shared
    BF_Memory_interface
    BF_Text_static
    BF_TMPUtils
    BF_DataStructuresCxx
    BF_Math_shared
    bfAnimation2D_shared
    BifrostScript_shared
    BF_UI_shared
    BF_RuntimeGraphics

    BF_Job_static

    DearImGUI

   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/LowLevel_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/LowLevelAABB_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/LowLevelDynamics_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysX_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXCharacterKinematic_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXCommon_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXCooking_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXExtensions_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXFoundation_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXPvdSDK_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXTask_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/PhysXVehicle_static_64.lib"
   # "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/SampleBase_static_64.lib"
   # "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/SampleFramework_static_64.lib"
   "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/SceneQuery_static_64.lib"
   # "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/SimulationController_static_64.lib"
   # "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/SnippetRender_static_64.lib"
   # "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/physx/win64/SnippetUtils_static_64.lib"
)

set_target_properties(BifrostEngineOLD PROPERTIES CXX_STANDARD 17)
set_target_properties(StandaloneRuntime PROPERTIES CXX_STANDARD 17)

  # NOTE(Shareef): Sets the working directory of the exe to be at the exe
  #   allowing us to load stuff from the assets folder.
set_target_properties(BifrostEngineOLD PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
set_target_properties(StandaloneRuntime PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  # NOTE(Shareef): Make the console not pop up.
# set_target_properties(BifrostEngine PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

# Copy over the assets folder. TODO: Instead of a copy make a symlink?
add_custom_command(TARGET BifrostEngineOLD PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:BifrostEngineOLD>/assets)

add_custom_command(TARGET StandaloneRuntime PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:BifrostEngineOLD>/assets)

add_custom_target(
    CopyAssets
    COMMAND 
      ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/assets"
)

add_dependencies(BifrostEngineOLD CopyAssets)

if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/win64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/win32")
    # set(BIFROST_ENGINE_FMOD_LIB "${BIFROST_ENGINE_LIB_DIR}/fmod_vc.lib")
  endif()
  set(BIFROST_ENGINE_LUA_LIB "${BIFROST_ENGINE_LIB_DIR}/lua53.lib")
  set(BIFROST_ENGINE_GLFW_LIB "${BIFROST_ENGINE_LIB_DIR}/glfw3dll.lib")

elseif (APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK")

  set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/macOS")
  set(BIFROST_ENGINE_GLFW_LIB "${PROJECT_SOURCE_DIR}/Engine/Runtime/lib/macOS/libglfw.3.dylib")
endif()

add_library(BF_RuntimeGraphics INTERFACE)

target_link_libraries(
  BF_RuntimeGraphics
  INTERFACE
    MachineIndependent
    OGLCompiler
    glslang
    SPIRV
)

# if(OpenCL_FOUND)
#   include_directories(${OPENCL_INCLUDE_DIR})
#   target_link_libraries(BifrostEngineOLD PRIVATE OpenCL::OpenCL)
#   #target_compile_definitions(my_target PRIVATE FOO=1 BAR=1)
#   add_compile_definitions(BIFROST_COMPUTE_OPENCL=1)
# else()
#   add_compile_definitions(BIFROST_COMPUTE_OPENCL=0)
# endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/lua53.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/fmodL64_vc.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/glfw3dll.lib")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
    # message(STATUS "We are building a DEBUG 32bit build...")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/lua53.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/fmodL_vc.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/glfw3dll.lib")
  endif()
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/lua53.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/fmod64_vc.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/glfw3dll.lib")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/lua53.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/fmod_vc.lib")
    # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/glfw3.lib")
  endif()
endif()

  # NOTE(Shareef): Automatically copy the needed dlls to the output directory
if (WIN32)
  # configure_file("${BIFROST_ENGINE_LIB_DIR}/glfw3.dll"                "${EXECUTABLE_OUTPUT_PATH}glfw3.dll"                COPYONLY)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
endif()

if (false)
add_custom_command(TARGET BifrostEngineOLD POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${BIFROST_ENGINE_LIB_DIR}/BifrostPlatform_shared.dll"
        $<TARGET_FILE_DIR:BifrostEngine>)
endif()



add_library(
  BF_JsonCpp
  STATIC
  
  "Engine/Runtime/include/bf/utility/bifrost_json.hpp"
  "Engine/Runtime/src/utility/bifrost_json.cpp"
  "Engine/Runtime/src/utility/bifrost_json.c"
)

target_include_directories(
  BF_JsonCpp
  PRIVATE
    "${PROJECT_SOURCE_DIR}/Runtime/include"
    "${PROJECT_SOURCE_DIR}/Experiments/include"
)

target_link_libraries(
  BF_JsonCpp
  PUBLIC
    BF_DataStructuresCxx
    BF_Memory_interface
    BF_TMPUtils
    BF_DataStructuresC_static
)

add_subdirectory(BifrostScript)
add_subdirectory(Engine)
add_subdirectory(Examples)
add_subdirectory(ThirdParty)
add_subdirectory(Tooling)
