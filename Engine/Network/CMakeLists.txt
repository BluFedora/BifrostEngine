################################################################################
#                                                                              #
#                             BF NETWORK PROJECT                               #
#                                                                              #
################################################################################
###  CMakeList.txt : Layer over the berkeley api on Win32 and Posix.         ###
################################################################################
cmake_minimum_required(VERSION 3.8)

project(BF_Networking
  VERSION     0.0.1 
  DESCRIPTION "Cross platform layer over the berkeley api on Win32 and Posix." 
  LANGUAGES   CXX
)

add_library(
  BF_Networking
  STATIC
   "bf/bf_net.hpp"
   "bf/bf_net.cpp"
)

target_include_directories(
  BF_Networking
  PUBLIC 
    "${PROJECT_SOURCE_DIR}"
)

set_target_properties(
  BF_Networking
  PROPERTIES
    PUBLIC_HEADER "${PROJECT_SOURCE_DIR}/bf/bf_net.hpp"
)
