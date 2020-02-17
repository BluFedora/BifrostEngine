################################################################################
#                                                                              #
#                           BIFROST SCRIPT PROJECT                             #
#                                                                              #
################################################################################
###  CMakeList.txt : Bifrost Script Sub Project of the Bifrost Engine        ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostScript VERSION 0.1.3 DESCRIPTION "This is a toy compiler and virtual machine for my simple scripting langauge.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories("${PROJECT_BINARY_DIR}")

set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win64)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

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
if (WIN32)
  set(CMAKE_CXX_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
  set(CMAKE_C_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
else()
  # GCC / Clang
  #   "/GR-"                      - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "/D_***_SECURE_NO_WARNINGS" - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "-std=c++17"                = "/std:c++17"
  #   "-fno-rtti"                 = "/GR-"
  #   TODO(Shareef): /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj
  set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs -fno-rtti -Wall -g")
endif()

file(GLOB_RECURSE BIFROST_SCRIPT_SOURCE_FILES
  "src/bifrost_vm_build.c"
)

include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/../include/bifrost)
include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/lib/include)
include_directories(BifrostScript ${PROJECT_SOURCE_DIR}/include)

add_library(
  BifrostScript STATIC
  ${BIFROST_SCRIPT_SOURCE_FILES}
)

target_include_directories(
  BifrostScript PUBLIC
  ${PROJECT_SOURCE_DIR}/include
)

set_target_properties(BifrostScript PROPERTIES OUTPUT_NAME BifrostScript_static)
set_target_properties(BifrostScript PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(BifrostScript PROPERTIES PUBLIC_HEADER include/bifrost/script/bifrost_vm.h)
set_target_properties(BifrostScript PROPERTIES CXX_STANDARD 17)
set_target_properties(BifrostScript PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
# set_target_properties(BifrostScript PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

# Shared Library Setttings

# project(BifrostScript_shared VERSION 0.1.3 DESCRIPTION "This is a toy compiler and virtual machine for my simple scripting langauge.")

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
