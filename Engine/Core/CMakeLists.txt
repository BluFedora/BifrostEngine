#
# Core unitilies that are allowed to be used throughout the codebase
# to allow for a standard shared set of functionality.
#
# EX: Logging, StringRange, Assert, Various Macros.
#
cmake_minimum_required(VERSION 3.8)

project(BF_Core VERSION 0.0.1 DESCRIPTION "Foundation library for the codebase to have a consistent API.")

add_library(
  "${PROJECT_NAME}" SHARED

  # Headers
    "${PROJECT_SOURCE_DIR}/include/bf/bf_core.h"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_core_export.h"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_dbg_logger.h"

  # Sources
    "${PROJECT_SOURCE_DIR}/src/bf_dbg_logger.c"
 "include/bf/bf_api_types.h")

target_compile_definitions(
  "${PROJECT_NAME}"
  PRIVATE 
    BF_CORE_EXPORT
)

target_include_directories(
  "${PROJECT_NAME}"

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
