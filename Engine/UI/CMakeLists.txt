#
# BluFedora UI Framework
#

cmake_minimum_required(VERSION 3.8)

project(BF_UI VERSION 1.0.0 DESCRIPTION "Immediate mode UI framework for the engine." LANGUAGES CXX)

set(
  BF_UI_INCLUDE_DIRS
  
  "${PROJECT_SOURCE_DIR}/include/"

  # TODO(SR): These are hacks.
  "${CMAKE_SOURCE_DIR}/Engine/Runtime/include/"
  "${CMAKE_SOURCE_DIR}/Engine/Graphics2D/include/"
  "${CMAKE_SOURCE_DIR}/Runtime/include/"
)

set(
  BF_UI_SOURCES
  
  # API Headers
  "${PROJECT_SOURCE_DIR}/include/bf/bf_ui.hpp"

  # Sources
  "${PROJECT_SOURCE_DIR}/src/bf_ui.cpp"

  # Hacks
  "${CMAKE_SOURCE_DIR}/Engine/Graphics2D/src/bf_painter.cpp" 
)

set(
  BF_UI_LIBRARIES
  
  BF_Graphics_shared
  BF_Math_shared
  BF_Memory_interface
  BF_Platform_shared
  BF_TMPUtils
  BF_Text_static

  BF_DataStructuresCxx
)

add_library(
  BF_UI_shared
  "${BF_UI_SOURCES}"
)

target_include_directories(
  BF_UI_shared

  PUBLIC
    "${BF_UI_INCLUDE_DIRS}"
)

target_link_libraries(
  BF_UI_shared

  PUBLIC
    "${BF_UI_LIBRARIES}"
)
