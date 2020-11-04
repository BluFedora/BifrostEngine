################################################################################
#                                                                              #
#                   BF Dynamic Polymorphics PROJECT                            #
#                                                                              #
################################################################################
###  CMakeList.txt :                                                         ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BF_Poly VERSION 0.0.1 DESCRIPTION "" LANGUAGES CXX)

# Libraries

set(CMAKE_CXX_FLAGS "/wd4996 /Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812 /permissive- /vmg")
  set(CMAKE_C_FLAGS "/wd4996 /Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812 /permissive- /vmg")

add_library("${PROJECT_NAME}_interface" INTERFACE)

target_include_directories(
  "${PROJECT_NAME}_interface"
  INTERFACE
    "${PROJECT_SOURCE_DIR}/include"
)

# Tests

add_executable(
  "${PROJECT_NAME}_test01"

  "${PROJECT_SOURCE_DIR}/tests/poly_main.cpp"
)

target_link_libraries(
  "${PROJECT_NAME}_test01"

  PRIVATE
    "${PROJECT_NAME}_interface" 
)

set_target_properties(
  "${PROJECT_NAME}_test01"
  PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
    CXX_STANDARD             17
    CXX_STANDARD_REQUIRED    YES
    CXX_EXTENSIONS           NO
)
