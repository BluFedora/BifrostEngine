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