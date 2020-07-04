################################################################################
#                                                                              #
#                           BIFROST SCRIPT PROJECT                             #
#                                                                              #
################################################################################
###  CMakeList.txt : Bifrost Script Sub Project of the Bifrost Engine        ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostScript VERSION 0.1.3 DESCRIPTION "This is a toy compiler and virtual machine for my simple scripting langauge." LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # GCC / Clang
  #   "/GR-"                      - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "/D_***_SECURE_NO_WARNINGS" - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "-std=c++17"                = "/std:c++17"
  #   "-fno-rtti"                 = "/GR-"
  #   TODO(Shareef): /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "-std=c++17 -fno-rtti -Wall -g -pedantic")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  # using Intel C++
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  # NOTE(Shareef): Setting the compiler flags we will be using.
  #    
  #   "/Zc:__cplusplus" - Report the correct '__cplusplus' macro on msvc.
  #   "/WX"   - treat warnings as errors.
  #   "/EHsc" - is for enabling C++ exceptions which we will NOT be using.
  #     Reason being the C++ STL uses exceptions like a mother f*cker and with warnings as
  #   errors on this would not build other wise.
  #   (alternative is to define: _HAS_EXCEPTIONS but that has some other implications.)
  #   "/GR-"  - Disables RTTI, trying to see if we can do without it for this engine.
  #   "/D_***_SECURE_NO_WARNINGS" - Disable the warnings for not using the "secure"
  #      versions of certain functions.
  #   "/std:c++17" - I needed some new C++17 features for some tEmPlAtE mAgIc
  #   "/GF"        - String Pooling is very important for the way the Material System works. I want stable
  #                  addresses. ("Eliminate Duplicate Strings")
  #   "/MP[N]"     - N is optional, will just use all the cores if omitted. Allows for
  #                  compilation on mutiple cores. Should give us a big speed-up on compile times.
  #
  set(CMAKE_CXX_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
  set(CMAKE_C_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /permissive- /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
endif()

include_directories("${PROJECT_BINARY_DIR}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
  set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
  set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win32)
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

file(GLOB_RECURSE BIFROST_SCRIPT_SOURCE_FILES
  "src/bifrost_vm_build.c"
)

include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/../include/bifrost)
include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/lib/include)
include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/include)

add_library(
  BifrostScript STATIC
  ${BIFROST_SCRIPT_SOURCE_FILES}
  "../src/bifrost/data_structures/bifrost_array_t.c"
  "../src/bifrost/data_structures/bifrost_dynamic_string.c"
  "../src/bifrost/data_structures/bifrost_hash_map.c"
)

target_include_directories(
  BifrostScript 
  PUBLIC ${PROJECT_SOURCE_DIR}/include
)

set_property(
  TARGET BifrostScript 
  PROPERTY C_STANDARD 99
)

target_compile_definitions(BifrostScript PRIVATE BIFROST_VM_EXPORT_STATIC)

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
    "../src/bifrost/data_structures/bifrost_array_t.c"
    "../src/bifrost/data_structures/bifrost_dynamic_string.c"
    "../src/bifrost/data_structures/bifrost_hash_map.c"
  )
  
  target_include_directories(BifrostScript_shared
      INTERFACE
      ${PROJECT_SOURCE_DIR}/include
  )
  
  target_compile_definitions(BifrostScript_shared PRIVATE BIFROST_VM_EXPORT)
  set_target_properties(BifrostScript_shared PROPERTIES PUBLIC_HEADER include/bifrost/script/bifrost_vm.h)
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
)

set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

target_compile_definitions(BifrostScript_cli PRIVATE BIFROST_VM_EXPORT_STATIC)

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

  "${CMAKE_SOURCE_DIR}/src/bifrost/utility/bifrost_hash.cpp"

  "${CMAKE_SOURCE_DIR}/src/bifrost_imgui_glfw.cpp"

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
      BifrostPlatform_static
      BifrostMemory
      BifrostMath_static
      BifrostGraphics_static
  )
else()
target_link_libraries(
  BifrostWeb 
  PRIVATE 
    BifrostScript
    BifrostPlatform_shared
    BifrostMemory
    BifrostMath_shared
    BifrostGraphics_shared
)
endif()

if(EMSCRIPTEN)
  #set(CMAKE_EXECUTABLE_SUFFIX ".html")
  set(CMAKE_CXX_FLAGS "--preload-file assets -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_SDL=2 -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR -s WASM=0 -s ASSERTIONS=2 --js-library ../../../lib/bifrost_webgl.js")

  file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/../)
endif()
