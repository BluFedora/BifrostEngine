################################################################################

cmake_minimum_required(VERSION 3.8)

project(BF_Networking VERSION 0.0.1 DESCRIPTION "" LANGUAGES C CXX)

add_executable(
  BFNetworkTest
  "network_main.cpp"
)

target_include_directories(
  BFNetworkTest
  PRIVATE
    "${PROJECT_SOURCE_DIR}"
)

target_link_libraries(
  BFNetworkTest
  PRIVATE
    BF_Networking_static
)

add_library(
  BF_Networking_static
  STATIC
    "${PROJECT_SOURCE_DIR}/NetworkContext.cpp"
    "${PROJECT_SOURCE_DIR}/NetworkPlatform.cpp"
    "${PROJECT_SOURCE_DIR}/Socket.cpp"
)

target_include_directories(
  BF_Networking_static
  PUBLIC 
    "${PROJECT_SOURCE_DIR}"
)

set_target_properties(
  BF_Networking_static 
  PROPERTIES
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/NetworkContext.hpp"
)
