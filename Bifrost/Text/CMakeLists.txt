cmake_minimum_required(VERSION 3.8)

project(BF_Text VERSION 1.0.0 DESCRIPTION "Handles unicode text encoding and fint rasterization with a dynamic texture atlas system.")

# Static Lib

add_library(
  BF_Text_static 
  STATIC
    "${PROJECT_SOURCE_DIR}/src/bf_text.cpp"
)

target_include_directories(
  BF_Text_static
  PUBLIC 
    "${PROJECT_SOURCE_DIR}/include"

  PRIVATE
    "${PROJECT_SOURCE_DIR}/lib"
)

target_compile_features(
  BF_Text_static
  PRIVATE
    cxx_std_17
)

target_link_libraries(
  BF_Text_static
  PRIVATE
    BifrostMemory
)

set_target_properties(
  BF_Text_static 
  PROPERTIES
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/include/bf/Text.hpp"
)