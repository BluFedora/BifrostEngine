add_executable(
  BasicWin32Window WIN32 

  "basic_win32_window.cpp"
)

target_link_libraries(
  BasicWin32Window
  PUBLIC
    BF_Core
)