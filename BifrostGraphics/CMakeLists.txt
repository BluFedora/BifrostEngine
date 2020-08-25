################################################################################
#                                                                              #
#                          BIFROST GRAPHICS PROJECT                            #
#                                                                              #
################################################################################
###  CMakeList.txt : Bifrost Graphics Sub Project of the Bifrost Engine      ###
################################################################################

cmake_minimum_required(VERSION 3.8)

option(BIFROST_OPT_GRAPHICS_VULKAN "Vulkan will be used as the graphics Backend" ON)
option(BIFROST_OPT_GRAPHICS_OPENGL "OpenGL will be used as the graphics Backend" OFF)

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

project(BifrostGraphics VERSION 1.0.0 DESCRIPTION "The graphics abstraction layer sub project.")

set(
  BIFROST_GRAPHICS_DEFINES
  ""
)

set(
  BIFROST_GRAPHICS_INCLUDE_DIRS
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/lib/include"
  "${CMAKE_SOURCE_DIR}/BifrostRuntime/include"
)

set(
  BIFROST_GRAPHICS_SOURCES
  "src/bifrost_gfx_common.c"
  "${CMAKE_SOURCE_DIR}/BifrostRuntime/src/utility/bifrost_hash.cpp"
)

set(
  BIFROST_GRAPHICS_LIBRARIES
  BifrostDataStructuresCxx
)

if (BIFROST_OPT_GRAPHICS_VULKAN)
  set(
    BIFROST_GRAPHICS_SOURCES
    "${BIFROST_GRAPHICS_SOURCES}"
    "src/vulkan/bifrost_vulkan_command_list.cpp"
    "src/vulkan/bifrost_vulkan_conversions.c"
    "src/vulkan/bifrost_vulkan_material_pool.cpp"
    "src/vulkan/bifrost_vulkan_mem_allocator.cpp"
    "src/vulkan/bifrost_vulkan_renderpass.cpp"
    "src/bifrost_gfx_vulkan.cpp"

    "${CMAKE_SOURCE_DIR}/BifrostRuntime/src/debug/bifrost_dbg_logger.c"
  )

  find_package(Vulkan)

  if(${Vulkan_FOUND})
    set(
      BIFROST_GRAPHICS_INCLUDE_DIRS 
      "${BIFROST_GRAPHICS_INCLUDE_DIRS}"
      "${Vulkan_INCLUDE_DIR}"
     )

     set(
       BIFROST_GRAPHICS_LIBRARIES
       "${BIFROST_GRAPHICS_LIBRARIES}"
       "${Vulkan_LIBRARIES}"
     )
  else()
    message(SEND_ERROR "Vulkan Not Found But is Required For The Vulkan Graphics Backend.")
  endif()

  if (WIN32)
    set(
      BIFROST_GRAPHICS_DEFINES
      "${BIFROST_GRAPHICS_DEFINES}"
      "VK_USE_PLATFORM_WIN32_KHR"
    )
  elseif(APPLE)
    set(
      BIFROST_GRAPHICS_DEFINES
      "${BIFROST_GRAPHICS_DEFINES}"
      "VK_USE_PLATFORM_MACOS_MVK"
    )
  else()
    message(SEND_ERROR "This is an unsupported Vulkan Platform.")
  endif()
elseif (BIFROST_OPT_GRAPHICS_OPENGL)
  set(
    BIFROST_GRAPHICS_SOURCES
    "${BIFROST_GRAPHICS_SOURCES}"
    "src/bifrost_gfx_opengl.cpp"
  )

  set(
    BIFROST_GRAPHICS_LIBRARIES
    "${BIFROST_GRAPHICS_LIBRARIES}"
    BifrostMemory
  )
else()
  message(SEND_ERROR "Either 'BIFROST_OPT_GRAPHICS_VULKAN' or 'BIFROST_OPT_GRAPHICS_OPENGL' must be set as no other graphics backend is supported.")
endif()

add_library(
  BifrostGraphics_static STATIC
  "${BIFROST_GRAPHICS_SOURCES}"
)

target_compile_definitions(
  BifrostGraphics_static
  PRIVATE
    "${BIFROST_GRAPHICS_DEFINES}"
    BIFROST_GRAPHICS_NO_LOG
  PUBLIC
    BIFROST_GFX_EXPORT_STATIC
)

target_include_directories(
  BifrostGraphics_static 
  PUBLIC 
    "${BIFROST_GRAPHICS_INCLUDE_DIRS}"
)

target_link_libraries(
  BifrostGraphics_static
  PUBLIC
    BifrostPlatform_static
  PRIVATE
    "${BIFROST_GRAPHICS_LIBRARIES}"
)

add_library(
  BifrostGraphics_shared SHARED
  "${BIFROST_GRAPHICS_SOURCES}"
)

target_compile_definitions(
  BifrostGraphics_shared
  PRIVATE
    "${BIFROST_GRAPHICS_DEFINES}"
    BIFROST_GFX_EXPORT
    BIFROST_GRAPHICS_NO_LOG
)

target_include_directories(
  BifrostGraphics_shared 
  PUBLIC 
    "${BIFROST_GRAPHICS_INCLUDE_DIRS}"
)

target_link_libraries(
  BifrostGraphics_shared
  PUBLIC
    BifrostPlatform_shared
  PRIVATE
    "${BIFROST_GRAPHICS_LIBRARIES}"
)
