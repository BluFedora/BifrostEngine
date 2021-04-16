# BluFedora Engine

## Description

This is my hobbyist game engine that I hack on in my free time to learn new concepts.

The main project is written in C++17 but the various Engine Submodules may be only need C99, C11, or C++11 compliance.

## Getting Started

Most of the Engine is separated into separate libraries that must be compiled and linked to be used.
The base Module that you should start with [Platform](https://github.com/BluFedora/BF-Platform) for basic windowing and other low level platform specifics.

## Project Layout

- **assets**:        Assets needed by the engine's runtime.
- **BifrostScript**:
- **cloc**:
- **docs**:
- [**Engine**](Engine): The source code for the main engine.
- **Examples**:                   Small Demo Programs using the engine.
- **SourceAssets**:               Contains the source asset files for some textures used by the engine / editor.
- **TestBFProject**:              Test project containing scenes that can be opened by the editor.
- **ThirdParty**:                 Contains the source code for thirdy party libraries used by the engine.
- **VsProject**:                  Contains some bat files showing how to create a visual studio project.

## Build System

- CMake v3.12
- MSVC (VS2019) On Windows and Clang on MacOS

## Build Instructions

- When you clone this repo you must pull in all the submodules.
  - `git submodule update --init --recursive`

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
