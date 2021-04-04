#
# Template Meta Programming Utilities
#

project(BF_TMPUtils VERSION 1.0.0 DESCRIPTION "Various self contained headers for compile-time programming fun." LANGUAGES CXX)

add_library(BF_TMPUtils INTERFACE)

target_include_directories(
  BF_TMPUtils

  INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)

target_sources(
  BF_TMPUtils

  INTERFACE
    "${PROJECT_SOURCE_DIR}/include/bf/bf_function_view.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_hash.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_meta_function_traits.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_meta_utils.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_non_copy_move.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/bf_poly.hpp"

    "${PROJECT_SOURCE_DIR}/src/bf_hash.cpp"
)

# Tests
add_executable(
  "${PROJECT_NAME}_poly"
  "${PROJECT_SOURCE_DIR}/tests/poly_main.cpp"
)
target_link_libraries(
  "${PROJECT_NAME}_poly"
  PRIVATE
    BF_TMPUtils
)
