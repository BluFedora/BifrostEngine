################################################################################
###  CMakeList.txt : Tool for batch anylyzing of Clang -ftime-trace files.   ###
################################################################################

cmake_minimum_required(VERSION 3.8)

project(BuildTimeAnalyzerGUI VERSION 0.0.1 DESCRIPTION "Tool for batch anylyzing of Clang -ftime-trace files.")

include_directories(BuildTimeAnalyzerGUI ${PROJECT_SOURCE_DIR}/lib/include ${CMAKE_SOURCE_DIR}/../include ${CMAKE_SOURCE_DIR}/BifrostMath/include ${CMAKE_SOURCE_DIR}/BifrostScript/include)

add_executable(
  BuildTimeAnalyzerGUI
  "src/bta_main.cpp"

  "${CMAKE_SOURCE_DIR}/Runtime/src/utility/bifrost_json.cpp"

  "${CMAKE_SOURCE_DIR}/Runtime/src/bifrost_imgui_glfw.cpp"

 "src/bta_dialog_win32.cpp"

 "${CMAKE_SOURCE_DIR}/Runtime/src/utility/bifrost_json.c"

 "lib/include/implot/implot.cpp"
 "lib/include/implot/implot_demo.cpp"
)

 target_include_directories(
  BuildTimeAnalyzerGUI
  PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/Experiments/include
    ${CMAKE_SOURCE_DIR}/Runtime/include
    ${CMAKE_SOURCE_DIR}/lib/include
)

target_compile_definitions(
  BuildTimeAnalyzerGUI
  PUBLIC
    VK_USE_PLATFORM_WIN32_KHR
    BIFROST_GRAPHICS_NO_LOG
    BIFROST_GRAPHICS_POWER_SAVER
)

target_link_libraries(
  BuildTimeAnalyzerGUI
  PUBLIC
    BF_Math_shared
    BF_Memory_interface
    BF_Platform_static
    BF_Graphics_static
    BF_DataStructuresC_static
)

# Vulkan Library Handling

find_package(Vulkan)

if(${Vulkan_FOUND})
  include_directories(
    BuildTimeAnalyzerGUI 
    ${Vulkan_INCLUDE_DIR}
  )

  target_link_libraries(
    BuildTimeAnalyzerGUI 
    PRIVATE 
      ${Vulkan_LIBRARIES}
  )
endif()

set_target_properties(
  BuildTimeAnalyzerGUI
  PROPERTIES
  # ARCHIVE_OUTPUT_DIRECTORY       "${CMAKE_SOURCE_DIR}/archive"
  # LIBRARY_OUTPUT_DIRECTORY       "${CMAKE_SOURCE_DIR}/library"
  RUNTIME_OUTPUT_DIRECTORY      "${CMAKE_SOURCE_DIR}/bin"
  CXX_STANDARD                  17
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  LINK_FLAGS                    "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS"
)
