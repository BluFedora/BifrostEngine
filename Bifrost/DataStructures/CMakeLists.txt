################################################################################
#                                                                              #
#                      BIFROST DataStructures PROJECT                          #
#                                                                              #
################################################################################
###  CMakeList.txt :                                                         ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostDataStructures VERSION 0.0.1 DESCRIPTION "" LANGUAGES C CXX)

add_library(
  BifrostDataStructuresC_static STATIC
  "src/bifrost_array_t.c"
  "src/bifrost_dynamic_string.c"
  "src/bifrost_hash_map.c"
)

target_include_directories(
  BifrostDataStructuresC_static

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)

add_library(BifrostDataStructuresCxx INTERFACE)

target_include_directories(
  BifrostDataStructuresCxx
  INTERFACE 
    ${PROJECT_SOURCE_DIR}/include
)

target_sources(
  BifrostDataStructuresCxx
  INTERFACE
    "${PROJECT_SOURCE_DIR}/src/bifrost_string.cpp"
)

target_link_libraries(
  BifrostDataStructuresCxx 

  INTERFACE
    BifrostMemory
    BifrostDataStructuresC_static
)
