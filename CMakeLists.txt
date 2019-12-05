################################################################################
#                                                                              #
#                            BIFROST CMAKE PROJECT                             #
#                                                                              #
################################################################################
###  CMakeList.txt : Top-level CMake project file, for global configuration  ###
################################################################################

  # NOTE(Shareef): This sets the minimum version of CMake that must be used
cmake_minimum_required(VERSION 3.8)

  # NOTE(Shareef): This sets project's name
project(BifrostEngine)

  # NOTE(Shareef): Bifrost Engine Version
set(BIFROST_ENGINE_VERSION_MAJOR 1)
set(BIFROST_ENGINE_VERSION_MINOR 0)
set(BIFROST_ENGINE_VERSION_PATCH 0)

  # This will generate a header file with the version info.
  # The file will be made in the build directory hence why the build
  # dir needs to be added to the includes.
configure_file(
  "${PROJECT_SOURCE_DIR}/docs/bifrost_version.h.in"
  "${PROJECT_BINARY_DIR}/bifrost/bifrost_version.h"
)
include_directories("${PROJECT_BINARY_DIR}")

message(STATUS "Generated headers located at ${PROJECT_BINARY_DIR}")

set(CMAKE_CXX_STANDARD 17)

# set(CMAKE_GENERATOR_PLATFORM x64)

  # NOTE(Shareef): Sets the output directory to be in a reasonable place.
set(CMAKE_BINARY_DIR       ${CMAKE_SOURCE_DIR}/bin/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR})

message(STATUS "Working Dir ${CMAKE_BINARY_DIR}../")

# NOTE(Shareef): Print out some CMake Info.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
		message(STATUS "We are building a DEBUG 64bit build...")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
		message(STATUS "We are building a DEBUG 32bit build...")
	endif()
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
	  message(STATUS "We are building a RELEASE 64bit build...")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
	 message(STATUS "We are building a RELEASE build...")
  endif()
endif()

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
  #   "/GR-"                      - Not needed since GCC / Clang isn't retarded like MSVC.
  #   "/D_***_SECURE_NO_WARNINGS" - Not needed since GCC / Clang isn't retarded like MSVC.
  #   "-std=c++17"                = /std:c++17
  #   TODO(Shareef): /WX /EHsc /GF /MP /fp:fast /Zm2000 /bigobj
  set(CMAKE_CXX_FLAGS "-std=c++17 -lstdc++fs -Wall -g")
endif()



  # NOTE(Shareef): Automatically generate a list of ALL source (C/++) files based
  #   of off what is in the folders.
file(GLOB_RECURSE BIFROST_ENGINE_SOURCE_FILES

  # "engine/animation/*.hpp"
  # "engine/animation/*.cpp"
  # "engine/core/*.hpp"
  # "engine/core/*.cpp"
  # "engine/memory/*.hpp"
  # "engine/memory/*.cpp"
  # "engine/*.hpp"
  # "engine/*.cpp"

  # Libs
  "lib/include/glad/glad.c"

  # Bifrost Datastructures Module
  "src/bifrost/data_structures/bifrost_array_t.c"
  "src/bifrost/data_structures/bifrost_dynamic_string.c"
  "src/bifrost/data_structures/bifrost_hash_map.c"

  # Bifrost Debug Module
  "src/bifrost/debug/bifrost_dbg_logger.c"

  # Bifrost Graphics Module
  "src/bifrost/graphics/bifrost_gfx_common.c"
  ## OpenGL Backend
  # "src/bifrost/graphics/bifrost_gfx_opengl.cpp"
  ## Vulkan Backend
  "src/bifrost/graphics/vulkan/bifrost_vulkan_command_list.cpp"
  "src/bifrost/graphics/vulkan/bifrost_vulkan_conversions.c"
  "src/bifrost/graphics/vulkan/bifrost_vulkan_material_pool.cpp"
  "src/bifrost/graphics/vulkan/bifrost_vulkan_mem_allocator.cpp"
  "src/bifrost/graphics/vulkan/bifrost_vulkan_renderpass.cpp"
  "src/bifrost/graphics/bifrost_gfx_vulkan.cpp"

  # Bfrost Script Module
  "src/bifrost/script/bifrost_vm_build.c"

  # Bifrost Math Module
  "src/bifrost/math/bifrost_camera.c"
  "src/bifrost/math/bifrost_mat4x4.c"
  "src/bifrost/math/bifrost_transform.c"
  "src/bifrost/math/bifrost_vec2.c"
  "src/bifrost/math/bifrost_vec3.c"

  # Bifrost Utility Module
  "src/bifrost/utility/bifrost_hash.cpp"

  "main.cpp"
 )
# set(GAME200_SOURCES_FILES game/main.cpp lib/include/glad/glad.c)

# add_subdirectory("engine")
# add_subdirectory("${GAME200_PROJECT_LIB_DIR}/include/glm")
# add_subdirectory("unit_tests")
# add_subdirectory("game")

# NOTE(Shareef):
#   Add the libraries we are linking against to the target.
find_package(OpenGL REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenCL)

  # NOTE(Shareef): This makes is you can #include engine / game / library code without doing the "../" thing.
