cmake_minimum_required(VERSION 3.8)

project(BF_Core VERSION 0.0.1 DESCRIPTION "")

add_library(
  "${PROJECT_NAME}_static" STATIC

  # Headers
  "${PROJECT_SOURCE_DIR}/include/bf/Core.h"
  "${PROJECT_SOURCE_DIR}/include/bf/StringRange.h"

  # Sources
  "${PROJECT_SOURCE_DIR}/src/bf_core.c"
)

target_include_directories(
  "${PROJECT_NAME}_static"

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

# target_link_libraries(
#   "${PROJECT_NAME}_static"
#   PUBLIC
#     BF_Graphics_shared
#     BF_Memory_interface
# )
