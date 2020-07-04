/******************************************************************************/
/*!
 * @file   webgl_testing.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Tetsing out a web backend for the engine.
 *
 * @version 0.0.1
 * @date    2020-06-30
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/

#include <bifrost/graphics/bifrost_gfx_api.h>  /* Graphics API */
#include <bifrost/platform/bifrost_platform.h> /* Platform API */

#include <bifrost_imgui_glfw.hpp> /* bifrost::imgui::*      */
#include <imgui/imgui.h>          /* ImGui::* */

#include <cstdio>  // printf

struct Application final
{
  bfGfxContextHandle    gfx_ctx;
  bfWindowSurfaceHandle main_window_surface;
};

int main(int argc, char* argv[])
{
  if (!bfPlatformInit({argc, argv, nullptr, nullptr}))
  {
    std::printf("Failed to initialize the platform.\n");
    return 1;
  }

  BifrostWindow* const main_window = bfPlatformCreateWindow("Reefy Web Game Dev", 1920, 1080, BIFROST_WINDOW_FLAGS_DEFAULT);

  if (!main_window)
  {
    std::printf("Failed to create the window.\n");
    bfPlatformQuit();
    return 2;
  }

  bfGfxContextCreateParams graphic_params;
  graphic_params.app_name    = "Reefy Web Game Dev";
  graphic_params.app_version = bfGfxMakeVersion(1, 0, 0);

  const bfGfxContextHandle gfx_ctx = bfGfxContext_new(&graphic_params);
  // const bfGfxDeviceHandle     gfx_device   = bfGfxContext_device(gfx_ctx);
  const bfWindowSurfaceHandle main_surface = bfGfxContext_createWindow(gfx_ctx, main_window);

  Application app{gfx_ctx, main_surface};

  main_window->user_data     = &app;
  main_window->renderer_data = main_surface;

  main_window->event_fn = [](BifrostWindow* window, bfEvent* evt) {
    bifrost::imgui::onEvent(window, *evt);
  };

  main_window->frame_fn = [](BifrostWindow* window) {
    Application* const app        = static_cast<Application*>(window->user_data);
    const float        delta_time = 1.0f / 60.0f;

    if (bfGfxContext_beginFrame(app->gfx_ctx, app->main_window_surface))
    {
#if 1
      const bfGfxCommandListHandle main_command_list = bfGfxContext_requestCommandList(app->gfx_ctx, app->main_window_surface, 0);

      if (main_command_list && bfGfxCmdList_begin(main_command_list))
      {
        const bfTextureHandle main_surface_tex = bfGfxDevice_requestSurface(app->main_window_surface);
        int                   window_width;
        int                   window_height;

        bfWindow_getSize(window, &window_width, &window_height);

        bifrost::imgui::beginFrame(
         main_surface_tex,
         float(window_width),
         float(window_height),
         delta_time);

        if (ImGui::Begin("First Window"))
        {
          ImGui::Text("Come On Just Work"); 
        }
        ImGui::End();

        
        if (ImGui::Begin("Another One"))
        {
          ImGui::Text("Some more text my dude.");
        }
        ImGui::End();

        bifrost::imgui::setupDefaultRenderPass(main_command_list, main_surface_tex);
        bifrost::imgui::endFrame();

        bfGfxCmdList_end(main_command_list);
        bfGfxCmdList_submit(main_command_list);
      }
#endif
      bfGfxContext_endFrame(app->gfx_ctx);
    }
  };

  bifrost::imgui::startup(gfx_ctx, main_window);

  bfPlatformDoMainLoop(main_window);

  bifrost::imgui::shutdown();
  bfGfxContext_destroyWindow(gfx_ctx, main_surface);
  bfGfxContext_delete(gfx_ctx);
  bfPlatformDestroyWindow(main_window);
  bfPlatformQuit();

  return 0;
}
