################################################################################
#                                                                              #
#                              BF MEMORY PROJECT                               #
#                                                                              #
################################################################################
###  CMakeList.txt : Memory Sub Project of the Engine                        ###
################################################################################

cmake_minimum_required(VERSION 3.12)

cmake_policy(PUSH)
cmake_policy(SET CMP0076 NEW)

project(BF_Memory VERSION 0.1.3 DESCRIPTION "Custom allocator interface for various allocation schemes.")

add_library(BifrostMemory INTERFACE)

target_include_directories(
  BifrostMemory
  INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)

target_sources(
  BifrostMemory
  INTERFACE
    "${PROJECT_SOURCE_DIR}/src/bifrost_c_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bifrost_freelist_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bifrost_imemory_manager.cpp"
    "${PROJECT_SOURCE_DIR}/src/bifrost_linear_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bifrost_memory_utils.c"
    "${PROJECT_SOURCE_DIR}/src/bifrost_pool_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bifrost_proxy_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bifrost_stack_allocator.cpp"
)

# set_target_properties(
#   BifrostMemory 
#   PROPERTIES
#     PUBLIC_HEADER include/bifrost/memory/bifrost_imemory_manager.hpp
# )

target_compile_features(
  BifrostMemory
  INTERFACE
    cxx_std_14
)

add_library(BF_Memory_interface ALIAS BifrostMemory)

cmake_policy(POP)
