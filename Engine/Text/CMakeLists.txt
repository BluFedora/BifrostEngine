cmake_minimum_required(VERSION 3.8)

project(BF_Text VERSION 1.0.0 DESCRIPTION "Handles unicode text encoding and font rasterization with a dynamic texture atlas system.")

# Static Lib

add_library(
  "${PROJECT_NAME}_static"
  STATIC
    # Headers
    "${PROJECT_SOURCE_DIR}/include/bf/Text.hpp"
    "${PROJECT_SOURCE_DIR}/include/bf/Font.hpp"
    # Sources
    "${PROJECT_SOURCE_DIR}/src/bf_text.cpp"
)

target_include_directories(
  "${PROJECT_NAME}_static"
  PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"
  PRIVATE
    "${PROJECT_SOURCE_DIR}/lib"
)

target_compile_features(
  "${PROJECT_NAME}_static"
  PRIVATE
    cxx_std_17
)

target_link_libraries(
  "${PROJECT_NAME}_static"
  PRIVATE
    BF_Memory_interface
)

set_target_properties(
  "${PROJECT_NAME}_static" 
  PROPERTIES
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/bf/Text.hpp"
)
