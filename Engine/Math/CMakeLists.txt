################################################################################
#                                                                              #
#                              BF MATH PROJECT                                 #
#                                                                              #
################################################################################
###    CMakeList.txt : Math Sub Project.                                     ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BF_Math VERSION 1.0.0)

set(
  BF_MATH_SOURCE_FILES
  
  "${PROJECT_SOURCE_DIR}/src/bifrost_camera.c"
  "${PROJECT_SOURCE_DIR}/src/bifrost_mat4x4.c"
  "${PROJECT_SOURCE_DIR}/src/bifrost_transform.c"
  "${PROJECT_SOURCE_DIR}/src/bifrost_vec2.c"
  "${PROJECT_SOURCE_DIR}/src/bifrost_vec3.c"
)

add_library(
  BF_Math_static 
  STATIC
    ${BF_MATH_SOURCE_FILES}
)

target_include_directories(
  BF_Math_static 
  PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)


target_compile_definitions(BF_Math_static PRIVATE BIFROST_MATH_EXPORT_STATIC)

set_target_properties(BF_Math_static PROPERTIES OUTPUT_NAME bf.MathLib)
set_target_properties(BF_Math_static PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(BF_Math_static PROPERTIES PUBLIC_HEADER include/bifrost/bifrost_math.h)
set_target_properties(BF_Math_static PROPERTIES CXX_STANDARD 17)
set_target_properties(BF_Math_static PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Shared Library Setttings

if(NOT EMSCRIPTEN)
  add_library(
    BF_Math_shared 
    SHARED
      ${BF_MATH_SOURCE_FILES}
  )

  target_include_directories(
    BF_Math_shared
    PUBLIC
      ${PROJECT_SOURCE_DIR}/include
  )

  target_compile_definitions(BF_Math_shared PRIVATE BIFROST_MATH_EXPORT)
  set_target_properties(
    BF_Math_shared 
    PROPERTIES 
    PUBLIC_HEADER include/bifrost/bifrost_math.h
    OUTPUT_NAME   bf.MathDLL
    )

  set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/bin)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
  set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})
endif()
