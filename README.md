# Bifrost Engine

## Description

A toy game engine with a custom scripting language.
The main engine requires C++17 but there are sub projects which are C99, C11, or C++11 compliant.

## Getting Started

Most of the Engine is separated into separate libraries that must be compiled and linked to be used.
The base Module that you should start with [BifrostPlatform](#Platform) for basic windowing and other low level platform specifics.

```cpp
#include <bifrost/platform/bifrost_platform.h> /* Platform API */

#include <cstdio> /* printf */

int main(int argc, char* argv[])
{
  bfPlatformAllocator allocator       = nullptr; // Use the default malloc and free, will be assigned to 'bfPlatformDefaultAllocator'.
  void*               global_userdata = nullptr; // Could be anything you want globally accessible.

  if (!bfPlatformInit({argc, argv, allocator, global_userdata}))
  {
    std::printf("Failed to initialize the platform.\n");
    return 1;
  }

  BifrostWindow* const main_window = bfPlatformCreateWindow("Title", 1280, 720, BIFROST_WINDOW_FLAGS_DEFAULT);

  if (!main_window)
  {
    std::printf("Failed to create the window.\n");
    bfPlatformQuit();
    return 2;
  }

  // Initialization Here

  main_window->user_data     = /* some user data here        */;
  main_window->renderer_data = /* another slot for user data */;

  main_window->event_fn = [](BifrostWindow* window, bfEvent* evt) {
    // Handle Events Here
  };

  main_window->frame_fn = [](BifrostWindow* window) {
    // Handle Drawing Here
  };

  // Main Loop

  bfPlatformDoMainLoop(main_window);

  //                      or...
  // You can also handle the main loop yourself but that will
  // be less cross-platform with Emscripten and mobile devices.
  //
  // EX:
  //   while(!bfWindow_wantsToClose(main_window))
  //   {
  //     bfPlatformPumpEvents();
  //     main_window->frame_fn(main_window);
  //   }
  //

  // Shutdown Code Here

  bfPlatformDestroyWindow(main_window);
  bfPlatformQuit();
}
```

## Build System

- CMake v3.12
- MSVC (VS2019) On Windows and Clang on MacOS

## Modules

### Platform

This is a abstraction for handling windowing and events for the various platforms.
Also defines some macros on which platform you are currently on.

Build Requirements:
- Libraries
  - GLFW or SDL (Changes based on the CMake Configuration)
  - Vulkan (If you choose the Vulkan graphics backend)
    - There is no linking to the Vulkan library, just needs headers.
- Compiler
  - C99

### Script

This is a small virtual machine for a custom scripting language in the same vein as Lua.

There is a more in depth [Language Guide](docs/bifrost_script.md) if you want to learn more.

Build Requirements:
  - [Bifrost Data Structures](#Data-Structures)
    - The source is embedded so not a hard requirement.
  - Compiler
    - C99
    - C++17 (Optional helpers for easier binding)

## External Libraries

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
- D3D12 Extension Library (d3dx12.h)

---

# OLD DOCS

### Data Structures

Relies on:
- [Memory](#Memory)
- [Utility](#Utility)


### Graphics

Relies on:
- [Data Structures](#Data-Structures)

Build Requirements:
- C++11 Implementation
- C99 API

### Memory

Relies on:

Build Requirements:
- C++11 API and Implementation

### Meta System

Relies on:
- [Memory](#Memory)
- [Data Structures](#Data Structures)

Build Requirements:
- C++17 API and Implementation

### Utility

Relies on:

Build Requirements:
- C++17 For Hash
- C99 for UUID
