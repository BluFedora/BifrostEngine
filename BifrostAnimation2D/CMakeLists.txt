################################################################################
#                                                                              #
#                     BIFROST SPRITE-ANIM RUNTIME PROJECT                      #
#                                                                              #
################################################################################
###  CMakeList.txt :                                                         ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostAnimation2D VERSION 0.0.1 DESCRIPTION "" LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(
  BifrostAnimation2D_static STATIC
  "src/bifrost_sprite_animation.c"
  "include/bifrost_animation2D_export.h"
)

# include(GenerateExportHeader)

#generate_export_header(
#  BifrostAnimation2D_static
#  EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/bifrost_animation2D_export.h"
#  EXPORT_MACRO_NAME BIFROST_SPRITE_ANIM_API
#)

target_include_directories(
  BifrostAnimation2D_static 
  PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/BifrostRuntime/include
    ${CMAKE_BINARY_DIR}
)

set_property(
  TARGET BifrostAnimation2D_static 
  PROPERTY 
    C_STANDARD 99
)

# target_compile_definitions(BifrostAnimation2D_static PRIVATE BIFROST_VM_EXPORT_STATIC)

set_target_properties(BifrostAnimation2D_static PROPERTIES OUTPUT_NAME BifrostAnimation2D_static)
set_target_properties(BifrostAnimation2D_static PROPERTIES VERSION ${PROJECT_VERSION})
# set_target_properties(BifrostAnimation2D_static PROPERTIES PUBLIC_HEADER include/bifrost/script/bifrost_vm.h)
set_target_properties(BifrostAnimation2D_static PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
