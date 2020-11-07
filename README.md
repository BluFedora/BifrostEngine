# BF Engine by Shareef Raheem

## Description

This is my hobbyist game engine that I hack on in my free time to learn new concepts.

The main project is written in C++17 but the various Engine Submodules may be only need C99, C11, or C++11 compliance.

## Getting Started

Most of the Engine is separated into separate libraries that must be compiled and linked to be used.
The base Module that you should start with [Platform](Engine/Platform/README.md) for basic windowing and other low level platform specifics.

## Build System

- CMake v3.12
- MSVC (VS2019) On Windows and Clang on MacOS

## External Libraries Used

- Vulkan SDK
  - Lunar G for Windows
  - Molten VK For MacOS
- FMOD
- Glad
- GLFW
- GLM
- Glslang
- Dear ImGui
- Lua
- Native File Dialog
- SDL
- SOL2
- stb_image.h
- stb_truetype.h
- D3D12 Extension Library (d3dx12.h)
