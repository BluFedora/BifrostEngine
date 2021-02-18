################################################################################
#                                                                              #
#                           ANIM2D RUNTIME PROJECT                             #
#                                                                              #
################################################################################
###  CMakeList.txt : Animation runtime for the SRSA tool.                    ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BF_Animation2D VERSION 0.0.1 DESCRIPTION "" LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# set(CMAKE_DEBUG_POSTFIX d)

add_library(
  bfAnimation2D_shared SHARED
  "${PROJECT_SOURCE_DIR}/include/bf/anim2D/bf_anim2D_api.h"
  "${PROJECT_SOURCE_DIR}/include/bf/anim2D/bf_anim2D_export.h"
  "${PROJECT_SOURCE_DIR}/include/bf/anim2D/bf_anim2D_network.h"

  "${PROJECT_SOURCE_DIR}/src/bf_sprite_animation.cpp"
)

target_include_directories(
  bfAnimation2D_shared 
  PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/BifrostRuntime/include
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(
  bfAnimation2D_shared
  PRIVATE
    BF_DataStructuresCxx
    BF_Networking_static
)

set_property(
  TARGET bfAnimation2D_shared 
  PROPERTY 
    C_STANDARD 99
)

target_compile_definitions(
  bfAnimation2D_shared 
  PRIVATE 
    BF_ANIM2D_EXPORT
)

set_target_properties(bfAnimation2D_shared PROPERTIES OUTPUT_NAME bf.Animation2DDLL)
set_target_properties(bfAnimation2D_shared PROPERTIES VERSION ${PROJECT_VERSION})

# include(GenerateExportHeader)

#generate_export_header(
#  bfAnimation2D_shared
#  EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/bifrost_animation2D_export.h"
#  EXPORT_MACRO_NAME BIFROST_SPRITE_ANIM_API
#)

# if(CMAKE_SIZEOF_VOID_P EQUAL 4)
#     set(ARCH_POSTFIX "")
# else()
#     set(ARCH_POSTFIX 64)
# endif()
# 
# add_library(my_lib${ARCH_POSTFIX} [...])
# CMAKE_STATIC_LIBRARY_SUFFIX
