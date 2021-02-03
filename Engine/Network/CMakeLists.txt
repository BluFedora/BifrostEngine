################################################################################

cmake_minimum_required(VERSION 3.8)

project(BF_Networking VERSION 0.0.1 DESCRIPTION "" LANGUAGES C CXX)

add_library(
  BF_Networking_static
  STATIC
   "bf/bf_net.hpp"
   "bf/bf_net.cpp"
)

target_include_directories(
  BF_Networking_static
  PUBLIC 
    "${PROJECT_SOURCE_DIR}"
)

set_target_properties(
  BF_Networking_static 
  PROPERTIES
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/bf/bf_net.hpp"
)
