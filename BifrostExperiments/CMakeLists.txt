################################################################################
#                                                                              #
#                         BIFROST EXPERIMENTS PROJECT                          #
#                                                                              #
################################################################################
###    CMakeList.txt : Bifrost Experiments Sub Project                       ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostExperiments VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/bin/_experiments)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

if (WIN32)
  set(CMAKE_CXX_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
  set(CMAKE_C_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
else()
  set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs -fno-rtti -Wall -g")
endif()

file(GLOB_RECURSE BIFROST_EXPERIMENTS_SOURCE_FILES
  "main.c"
  "src/bifrost_json.c"
)

include_directories(BifrostExperiments ${PROJECT_SOURCE_DIR}/include)

add_executable(BifrostExperiments ${BIFROST_EXPERIMENTS_SOURCE_FILES})



set_target_properties(BifrostExperiments PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
# set_target_properties(BifrostScript PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
