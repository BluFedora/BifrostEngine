#ifndef BIFROST_IMGUI_GLFW
#define BIFROST_IMGUI_GLFW

#include "bifrost/platform/bifrost_ibase_window.hpp"
#include "bifrost/graphics/bifrost_gfx_handle.h"

namespace bifrost
{
  struct Event;

  namespace imgui
  {
    void startup(bfGfxContextHandle graphics, IBaseWindow& window);
    void onEvent(Event& evt);
    void beginFrame(bfTextureHandle surface, float window_width, float window_height, float current_time);
    void endFrame(bfGfxCommandListHandle command_list);
    void shutdown();
  }  // namespace editor::imgui

}  // namespace bifrost

#endif /* BIFROST_IMGUI_GLFW */