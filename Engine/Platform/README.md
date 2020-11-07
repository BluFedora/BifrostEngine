# BluFedora Platform Layer Library

This is a abstraction for handling windowing and events for the various platforms.
Also defines some macros for detecting which platform you are currently building for.


Minimal Window Creation Example:

```cpp
#include <bf/Platform.h> /* Platform API */

#include <cstdio> /* printf */

int main(int argc, char* argv[])
{
  bfPlatformAllocator allocator       = nullptr; // Use the default realloc and free, will be assigned to 'bfPlatformDefaultAllocator'.
  void*               global_userdata = nullptr; // Could be anything you want globally accessible.

  if (!bfPlatformInit({argc, argv, allocator, global_userdata}))
  {
    std::printf("Failed to initialize the platform.\n");
    return 1;
  }

  bfWindow* const main_window = bfPlatformCreateWindow("Title", 1280, 720, BIFROST_WINDOW_FLAGS_DEFAULT);

  if (!main_window)
  {
    std::printf("Failed to create the window.\n");
    bfPlatformQuit();
    return 2;
  }

  // Initialization Here

  main_window->user_data     = /* some user data here        */;
  main_window->renderer_data = /* another slot for user data */;

  main_window->event_fn = [](bfWindow* window, bfEvent* evt) {
    // Handle Events Here
  };

  main_window->frame_fn = [](bfWindow* window) {
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

## Build Requirements

- Libraries
  - GLFW or SDL (Changes based on the CMake Configuration)
  - Vulkan (If you choose the Vulkan graphics backend)
    - There is no linking to the Vulkan library, just needs headers.
- Compiler
  - C99
