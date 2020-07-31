################################################################################
#                                                                              #
#                          BIFROST PLATFORM PROJECT                            #
#                                                                              #
################################################################################
###  CMakeList.txt : Bifrost Platform Sub Project of the Bifrost Engine      ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostPlatform VERSION 0.0.2 DESCRIPTION "The platform abstraction layer sub project.")

option(BIFROST_PLATFORM_GLFW       "This will use GLFW for windowing and system functions." ON)
option(BIFROST_PLATFORM_SDL        "This will use SDL for windowing and system functions."  OFF)
option(BIFROST_PLATFORM_QT         "This will use QT for windowing and system functions."   OFF)
option(BIFROST_OPT_GRAPHICS_VULKAN "Vulkan will be used as the graphics Backend"            ON)
option(BIFROST_OPT_GRAPHICS_OPENGL "OpenGL will be used as the graphics Backend"            OFF)

  # GCC / Clang
  #   "/GR-"                      - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "/D_***_SECURE_NO_WARNINGS" - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "-std=c++17"                = "/std:c++17"
  #   "-fno-rtti"                 = "/GR-"
  #   TODO(Shareef): /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs -fno-rtti -Wall -g")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # NOTE(Shareef): Setting the compiler flags we will be using.
  #
  #   "/Zc:__cplusplus" - Report the correct '__cplusplus' macro on msvc.
  #   "/WX"   - treat warnings as errors.
  #   "/EHsc" - is for enabling C++ exceptions which we will NOT be using.
  #     Reason being the C++ STL uses exceptions like a mother f*cker and with warnings as
  #   errors on this would not build other wise.
  #   (alternative is to define: _HAS_EXCEPTIONS but that has some other implications.)
  #   "/GR-"  - Disables RTTI, trying to see if we can do without it for this engine.
  #   "/D_***_SECURE_NO_WARNINGS" - Disable the warnings for not using the "secure"
  #      versions of certain functions.
  #   "/std:c++17" - I needed some new C++17 features for some tEmPlAtE mAgIc
  #   "/GF"        - String Pooling is very important for the way the Material System works. I want stable
  #                  addresses. ("Eliminate Duplicate Strings")
  #   "/MP[N]"     - N is optional, will just use all the cores if omitted. Allows for
  #                  compilation on mutiple cores. Should give us a big speed-up on compile times.
  #
  set(CMAKE_CXX_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
  set(CMAKE_C_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
endif()

if (WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
     set(BIFROST_PLATFORM_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/win64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
     set(BIFROST_PLATFORM_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/win32)
  endif()
elseif(APPLE)
  set(BIFROST_PLATFORM_LIB_DIR ${CMAKE_SOURCE_DIR}/lib/macOS)
endif()

if (WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
     set(BIFROST_PLATFORM_LIB ${PROJECT_SOURCE_DIR}/lib/win64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
     set(BIFROST_PLATFORM_LIB ${PROJECT_SOURCE_DIR}/lib/win32)
  endif()
elseif(APPLE)
  set(BIFROST_PLATFORM_LIB ${PROJECT_SOURCE_DIR}/lib/macOS)
endif()

set(BIFROST_PLATFORM_SOURCE_FILES "src/bifrost_platform.c")
set(BIFROST_PLATFORM_LIB_FILES "")

if(BIFROST_PLATFORM_GLFW)
  set(BIFROST_PLATFORM_SOURCE_FILES
    ${BIFROST_PLATFORM_SOURCE_FILES}
    "src/bifrost_platform_glfw.c"
  )

  if(EMSCRIPTEN)
    # TODO(SR): Add Compiler flags here.
  elseif (WIN32)
    set(BIFROST_PLATFORM_LIB_FILES
      ${BIFROST_PLATFORM_LIB_FILES}
      "${BIFROST_PLATFORM_LIB_DIR}/glfw3dll.lib"
    )
  elseif(APPLE)
    set(BIFROST_PLATFORM_LIB_FILES
      ${BIFROST_PLATFORM_LIB_FILES}
      "${BIFROST_PLATFORM_LIB_DIR}/libglfw.3.dylib"
    )
  endif()

elseif(BIFROST_PLATFORM_SDL)
  set(BIFROST_PLATFORM_SOURCE_FILES
    ${BIFROST_PLATFORM_SOURCE_FILES}
    "src/bifrost_platform_sdl.c"
  )

  if(EMSCRIPTEN)
  elseif (WIN32)
    set(BIFROST_PLATFORM_LIB_FILES
      ${BIFROST_PLATFORM_LIB_FILES}
      "${BIFROST_PLATFORM_LIB}/SDL2.lib"
    )
  elseif(APPLE)
    set(BIFROST_PLATFORM_LIB_FILES
      ${BIFROST_PLATFORM_LIB_FILES}
      # "${BIFROST_PLATFORM_LIB_DIR}/libglfw.3.dylib"
    )
  endif()

elseif(BIFROST_PLATFORM_QT)
  set(BIFROST_PLATFORM_SOURCE_FILES
    ${BIFROST_PLATFORM_SOURCE_FILES}
    "src/bifrost_platform_qt.c"
  )
else()
endif()

add_library(
  BifrostPlatform_static STATIC
  ${BIFROST_PLATFORM_SOURCE_FILES}
)

target_include_directories(
  BifrostPlatform_static
   INTERFACE ${PROJECT_SOURCE_DIR}/include
   PRIVATE   ${PROJECT_SOURCE_DIR}/include
)

if(BIFROST_OPT_GRAPHICS_VULKAN)
  find_package(Vulkan)

  if(${Vulkan_FOUND})
    target_include_directories(
      BifrostPlatform_static 
      PRIVATE 
        "${Vulkan_INCLUDE_DIR}"
     )
  endif()

  target_compile_definitions(
    BifrostPlatform_static
    PRIVATE
      GLFW_INCLUDE_VULKAN
      BIFROST_PLATFORM_USE_VULKAN=1
  )
else()
  target_compile_definitions(
    BifrostPlatform_static
    PRIVATE
      GLFW_INCLUDE_VULKAN
      BIFROST_PLATFORM_USE_OPENGL=1
  )
endif()

target_link_libraries(
  BifrostPlatform_static
  PUBLIC 
    "${BIFROST_PLATFORM_LIB_FILES}"
)

target_compile_definitions(
  BifrostPlatform_static
  PUBLIC 
    BIFROST_PLATFORM_EXPORT_STATIC
)

set_target_properties(BifrostPlatform_static PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(BifrostPlatform_static PROPERTIES PUBLIC_HEADER include/bifrost/platform/bifrost_platform.h)
set_target_properties(BifrostPlatform_static PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Shared Library

if(NOT EMSCRIPTEN)
  add_library(
      BifrostPlatform_shared SHARED
      ${BIFROST_PLATFORM_SOURCE_FILES}
  )

  target_include_directories(
      BifrostPlatform_shared
      INTERFACE ${PROJECT_SOURCE_DIR}/include
      PRIVATE   ${PROJECT_SOURCE_DIR}/include
  )

  target_link_libraries(
      BifrostPlatform_shared
      PUBLIC 
        "${BIFROST_PLATFORM_LIB_FILES}"
  )

  target_compile_definitions(BifrostPlatform_shared PRIVATE BIFROST_PLATFORM_EXPORT)

if(BIFROST_OPT_GRAPHICS_VULKAN)
  if(${Vulkan_FOUND})
    target_include_directories(
      BifrostPlatform_shared 
      PRIVATE 
        "${Vulkan_INCLUDE_DIR}"
     )
  endif()

  target_compile_definitions(
    BifrostPlatform_shared
    PRIVATE
      GLFW_INCLUDE_VULKAN
    PUBLIC
      BIFROST_PLATFORM_USE_VULKAN=1
  )
else()
  target_compile_definitions(
    BifrostPlatform_shared
    PUBLIC
      BIFROST_PLATFORM_USE_OPENGL=1
  )
endif()

  set_target_properties(BifrostPlatform_shared PROPERTIES PUBLIC_HEADER include/bifrost/platform/bifrost_platform.h)
endif()
