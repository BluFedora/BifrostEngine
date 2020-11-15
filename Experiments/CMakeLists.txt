################################################################################
#                                                                              #
#                         BIFROST EXPERIMENTS PROJECT                          #
#                                                                              #
################################################################################
###    CMakeList.txt : Bifrost Experiments Sub Project                       ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostExperiments VERSION 1.0.0)

file(GLOB_RECURSE BIFROST_EXPERIMENTS_SOURCE_FILES
  "main.c"
  "src/bifrost_json.c"
)

include_directories(BifrostExperiments ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/../BifrostRuntime/include)

add_executable(BifrostExperiments ${BIFROST_EXPERIMENTS_SOURCE_FILES})

set_target_properties(BifrostExperiments PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
# set_target_properties(BifrostScript PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
