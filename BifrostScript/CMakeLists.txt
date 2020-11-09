################################################################################
#                                                                              #
#                           BIFROST SCRIPT PROJECT                             #
#                                                                              #
################################################################################
###  CMakeList.txt : Bifrost Script bytecode Virtual Machine project.        ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostScript VERSION 0.1.3 DESCRIPTION "This is a toy compiler and virtual machine for my simple scripting langauge." LANGUAGES C)

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
  set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
  set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win32)
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE BIFROST_SCRIPT_SOURCE_FILES
  "${PROJECT_SOURCE_DIR}/src/bifrost_vm_build.c"
  "${PROJECT_SOURCE_DIR}/src/bifrost_hash_map.c"
)

include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/../include/bifrost)
include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/lib/include)
include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/include)

add_library(
  BifrostScript STATIC
  ${BIFROST_SCRIPT_SOURCE_FILES}
)

target_link_libraries(
  BifrostScript

  PUBLIC
    BF_Core_static
)

target_include_directories(
  BifrostScript 
  PUBLIC 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/BifrostRuntime/include
)

set_property(
  TARGET BifrostScript 
  PROPERTY C_STANDARD 99
)

target_compile_definitions(BifrostScript PRIVATE BF_VM_EXPORT_STATIC)

set_target_properties(BifrostScript PROPERTIES OUTPUT_NAME BifrostScript_static)
set_target_properties(BifrostScript PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(BifrostScript PROPERTIES PUBLIC_HEADER include/bifrost/script/bifrost_vm.h)
# set_target_properties(BifrostScript PROPERTIES CXX_STANDARD 17)
set_target_properties(BifrostScript PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
# set_target_properties(BifrostScript PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

# Shared Library Setttings

if(NOT EMSCRIPTEN)
  project(BifrostScript_shared VERSION 0.1.3 DESCRIPTION "This is a toy compiler and virtual machine for my simple scripting langauge.")
  
  add_library(
    BifrostScript_shared SHARED
    ${BIFROST_SCRIPT_SOURCE_FILES}
  )
  
  target_include_directories(BifrostScript_shared
      INTERFACE
        ${PROJECT_SOURCE_DIR}/include
      PRIVATE
        ${CMAKE_SOURCE_DIR}/BifrostRuntime/include
  )

  target_link_libraries(
    BifrostScript_shared
    PRIVATE
      BF_DataStructuresC_static
      BF_Core_static
  )
  
  target_compile_definitions(BifrostScript_shared PRIVATE BF_VM_EXPORT)
  set_target_properties(
    BifrostScript_shared 
    PROPERTIES 
      PUBLIC_HEADER include/bifrost/script/bifrost_vm.h
      OUTPUT_NAME   bf.ScriptDLL)
endif()

# Command Line Interface

project(BifrostScript_cli DESCRIPTION "Command line interface for interacting with the vm.")

add_executable(
  BifrostScript_cli
  "vm_command_line.cpp"
)

target_link_libraries(
  BifrostScript_cli 
  PRIVATE 
    BifrostScript
    # BifrostDataStructuresCxx
)

set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

target_compile_definitions(BifrostScript_cli PRIVATE BF_VM_EXPORT_STATIC)

set_target_properties(
  BifrostScript_cli
  PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
)

# WebGL

project(BifrostWeb)

add_executable(
  BifrostWeb
  "webgl_testing.cpp"

  "${CMAKE_SOURCE_DIR}/Runtime/src/utility/bifrost_hash.cpp"
  "${CMAKE_SOURCE_DIR}/Runtime/src/bifrost_imgui_glfw.cpp"

  "${CMAKE_SOURCE_DIR}/lib/include/imgui/imgui.cpp"
  "${CMAKE_SOURCE_DIR}/lib/include/imgui/imgui_widgets.cpp"
  "${CMAKE_SOURCE_DIR}/lib/include/imgui/imgui_draw.cpp"
 )

target_include_directories(
  BifrostWeb
  PRIVATE 
    ${CMAKE_SYSTEM_INCLUDE_PATH}
)

set_target_properties(
  BifrostWeb
  PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  CXX_STANDARD_REQUIRED ON
)

if (EMSCRIPTEN)
  target_compile_definitions(
    BifrostWeb
    PRIVATE 
      __EMSCRIPTEN__=1
  )

  target_link_libraries(
    BifrostWeb 
    PRIVATE 
      BifrostScript
      BF_Platform_static
      BF_Memory_interface
      BifrostMath_static
      BifrostGraphics_static
  )
else()
target_link_libraries(
  BifrostWeb 
  PRIVATE 
    BifrostScript
    BF_Platform_shared
    BF_Memory_interface
    BifrostMath_shared
    BifrostGraphics_shared
)
endif()

if(EMSCRIPTEN)
  #set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(CMAKE_CXX_FLAGS "--preload-file assets -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_SDL=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR -s WASM=0 -s ASSERTIONS=2 --js-library ../../../lib/bifrost_webgl.js")

  file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../)
endif()
