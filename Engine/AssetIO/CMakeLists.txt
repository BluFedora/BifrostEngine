cmake_minimum_required(VERSION 3.8)

project(BF_AssetIO VERSION 0.0.1 DESCRIPTION "")

add_library(
  "${PROJECT_NAME}_static" STATIC

  "${PROJECT_SOURCE_DIR}/include/bf/asset_io/bf_model_loader.hpp"
  "${PROJECT_SOURCE_DIR}/include/bf/asset_io/bf_base_asset.hpp"
  "${PROJECT_SOURCE_DIR}/include/bf/asset_io/bf_gfx_assets.hpp"
 
  "${PROJECT_SOURCE_DIR}/src/bf_model_loader.cpp"
  "${PROJECT_SOURCE_DIR}/src/bf_gfx_assets.cpp"
 "src/bf_base_asset.cpp" "include/bf/asset_io/bf_asset_map.hpp")

target_include_directories(
  "${PROJECT_NAME}_static"

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/../../Runtime/include" # Fuck dude

  PRIVATE
    "${PROJECT_SOURCE_DIR}/lib/include"
)

set(BF_ASSETIO_LIBS "")

if (WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(
        BF_ASSETIO_LIBS
        "${BF_ASSETIO_LIBS}"
        "${PROJECT_SOURCE_DIR}/lib/win64/assimp-vc142-mtd.lib"
      )
    else()
      set(
        BF_ASSETIO_LIBS
        "${BF_ASSETIO_LIBS}"
        "${PROJECT_SOURCE_DIR}/lib/win64/assimp-vc142-mt.lib"
      )
    endif()
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
     if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(
        BF_ASSETIO_LIBS
        "${BF_ASSETIO_LIBS}"
        "${PROJECT_SOURCE_DIR}/lib/win32/assimp-vc142-mtd.lib"
      )
    else()
      set(
        BF_ASSETIO_LIBS
        "${BF_ASSETIO_LIBS}"
        "${PROJECT_SOURCE_DIR}/lib/win32/assimp-vc142-mt.lib"
      )
    endif()
  endif()
elseif(APPLE)
  message(SEND_ERROR "MacOS needs a build of the Assimp Model Loading Library.")
else()
  message(SEND_ERROR "Linux needs a build of the Assimp Model Loading Library.")
endif()


target_link_libraries(
  "${PROJECT_NAME}_static"
  PUBLIC
    BF_Core_static
    BF_Graphics_shared
    BF_Math_shared
    BF_Memory_interface
    BF_TMPUtils

    BF_DataStructuresCxx

  PRIVATE
    "${BF_ASSETIO_LIBS}"
)
