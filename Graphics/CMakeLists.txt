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
  BIFROST_GRAPHICS_DEFINES
  ""
)

set(
  BIFROST_GRAPHICS_INCLUDE_DIRS
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/lib/include"
  "${CMAKE_SOURCE_DIR}/Runtime/include"
)

set(
  BIFROST_GRAPHICS_SOURCES
  "src/bifrost_gfx_common.c"
)

set(
  BF_GRAPHICS_LIBRARIES

  BF_DataStructuresC_static
  BF_Core_static
  BF_TMPUtils
)

if (BF_OPT_GRAPHICS_VULKAN)
  set(
    BIFROST_GRAPHICS_SOURCES
    "${BIFROST_GRAPHICS_SOURCES}"
    "src/vulkan/bifrost_vulkan_command_list.cpp"
    "src/vulkan/bifrost_vulkan_conversions.c"
    "src/vulkan/bifrost_vulkan_material_pool.cpp"
    "src/vulkan/bifrost_vulkan_mem_allocator.cpp"
    "src/vulkan/bifrost_vulkan_renderpass.cpp"
    "src/bifrost_gfx_vulkan.cpp"

    "${CMAKE_SOURCE_DIR}/Runtime/src/debug/bifrost_dbg_logger.c"
  )

  find_package(Vulkan)

  if(${Vulkan_FOUND})
    set(
      BIFROST_GRAPHICS_INCLUDE_DIRS 
      "${BIFROST_GRAPHICS_INCLUDE_DIRS}"
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
elseif (BF_OPT_GRAPHICS_OPENGL)
  set(
    BIFROST_GRAPHICS_SOURCES
    "${BIFROST_GRAPHICS_SOURCES}"
    "src/bifrost_gfx_opengl.cpp"
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
    BF_Platform_static
  PRIVATE
    "${BF_GRAPHICS_LIBRARIES}"
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
    BF_Platform_shared
  PRIVATE
    "${BF_GRAPHICS_LIBRARIES}"
)

add_library("${PROJECT_NAME}_static" ALIAS BifrostGraphics_static)
add_library("${PROJECT_NAME}_shared" ALIAS BifrostGraphics_shared)
