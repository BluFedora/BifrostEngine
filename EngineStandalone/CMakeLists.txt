################################################################################
#                                                                              #
#                           BIFROST ENGINE PROJECT                             #
#                                                                              #
################################################################################
###  CMakeList.txt : Lightweight Runtime version of the engine for release.  ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BifrostRuntime VERSION 0.1.3 DESCRIPTION "Lightweight Runtime version of the engine for game release.")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

include_directories("${PROJECT_BINARY_DIR}")

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
  # set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
  # set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/lib/win32)
endif()

file(GLOB_RECURSE ENGINE_SOURCE_FILES
  "src/engine_runtime.cpp"
)

include_directories(BifrostRuntime ${PROJECT_SOURCE_DIR}/../include/bifrost)
include_directories(BifrostRuntime ${PROJECT_SOURCE_DIR}/lib/include)
include_directories(BifrostRuntime ${PROJECT_SOURCE_DIR}/include)

add_executable(
  BifrostRuntime
  ${ENGINE_SOURCE_FILES}
)

target_link_libraries(
  BifrostRuntime
  PRIVATE
  "${PROJECT_SOURCE_DIR}/../lib/win64/SDL2.lib"
  "${PROJECT_SOURCE_DIR}/../lib/win64/SDL2main.lib"
)

target_link_libraries(BifrostRuntime PRIVATE "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")

# install(TARGETS BifrostRuntime DESTINATION bin/runtime)

set_target_properties(
  BifrostRuntime
  PROPERTIES
  # ARCHIVE_OUTPUT_DIRECTORY       "${CMAKE_SOURCE_DIR}/archive"
  # LIBRARY_OUTPUT_DIRECTORY       "${CMAKE_SOURCE_DIR}/library"
  RUNTIME_OUTPUT_DIRECTORY       "${CMAKE_SOURCE_DIR}/bin"
  CXX_STANDARD                   17
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# set_target_properties(BifrostRuntime PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

if (WIN32)
  configure_file("${PROJECT_SOURCE_DIR}/../lib/win64/SDL2.dll" "${CMAKE_SOURCE_DIR}/bin/SDL2.dll" COPYONLY)
  # configure_file("${BIFROST_ENGINE_LIB_DIR}/BifrostScript_shared.dll" "${EXECUTABLE_OUTPUT_PATH}BifrostScript_shared.dll" COPYONLY)
  # configure_file("${BIFROST_ENGINE_LIB_DIR}/BifrostMath_shared.dll"   "${EXECUTABLE_OUTPUT_PATH}BifrostMath_shared.dll"   COPYONLY)
endif()
