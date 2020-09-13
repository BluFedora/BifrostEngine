################################################################################
#                                                                              #
#                               BF CMAKE PROJECT                               #
#                                                                              #
################################################################################
###  CMakeList.txt : Top-level CMake project file, for global configuration  ###
################################################################################

  # NOTE(Shareef): This sets the minimum version of CMake that must be used
cmake_minimum_required(VERSION 3.8)

project(BluFedoraEngine VERSION 1.0)

  # NOTE(Shareef): Engine Version
set(BF_ENGINE_VERSION_MAJOR 1)
set(BF_ENGINE_VERSION_MINOR 3)
set(BF_ENGINE_VERSION_PATCH 0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # This will generate a header file with the version info.
  # The file will be made in the build directory hence why the build
  # dir needs to be added to the includes.
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/bifrost_version.h.in"
  "${PROJECT_BINARY_DIR}/bifrost/bifrost_version.h"
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
  set(CMAKE_CXX_FLAGS "/wd4996 /Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812 /permissive- /vmg")
  set(CMAKE_C_FLAGS "/wd4996 /Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812 /permissive- /vmg")
endif()

  # NOTE(Shareef): Automatically generate a list of ALL source (C/++) files based of off what is in the folders.
file(GLOB_RECURSE BIFROST_ENGINE_SOURCE_FILES

  # Libs
  "lib/include/imgui/imgui.cpp"
  "lib/include/imgui/imgui_widgets.cpp"
  "lib/include/imgui/imgui_draw.cpp"
  "lib/include/imgui/imgui_demo.cpp"
  "lib/include/nativefiledialog/nfd_common.c"

  # Experiments
  "BifrostExperiments/src/*.c"

  # Bifrost Asset Module
  "BifrostRuntime/src/asset_io/*.cpp"

  # Bifrost Core Module
  "BifrostRuntime/src/core/bifrost_game_state_machine.cpp"
  "BifrostRuntime/src/core/bifrost_igame_state_layer.cpp"

  # Bifrost Debug Module
  "BifrostRuntime/src/debug/bifrost_dbg_logger.c"

  # Bifrost ECS Module
  "BifrostRuntime/src/ecs/bifrost_entity.cpp"
  "Engine/Runtime/src/ecs/bf_base_component.cpp"
  "BifrostRuntime/src/ecs/bifrost_iecs_system.cpp"

  # Bifrost Editor Module
  "include/bifrost/editor/*.hpp"
  "BifrostRuntime/src/editor/bifrost_editor_build.cpp"
  "BifrostRuntime/src/bifrost_imgui_glfw.cpp"

  # Bifrost Graphics Module
  "BifrostRuntime/src/graphics/bifrost_standard_renderer.cpp"
  "BifrostRuntime/src/graphics/bifrost_glsl_compiler.cpp"

  # Bifrost Math Module
  "Engine/Math/src/bifrost_rect2.cpp"

  # Bifrost Meta Module
  "BifrostRuntime/src/meta/bifrost_meta_runtime.cpp"

  # Bifrost Utility Module
  "BifrostRuntime/src/utility/bifrost_hash.cpp"
  "BifrostRuntime/src/utility/bifrost_json.cpp"
  "BifrostRuntime/src/utility/bifrost_uuid.c"

  # Physics
  "BifrostRuntime/src/bifrost/physics/bifrost_particle.cpp"

  "main.cpp"
 )

if (WIN32)
  set(BIFROST_ENGINE_SOURCE_FILES ${BIFROST_ENGINE_SOURCE_FILES} "lib/include/nativefiledialog/nfd_win.cpp")
elseif (APPLE)
  set(BIFROST_ENGINE_SOURCE_FILES ${BIFROST_ENGINE_SOURCE_FILES} "lib/include/nativefiledialog/nfd_cocoa.m")
endif()

# NOTE(Shareef): Add the libraries we are linking against to the target.
# find_package(OpenGL REQUIRED)
find_package(OpenCL)

add_executable(
  BifrostEngineOLD
  ${BIFROST_ENGINE_SOURCE_FILES}
  "BifrostRuntime/src/ecs/bifrost_collision_system.cpp"
  "BifrostRuntime/src/graphics/bifrost_debug_renderer.cpp"
  "BifrostRuntime/src/core/bifrost_engine.cpp"
  "DemoApp/src/main_demo.cpp"
  "BifrostRuntime/src/ecs/bifrost_behavior.cpp"
  "BifrostRuntime/src/ecs/bifrost_behavior_system.cpp"
  "BifrostRuntime/src/ecs/bifrost_entity_ref.cpp"

  # Runtime

  "${PROJECT_SOURCE_DIR}/BifrostRuntime/src/bifrost_script_behavior.cpp"
  "${PROJECT_SOURCE_DIR}/BifrostRuntime/src/bifrost_script.cpp"
  "${PROJECT_SOURCE_DIR}/Engine/Editor/lib/include/ImGuizmo/ImGuizmo.cpp"

  # 'Game' Code
 "DemoApp/src/camera_controller.cpp"

 "BifrostRuntime/src/graphics/bifrost_component_renderer.cpp"

 "${PROJECT_SOURCE_DIR}/Engine/Graphics2D/src/bf_painter.cpp" 
 "${PROJECT_SOURCE_DIR}/Engine/Runtime/src/anim2D/bf_animation_system.cpp" 
 "${PROJECT_SOURCE_DIR}/Engine/Runtime/include/bf/asset_io/bf_spritesheet_asset.hpp" 
 "Engine/Runtime/src/asset_io/bf_spritesheet_asset.cpp" 
 "Engine/Runtime/src/asset_io/bf_asset_info.cpp" 
 "Engine/Runtime/include/bf/asset_io/bf_path_manip.hpp" 
 "Engine/Runtime/src/asset_io/bf_path_manip.cpp"
 )

# if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#   target_compile_options(BifrostEngineOLD PRIVATE "/ZI")
#   target_link_options(BifrostEngineOLD PRIVATE "/SAFESEH:NO")
# endif()

# Bifrost Engine Submodules

target_include_directories(
  BifrostEngineOLD
  PRIVATE
    ${PROJECT_SOURCE_DIR}/lib/include
    ${PROJECT_SOURCE_DIR}/BifrostRuntime/include
    ${PROJECT_SOURCE_DIR}/Engine/Editor/lib/include
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/BifrostExperiments/include
    ${PROJECT_SOURCE_DIR}/DemoApp/include
    ${PROJECT_SOURCE_DIR}/Engine/Runtime/include
    ${PROJECT_SOURCE_DIR}/Engine/Platform/lib/include
    ${PROJECT_BINARY_DIR} # For the cmake version file
    ${PROJECT_SOURCE_DIR}/Engine/Graphics2D/include 
    ${PROJECT_SOURCE_DIR}/Engine/Runtime/include
)

target_link_libraries(
  BifrostEngineOLD
  PRIVATE
    BF_AssetIO_static
    BF_Core_static
    BF_Platform_shared
    BF_Text_static

    BifrostDataStructuresCxx
    BifrostScript_shared
    BifrostMath_shared
    BifrostMemory
    bfAnimation2D_shared

    BifrostGraphics_shared
)

set_target_properties(BifrostEngineOLD PROPERTIES CXX_STANDARD 17)

  # NOTE(Shareef): Sets the working directory of the exe to be at the exe
  #   allowing us to load stuff from the assets folder.
set_target_properties(BifrostEngineOLD PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  # NOTE(Shareef): Make the console not pop up.
# set_target_properties(BifrostEngine PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

# Copy over the assets folder. TODO: Instead of a copy make a symlink?
add_custom_command(TARGET BifrostEngineOLD PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:BifrostEngineOLD>/assets)

if (WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")

  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/lib/win64")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/lib/win32")
    # set(BIFROST_ENGINE_FMOD_LIB "${BIFROST_ENGINE_LIB_DIR}/fmod_vc.lib")
  endif()
  set(BIFROST_ENGINE_LUA_LIB "${BIFROST_ENGINE_LIB_DIR}/lua53.lib")
  set(BIFROST_ENGINE_GLFW_LIB "${BIFROST_ENGINE_LIB_DIR}/glfw3dll.lib")

elseif (APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK")

  set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/lib/macOS")
  set(BIFROST_ENGINE_GLFW_LIB "${PROJECT_SOURCE_DIR}/lib/macOS/libglfw.3.dylib")
endif()

if (WIN32)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/glslangd.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/HLSLd.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/OGLCompilerd.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/OSDependentd.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPIRVd.lib")
    # target_link_libraries(BifrostEngine PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPIRV-Tools.lib")
    # target_link_libraries(BifrostEngine PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPIRV-Tools-opt.lib")
    # target_link_libraries(BifrostEngine PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPVRemapperd.lib")
  else()
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/glslang.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/HLSL.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/OGLCompiler.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/OSDependent.lib")
    target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPIRV.lib")
    # target_link_libraries(BifrostEngine PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPIRV-Tools.lib")
    # target_link_libraries(BifrostEngine PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPIRV-Tools-opt.lib")
    # target_link_libraries(BifrostEngine PRIVATE "${BIFROST_ENGINE_LIB_DIR}/SPVRemapper.lib")
  endif()
  # target_link_libraries(BifrostEngine PRIVATE "opengl32.lib")
elseif (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit")
  target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/libglslang.a")
  target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/libHLSL.a")
  target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/libOGLCompiler.a")
  target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/libSPIRV.a")
  target_link_libraries(BifrostEngineOLD PRIVATE "${BIFROST_ENGINE_LIB_DIR}/libOSDependent.a")
endif()


if(OpenCL_FOUND)
  include_directories(${OPENCL_INCLUDE_DIR})
  target_link_libraries(BifrostEngineOLD PRIVATE OpenCL::OpenCL)
  #target_compile_definitions(my_target PRIVATE FOO=1 BAR=1)
  add_compile_definitions(BIFROST_COMPUTE_OPENCL=1)
else()
  add_compile_definitions(BIFROST_COMPUTE_OPENCL=0)
endif()

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
  # configure_file("${BIFROST_ENGINE_LIB_DIR}/BifrostScript_shared.dll" "${EXECUTABLE_OUTPUT_PATH}BifrostScript_shared.dll" COPYONLY)
  # configure_file("${BIFROST_ENGINE_LIB_DIR}/BifrostMath_shared.dll"   "${EXECUTABLE_OUTPUT_PATH}BifrostMath_shared.dll"   COPYONLY)
  # configure_file("${BIFROST_ENGINE_LIB_DIR}/BifrostPlatform_shared.dll"   "${EXECUTABLE_OUTPUT_PATH}BifrostPlatform_shared.dll"   COPYONLY)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN64}/fmodL64.dll" "${EXECUTABLE_OUTPUT_PATH}fmodL64.dll" COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN64}/fmod64.dll"  "${EXECUTABLE_OUTPUT_PATH}fmod64.dll"  COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN64}/glfw3.dll"   "${EXECUTABLE_OUTPUT_PATH}glfw3.dll" COPYONLY)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN32}/fmodL.dll" "${EXECUTABLE_OUTPUT_PATH}fmodL.dll" COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN32}/fmod.dll"  "${EXECUTABLE_OUTPUT_PATH}fmod.dll"  COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN32}/glfw3.dll" "${EXECUTABLE_OUTPUT_PATH}glfw3.dll" COPYONLY)
endif()

if (false)
add_custom_command(TARGET BifrostEngineOLD POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${BIFROST_ENGINE_LIB_DIR}/BifrostPlatform_shared.dll"
        $<TARGET_FILE_DIR:BifrostEngine>)
endif()

add_subdirectory(Engine/Anim2D)
add_subdirectory(Engine/AssetIO)
add_subdirectory(Engine/Core)
add_subdirectory(Engine/DataStructures)
add_subdirectory(Engine/Dispatch)
add_subdirectory(Engine/Math)
add_subdirectory(Engine/Memory)
add_subdirectory(Engine/Network)
add_subdirectory(Engine/Platform)
add_subdirectory(Engine/Runtime)
add_subdirectory(Engine/Text)
add_subdirectory(Engine/Job)

add_subdirectory(Games/DungeonSwiper)

add_subdirectory(BifrostExperiments)
add_subdirectory(BifrostGraphics)
add_subdirectory(BifrostScript)

add_subdirectory(BuildTimeAnalyzerGUI)
