cmake_minimum_required(VERSION 3.8)

project(BF_AssetIO VERSION 0.0.1 DESCRIPTION "")

add_library(
  "${PROJECT_NAME}_static" STATIC
  "${PROJECT_SOURCE_DIR}/src/bf_model_loader.cpp"
)

target_include_directories(
  "${PROJECT_NAME}_static"

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"

  PRIVATE
    "${PROJECT_SOURCE_DIR}/lib/include"
)

set(BF_ASSETIO_LIBS "")

if (WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
     set(
       BF_ASSETIO_LIBS
       "${BF_ASSETIO_LIBS}"
       "${PROJECT_SOURCE_DIR}/lib/win64/assimp-vc142-mt.lib"
     )
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
     set(
       BF_ASSETIO_LIBS
       "${BF_ASSETIO_LIBS}"
       "${PROJECT_SOURCE_DIR}/lib/win32/assimp-vc142-mt.lib"
     )
  endif()
elseif(APPLE)
  message(SEND_ERROR "MacOS needs a build of the Assimp Model Loading Library.")
else()
  message(SEND_ERROR "Linux needs a build of the Assimp Model Loading Library.")
endif()


target_link_libraries(
  "${PROJECT_NAME}_static"
  PUBLIC
    BF_Memory_interface
    BF_Core_static

  PRIVATE
    "${BF_ASSETIO_LIBS}"
)
