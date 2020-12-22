#
# Runtime Type Information System
#

project(BF_MetaRTTI VERSION 1.0.0 DESCRIPTION "" LANGUAGES CXX)

add_library(BF_TMPUtils INTERFACE)

target_include_directories(
  BF_MetaRTTI

  INTERFACE
    "${PROJECT_SOURCE_DIR}/include/bf/bf_meta_function_traits.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_hash.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_meta_utils.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_non_copy_move.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_function_view.hpp"
    "${PROJECT_SOURCE_DIR}/include"
)

target_sources(
  BF_MetaRTTI

  INTERFACE
    "${PROJECT_SOURCE_DIR}/src/bf_hash.cpp"
)
