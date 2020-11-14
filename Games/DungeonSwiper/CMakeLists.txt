#
# Game Propject
#

cmake_minimum_required(VERSION 3.8)

project(DungeonSwiper)

add_executable(
  DungeonSwiper
  "${PROJECT_SOURCE_DIR}/src/ds_main.cpp"
)

target_link_libraries(
  DungeonSwiper

  PRIVATE
    BF_Graphics_static
    BF_Platform_static
    BifrostRuntime
)
