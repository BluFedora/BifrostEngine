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

add_library("${PROJECT_NAME}_interface" INTERFACE)

target_include_directories(
  "${PROJECT_NAME}_interface"
  INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)

target_sources(
  "${PROJECT_NAME}_interface"
  INTERFACE
    "${PROJECT_SOURCE_DIR}/src/bf_c_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bf_freelist_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bf_imemory_manager.cpp"
    "${PROJECT_SOURCE_DIR}/src/bf_linear_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bf_memory_utils.c"
    "${PROJECT_SOURCE_DIR}/src/bf_pool_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bf_proxy_allocator.cpp"
    "${PROJECT_SOURCE_DIR}/src/bf_stack_allocator.cpp"
)

# set_target_properties(
#   "${PROJECT_NAME}_interface" 
#   PROPERTIES
#     PUBLIC_HEADER include/bifrost/memory/bifrost_imemory_manager.hpp
# )

target_compile_features(
  "${PROJECT_NAME}_interface"
  INTERFACE
    cxx_std_14
)

# add_library("${PROJECT_NAME}_interface" ALIAS BifrostMemory)

cmake_policy(POP)
