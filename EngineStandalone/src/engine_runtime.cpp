#define WIN32_LEAN_AND_MEAN

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
 
#include <d3dx12.h> // D3D12 extension library.

#include <wrl.h>
using namespace Microsoft::WRL;

// #define SDL_MAIN_HANDLED
#include <sdl/SDL.h>
#include <sdl/SDL_vulkan.h>

#include <cstdio>

enum ErrorCode
{
  ERROR_NONE       = 0,
  ERROR_SDL_INIT   = 1,
  ERROR_SDL_WINDOW = 2,
};

// Dx12 Notes:
/*
When using runtime compiled HLSL shaders using any of the D3DCompiler functions, do not forget to link against the d3dcompiler.lib library and copy the D3dcompiler_47.dll to the same folder as the binary executable when distributing your project.
A redistributable version of the D3dcompiler_47.dll file can be found in the Windows 10 SDK installation folder at C:\Program Files (x86)\Windows Kits\10\Redist\D3D\.
*/

extern "C" int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  ErrorCode error = ERROR_NONE;

  if (SDL_Init(SDL_INIT_EVERYTHING))
  {
    std::printf("Failed to initialize SDL2: %s\n", SDL_GetError());
    error = ERROR_SDL_INIT;
    goto quit_app;
  }

  SDL_Window* const window = SDL_CreateWindow("Space Invaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

  if (!window)
  {
    std::printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
    error = ERROR_SDL_WINDOW;
    goto quit_sdl;
  }

  unsigned int count = 0;
  SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);

  const char** names = new const char*[count];

  SDL_Vulkan_GetInstanceExtensions(window, &count, names);

  for (unsigned int i = 0; i < count; i++)
  {
    std::printf("Extension %i: %s\n", i, names[i]);
  }

  delete[] names;

  SDL_Event evt;
  bool      quit = false;

  while (!quit)
  {
    while (SDL_PollEvent(&evt))
    {
      if (evt.type == SDL_QUIT)
      {
        quit = true;
      }

      if (evt.type == SDL_KEYDOWN)
      {
        quit = true;
      }
    }
  }

  SDL_DestroyWindow(window);

quit_sdl:
  SDL_Quit();

quit_app:
  return error;
}