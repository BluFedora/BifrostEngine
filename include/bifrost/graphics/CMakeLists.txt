project(RenderGraph)

# IF you compile with g++ on windows with a single header file (with no c/cpp file) you will get an error sayin incpompatible vertion.
# g++ -g -std=c++17 bifrost_gfx_render_graph.cpp -o RenderGraphTest
# cl.exe /std:c++17 /EHsc /Zi /DEBUG:FULL /Fe:RenderGraphTest_MSVC.exe bifrost_gfx_render_graph.cpp
add_executable(${PROJECT_NAME} bifrost_gfx_render_graph.cpp "bifrost_debug_renderer.hpp")
