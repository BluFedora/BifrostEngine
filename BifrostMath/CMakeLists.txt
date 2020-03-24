################################################################################
#                                                                              #
#                            BIFROST MATH PROJECT                              #
#                                                                              #
################################################################################
###    CMakeList.txt : Bifrost Math Sub Project of the Bifrost Engine        ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostMath_static VERSION 1.0.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories("${PROJECT_BINARY_DIR}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
  set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
  set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win32)
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

  # GCC / Clang
  #   "/GR-"                      - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "/D_***_SECURE_NO_WARNINGS" - Not needed since GCC / Clang isn't stupid like MSVC.
  #   "-std=c++17"                = "/std:c++17"
  #   "-fno-rtti"                 = "/GR-"
  #   TODO(Shareef): /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs -fno-rtti -Wall -g")
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
  set(CMAKE_C_FLAGS "/Zc:__cplusplus /GR- /D_SCL_SECURE_NO_WARNINGS /D_CRT_SECURE_NO_WARNINGS /std:c++17 /W3 /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj /wd26812")
endif()

file(GLOB_RECURSE BIFROST_MATH_SOURCE_FILES
  "src/bifrost_camera.c"
  "src/bifrost_mat4x4.c"
  "src/bifrost_transform.c"
  "src/bifrost_vec2.c"
  "src/bifrost_vec3.c"
)

include_directories(BifrostMath_static ${PROJECT_SOURCE_DIR}/../include/bifrost)
include_directories(BifrostMath_static ${PROJECT_SOURCE_DIR}/lib/include)
include_directories(BifrostMath_static ${PROJECT_SOURCE_DIR}/include)

add_library(
  BifrostMath_static STATIC
  ${BIFROST_MATH_SOURCE_FILES}
)

target_include_directories(
  BifrostMath_static PUBLIC
  ${BIFROST_MATH_SOURCE_FILES}/include
)

target_compile_definitions(BifrostMath_static PRIVATE BIFROST_MATH_EXPORT_STATIC)

set_target_properties(BifrostMath_static PROPERTIES OUTPUT_NAME BifrostMath_static)
set_target_properties(BifrostMath_static PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(BifrostMath_static PROPERTIES PUBLIC_HEADER include/bifrost/bifrost_math.h)
set_target_properties(BifrostMath_static PROPERTIES CXX_STANDARD 17)
set_target_properties(BifrostMath_static PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
# set_target_properties(BifrostScript PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

# Shared Library Setttings

project(BifrostMath_shared)

add_library(
  BifrostMath_shared SHARED
  ${BIFROST_MATH_SOURCE_FILES}
)

target_include_directories(BifrostMath_shared
    INTERFACE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(BifrostMath_shared PRIVATE BIFROST_MATH_EXPORT)
set_target_properties(BifrostMath_shared PROPERTIES PUBLIC_HEADER include/bifrost/bifrost_math.h)

set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})
