################################################################################
#                                                                              #
#                             BF GRAPHICS PROJECT                              #
#                                                                              #
################################################################################
###  CMakeList.txt : Graphics Sub Project of the Engine                      ###
################################################################################

cmake_minimum_required(VERSION 3.8)

option(BF_OPT_GRAPHICS_VULKAN "Vulkan will be used as the graphics Backend" ON)
option(BF_OPT_GRAPHICS_OPENGL "OpenGL will be used as the graphics Backend" OFF)

project(BF_Graphics VERSION 1.0.0 DESCRIPTION "The graphics abstraction layer sub project.")

set(
  BF_GRAPHICS_DEFINES
  ""
)

set(
  BF_GRAPHICS_INCLUDE_DIRS
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/lib/include"
)

set(
  BF_GRAPHICS_SOURCES
  
  # API Headers
  "${PROJECT_SOURCE_DIR}/include/bf/bf_gfx_api.h"
  "${PROJECT_SOURCE_DIR}/include/bf/bf_gfx_export.h"
  "${PROJECT_SOURCE_DIR}/include/bf/bf_gfx_handle.h"
  "${PROJECT_SOURCE_DIR}/include/bf/bf_gfx_limits.h"
  "${PROJECT_SOURCE_DIR}/include/bf/bf_gfx_object_cache.hpp"
  "${PROJECT_SOURCE_DIR}/include/bf/bf_gfx_pipeline_state.h"
  "${PROJECT_SOURCE_DIR}/include/bf/bf_gfx_types.h"

  # Shared Source
  "${PROJECT_SOURCE_DIR}/src/bf_gfx_common.c"
)

set(
  BF_GRAPHICS_LIBRARIES

  BF_DataStructuresC_static
  BF_Core_static
  BF_TMPUtils
)

if (BF_OPT_GRAPHICS_VULKAN)
  set(
    BF_GRAPHICS_SOURCES
    "${BF_GRAPHICS_SOURCES}"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_command_list.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_conversions.c"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_conversions.h"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_hash.hpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_logical_device.h"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_material_pool.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_material_pool.h"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_mem_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_mem_allocator.h"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_physical_device.h"
    "${PROJECT_SOURCE_DIR}/src/vulkan/bf_vulkan_renderpass.cpp"
    "${PROJECT_SOURCE_DIR}/src/bf_gfx_vulkan.cpp"
  )

  find_package(Vulkan)

  if(${Vulkan_FOUND})
    set(
      BF_GRAPHICS_INCLUDE_DIRS 
      "${BF_GRAPHICS_INCLUDE_DIRS}"
      "${Vulkan_INCLUDE_DIR}"
     )

     set(
       BF_GRAPHICS_LIBRARIES
       "${BF_GRAPHICS_LIBRARIES}"
       "${Vulkan_LIBRARIES}"
     )
  else()
    message(SEND_ERROR "Vulkan Not Found But is Required For The Vulkan Graphics Backend.")
  endif()

  if (WIN32)
    set(
      BF_GRAPHICS_DEFINES
      "${BF_GRAPHICS_DEFINES}"
      "VK_USE_PLATFORM_WIN32_KHR"
    )
  elseif(APPLE)
    set(
      BF_GRAPHICS_DEFINES
      "${BF_GRAPHICS_DEFINES}"
      "VK_USE_PLATFORM_MACOS_MVK"
    )
  else()
    message(SEND_ERROR "This is an unsupported Vulkan Platform.")
  endif()
elseif (BF_OPT_GRAPHICS_OPENGL)
  set(
    BF_GRAPHICS_SOURCES
    "${BF_GRAPHICS_SOURCES}"
    "src/bf_gfx_opengl.cpp"
  )

  set(
    BF_GRAPHICS_LIBRARIES
    "${BF_GRAPHICS_LIBRARIES}"
    BF_Memory_interface
  )
else()
  message(SEND_ERROR "Either 'BF_OPT_GRAPHICS_VULKAN' or 'BF_OPT_GRAPHICS_OPENGL' must be set as no other graphics backend is supported.")
endif()

add_library(
  BF_Graphics_static STATIC
  "${BF_GRAPHICS_SOURCES}"
)

target_compile_definitions(
  BF_Graphics_static
  PRIVATE
    "${BF_GRAPHICS_DEFINES}"
  PUBLIC
    BF_GFX_EXPORT_STATIC
)

target_include_directories(
  BF_Graphics_static 
  PUBLIC 
    "${BF_GRAPHICS_INCLUDE_DIRS}"
)

target_link_libraries(
  BF_Graphics_static
  PUBLIC
    BF_Platform_static
  PRIVATE
    "${BF_GRAPHICS_LIBRARIES}"
)

add_library(
  BF_Graphics_shared SHARED
  "${BF_GRAPHICS_SOURCES}"
)

target_compile_definitions(
  BF_Graphics_shared
  PRIVATE
    "${BF_GRAPHICS_DEFINES}"
    BF_GFX_EXPORT
)

target_include_directories(
  BF_Graphics_shared 
  PUBLIC 
    "${BF_GRAPHICS_INCLUDE_DIRS}"
)

target_link_libraries(
  BF_Graphics_shared
  PUBLIC
    BF_Platform_shared
  PRIVATE
    "${BF_GRAPHICS_LIBRARIES}"
)
