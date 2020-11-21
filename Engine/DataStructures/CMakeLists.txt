################################################################################
#                                                                              #
#                         BF DataStructures PROJECT                            #
#                                                                              #
################################################################################
###  CMakeList.txt :                                                         ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BF_DataStructures VERSION 0.0.1 DESCRIPTION "" LANGUAGES C CXX)

add_library(
  BF_DataStructuresC_static STATIC
  "${PROJECT_SOURCE_DIR}/src/bifrost_array_t.c"
  "${PROJECT_SOURCE_DIR}/src/bifrost_dynamic_string.c"
)

target_include_directories(
  BF_DataStructuresC_static

  PUBLIC
    "${PROJECT_SOURCE_DIR}/include/"
)

add_library(BF_DataStructuresCxx INTERFACE)

target_include_directories(
  BF_DataStructuresCxx

  INTERFACE 
    "${PROJECT_SOURCE_DIR}/include/"
)

target_sources(
  BF_DataStructuresCxx
  INTERFACE
    "${PROJECT_SOURCE_DIR}/src/bifrost_string.cpp"
)

target_link_libraries(
  BF_DataStructuresCxx 

  INTERFACE
    BF_Core_static
    BF_DataStructuresC_static
    BF_Memory_interface
)
