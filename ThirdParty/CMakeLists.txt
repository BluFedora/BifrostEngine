# Dear ImGUI

add_library(
  DearImGUI STATIC

  "imgui/imconfig.h"
  "imgui/imgui.h"
  "imgui/imgui_internal.h"

  "imgui/imgui.cpp"
  "imgui/imgui_demo.cpp"
  "imgui/imgui_draw.cpp"
  "imgui/imgui_tables.cpp"
  "imgui/imgui_widgets.cpp"
)

target_include_directories(
  DearImGUI
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}"
)

# SPIRV-Cross

add_subdirectory("SPIRV-Cross")

# SPIRV-Reflect

add_library(
  spirv-reflect STATIC

  "SPIRV-Reflect/spirv_reflect.h"
  "SPIRV-Reflect/spirv_reflect.c"
)

target_include_directories(
  spirv-reflect

  PRIVATE
    "SPIRV-Reflect/include"

  PUBLIC
    "SPIRV-Reflect"
)