include_directories(BifrostEngine ${PROJECT_SOURCE_DIR})
include_directories(BifrostEngine ${PROJECT_SOURCE_DIR}/lib/include)
include_directories(BifrostEngine ${PROJECT_SOURCE_DIR}/include)
include_directories(BifrostEngine C:/Users/rshar/Documents/bifrost-core/bifrost_core)
include_directories(BifrostEngine ${Vulkan_INCLUDE_DIR})

add_executable(BifrostEngine ${BIFROST_ENGINE_SOURCE_FILES})

# NOTE(Shareef): Allows Visual Studio to detect the use of C++17 while doing static analysis.
set_target_properties(BifrostEngine PROPERTIES CXX_STANDARD 17)

  # NOTE(Shareef): Sets the working directory of the exe to be at the exe
  #   allowing us to load stuff from the assets folder.
set_target_properties(BifrostEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}../)
  # NOTE(Shareef): Make the console not pop up.
# set_target_properties(BifrostEngine PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")

## Uncomment to inject these options into the command line when preforming a code analysis, it just works
## add_compile_definitions( "dummyDefine=0 /DWIN32 /D_CRT_SECURE_NO_DEPRECATE /D_CRT_NONSTDC_NO_DEPRECATE /D_WINDOWS /GR /EHsc /std:c++17 /Zc:__cplusplus /DDummy2")

if (WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/lib/win64")
		set(BIFROST_ENGINE_GLFW_LIB "${BIFROST_ENGINE_LIB_DIR}/glfw3dll.lib")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/lib/win32")
		set(BIFROST_ENGINE_GLFW_LIB "${BIFROST_ENGINE_LIB_DIR}/glfw3dll.lib")
		set(BIFROST_ENGINE_FMOD_LIB "${BIFROST_ENGINE_LIB_DIR}/fmod_vc.lib")
	endif()
	set(BIFROST_ENGINE_LUA_LIB "${BIFROST_ENGINE_LIB_DIR}/lua53.lib")
elseif (APPLE)
	set(BIFROST_ENGINE_LIB_DIR  "${PROJECT_SOURCE_DIR}/lib/macOS")
	set(BIFROST_ENGINE_GLFW_LIB "${PROJECT_SOURCE_DIR}/lib/macOS/libglfw.3.3.dylib")
endif()

target_link_libraries(BifrostEngine "${BIFROST_ENGINE_GLFW_LIB}")

if (WIN32)
  target_link_libraries(BifrostEngine "opengl32.lib")
  target_link_libraries(BifrostEngine "D:/Vulkan/1.1.106.0/Lib32/vulkan-1.lib")
elseif (APPLE)
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL -framework IOKit")
  # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_MACOS}/libfmod.dylib")
  # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_MACOS}/libglfw.3.3.dylib")
  # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_MACOS}/liblua.a")
endif()

target_link_libraries(BifrostEngine ${Vulkan_LIBRARIES})

if(OpenCL_FOUND)
	include_directories(${OPENCL_INCLUDE_DIR})
	target_link_libraries(BifrostEngine OpenCL::OpenCL)
	#target_compile_definitions(my_target PRIVATE FOO=1 BAR=1)
	add_compile_definitions(BIFROST_COMPUTE_OPENCL=1)
else()
	add_compile_definitions(BIFROST_COMPUTE_OPENCL=0)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
		# target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/lua53.lib")
		# target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/fmodL64_vc.lib")
		# target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/glfw3dll.lib")
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
		# message(STATUS "We are building a DEBUG 32bit build...")
		# target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/lua53.lib")
		# target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/fmodL_vc.lib")
		# target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/glfw3dll.lib")
	endif()
else()
  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
	  # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/lua53.lib")
	  # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/fmod64_vc.lib")
	  # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN64}/glfw3dll.lib")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
	 # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/lua53.lib")
	 # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/fmod_vc.lib")
	 # target_link_libraries(Project_Gemini "${GAME200_PROJECT_LIB_DIR_WIN32}/glfw3.lib")
  endif()
endif()

  # NOTE(Shareef): Automatically copy the needed dlls to the output directory

if (WIN32)
	configure_file("${BIFROST_ENGINE_LIB_DIR}/glfw3.dll"   "${EXECUTABLE_OUTPUT_PATH}glfw3.dll" COPYONLY)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN64}/fmodL64.dll" "${EXECUTABLE_OUTPUT_PATH}fmodL64.dll" COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN64}/fmod64.dll"  "${EXECUTABLE_OUTPUT_PATH}fmod64.dll"  COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN64}/glfw3.dll"   "${EXECUTABLE_OUTPUT_PATH}glfw3.dll" COPYONLY)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32 bits
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN32}/fmodL.dll" "${EXECUTABLE_OUTPUT_PATH}fmodL.dll" COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN32}/fmod.dll"  "${EXECUTABLE_OUTPUT_PATH}fmod.dll"  COPYONLY)
  # configure_file("${GAME200_PROJECT_LIB_DIR_WIN32}/glfw3.dll" "${EXECUTABLE_OUTPUT_PATH}glfw3.dll" COPYONLY)
endif()

add_subdirectory(include/bifrost/graphics/)
