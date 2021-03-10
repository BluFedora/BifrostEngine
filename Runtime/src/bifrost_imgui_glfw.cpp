#include "bf/bifrost_imgui_glfw.hpp"

#include "bf/Platform.h"
#include "bf/bf_gfx_api.h"
#include "bf/editor/bf_editor_icons.hpp"
#include "bf/math/bifrost_mat4x4.h"

#include <glfw/glfw3.h>  // TODO(SR): Remove, but still needed for Cursors, Clipboard, and monitors.
#include <imgui/imgui.h>

#include <algorithm>
#include <memory>  // unique_ptr

namespace bf::imgui
{
  struct UIFrameData
  {
    bfBufferHandle vertex_buffer  = nullptr;
    bfBufferHandle index_buffer   = nullptr;
    bfBufferHandle uniform_buffer = nullptr;

    void create(bfGfxDeviceHandle device)
    {
      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = 0x100;
      buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_UNIFORM_BUFFER;

      uniform_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
    }

    void setTexture(bfGfxCommandListHandle command_list, bfTextureHandle texture)
    {
      uint64_t offset = 0;
      uint64_t sizes  = sizeof(Mat4x4);

      bfDescriptorSetInfo desc_set = bfDescriptorSetInfo_make();
      bfDescriptorSetInfo_addTexture(&desc_set, 0, 0, &texture, 1);
      bfDescriptorSetInfo_addUniform(&desc_set, 1, 0, &offset, &sizes, &uniform_buffer, 1);

      bfGfxCmdList_bindDescriptorSet(command_list, 0, &desc_set);
    }

    void checkSizes(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
    {
      if (!vertex_buffer || bfBuffer_size(vertex_buffer) < vertex_size)
      {
        bfGfxDevice_release(device, vertex_buffer);

        bfBufferCreateParams buffer_params;
        buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
        buffer_params.allocation.size       = vertex_size;
        buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_VERTEX_BUFFER;

        vertex_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
      }

      if (!index_buffer || bfBuffer_size(index_buffer) < indices_size)
      {
        bfGfxDevice_release(device, index_buffer);

        bfBufferCreateParams buffer_params;
        buffer_params.allocation.properties = BF_BUFFER_PROP_HOST_MAPPABLE | BF_BUFFER_PROP_HOST_CACHE_MANAGED;
        buffer_params.allocation.size       = indices_size;
        buffer_params.usage                 = BF_BUFFER_USAGE_TRANSFER_DST | BF_BUFFER_USAGE_INDEX_BUFFER;

        index_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
      }
    }

    void destroy(bfGfxDeviceHandle device) const
    {
      bfGfxDevice_release(device, vertex_buffer);
      bfGfxDevice_release(device, index_buffer);
      bfGfxDevice_release(device, uniform_buffer);
    }
  };

  struct UIRenderData;

  struct UIRenderer final
  {
    bfGfxDeviceHandle       device;
    bfVertexLayoutSetHandle vertex_layout;
    bfShaderModuleHandle    vertex_shader;
    bfShaderModuleHandle    fragment_shader;
    bfTextureHandle         font;
    bfShaderProgramHandle   program;
    UIRenderData*           main_viewport_data;
  };

  static GLFWcursor* s_MouseCursors[ImGuiMouseCursor_COUNT] = {};
  static UIRenderer  s_RenderData                           = {};

  struct UIRenderData final
  {
   private:
    std::size_t                    num_buffers;
    std::unique_ptr<UIFrameData[]> buffers;

   public:
    explicit UIRenderData(std::size_t num_buffers) :
      num_buffers{num_buffers},
      buffers{std::make_unique<UIFrameData[]>(num_buffers)}
    {
      forEachBuffer([](UIFrameData& buffer) {
        buffer.create(s_RenderData.device);
      });
    }

    UIFrameData& grabFrameData(const uint32_t index) const
    {
      assert(index < num_buffers);

      return buffers[index];
    }

    ~UIRenderData()
    {
      forEachBuffer([](const UIFrameData& buffer) {
        buffer.destroy(s_RenderData.device);
      });
    }

   private:
    template<typename F>
    void forEachBuffer(F&& fn) const
    {
      for (std::size_t i = 0; i < num_buffers; ++i)
      {
        fn(buffers[i]);
      }
    }
  };

  static const char* GLFW_ClipboardGet(void* user_data);
  static void        GLFW_ClipboardSet(void* user_data, const char* text);

  static void ImGui_ImplGlfw_UpdateMonitors()
  {
#ifndef __EMSCRIPTEN__
    ImGuiPlatformIO& platform_io    = ImGui::GetPlatformIO();
    int              monitors_count = 0;
    GLFWmonitor**    glfw_monitors  = glfwGetMonitors(&monitors_count);
    platform_io.Monitors.resize(0);
    for (int n = 0; n < monitors_count; n++)
    {
      ImGuiPlatformMonitor monitor;
      int                  x, y;
      glfwGetMonitorPos(glfw_monitors[n], &x, &y);
      const GLFWvidmode* vid_mode = glfwGetVideoMode(glfw_monitors[n]);
      monitor.MainPos = monitor.WorkPos = ImVec2((float)x, (float)y);
      monitor.MainSize = monitor.WorkSize = ImVec2((float)vid_mode->width, (float)vid_mode->height);

      float x_scale, y_scale;
      glfwGetMonitorContentScale(glfw_monitors[n], &x_scale, &y_scale);
      monitor.DpiScale = x_scale;

      platform_io.Monitors.push_back(monitor);
    }
#endif
  }

  static void   ImGui_PlatformCreateWindow(ImGuiViewport* vp);
  static void   ImGui_PlatformDestroyWindow(ImGuiViewport* vp);
  static void   ImGui_PlatformShowWindow(ImGuiViewport* vp);
  static void   ImGui_PlatformSetWindowPos(ImGuiViewport* vp, ImVec2 pos);
  static ImVec2 ImGui_PlatformGetWindowPos(ImGuiViewport* vp);
  static void   ImGui_PlatformSetWindowSize(ImGuiViewport* vp, ImVec2 size);
  static ImVec2 ImGui_PlatformGetWindowSize(ImGuiViewport* vp);
  static void   ImGui_PlatformSetWindowFocus(ImGuiViewport* vp);
  static bool   ImGui_PlatformGetWindowFocus(ImGuiViewport* vp);
  static bool   ImGui_PlatformGetWindowMinimized(ImGuiViewport* vp);
  static void   ImGui_PlatformSetWindowTitle(ImGuiViewport* vp, const char* str);
  static void   ImGui_PlatformSetWindowAlpha(ImGuiViewport* vp, float alpha);
  static void   ImGui_RendererCreateWindow(ImGuiViewport* vp);
  static void   ImGui_RendererDestroyWindow(ImGuiViewport* vp);
  static void   ImGui_RendererSetWindowSize(ImGuiViewport* vp, ImVec2 size);
  static void   ImGui_RendererRenderWindow(ImGuiViewport* vp, void* render_arg);

  void startup(bfWindow* window)
  {
    ImGui::CreateContext(nullptr);

    const float dpi_scale_factor = bfPlatformGetDPIScale();
    ImGuiIO&    io               = ImGui::GetIO();

    // io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;

    // Basic Setup
    io.BackendPlatformName               = "Bifrost GLFW Backend";
    io.BackendRendererName               = "Bifrost Graphics";
    io.IniFilename                       = nullptr;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.ConfigDockingAlwaysTabBar         = true;

    // Keyboard Setup
    io.KeyMap[ImGuiKey_Tab]         = BIFROST_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow]   = BIFROST_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow]  = BIFROST_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]     = BIFROST_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow]   = BIFROST_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp]      = BIFROST_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown]    = BIFROST_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home]        = BIFROST_KEY_HOME;
    io.KeyMap[ImGuiKey_End]         = BIFROST_KEY_END;
    io.KeyMap[ImGuiKey_Insert]      = BIFROST_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete]      = BIFROST_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace]   = BIFROST_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space]       = BIFROST_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter]       = BIFROST_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape]      = BIFROST_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = BIFROST_KEY_PAD_ENTER;
    io.KeyMap[ImGuiKey_A]           = BIFROST_KEY_A;
    io.KeyMap[ImGuiKey_C]           = BIFROST_KEY_C;
    io.KeyMap[ImGuiKey_V]           = BIFROST_KEY_V;
    io.KeyMap[ImGuiKey_X]           = BIFROST_KEY_X;
    io.KeyMap[ImGuiKey_Y]           = BIFROST_KEY_Y;
    io.KeyMap[ImGuiKey_Z]           = BIFROST_KEY_Z;

    io.GetClipboardTextFn = GLFW_ClipboardGet;
    io.SetClipboardTextFn = GLFW_ClipboardSet;
    io.ClipboardUserData  = nullptr;

    // Mouse Cursors
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    s_MouseCursors[ImGuiMouseCursor_Arrow]      = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_TextInput]  = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNS]   = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeEW]   = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_Hand]       = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeAll]  = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

    // Docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Viewport
#if !BIFROST_PLATFORM_ANDROID && !BIFROST_PLATFORM_IOS && !BIFROST_PLATFORM_EMSCRIPTEN
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_RendererHasViewports;

      ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

      platform_io.Platform_CreateWindow       = &ImGui_PlatformCreateWindow;
      platform_io.Platform_DestroyWindow      = &ImGui_PlatformDestroyWindow;
      platform_io.Platform_ShowWindow         = &ImGui_PlatformShowWindow;
      platform_io.Platform_SetWindowPos       = &ImGui_PlatformSetWindowPos;
      platform_io.Platform_GetWindowPos       = &ImGui_PlatformGetWindowPos;
      platform_io.Platform_SetWindowSize      = &ImGui_PlatformSetWindowSize;
      platform_io.Platform_GetWindowSize      = &ImGui_PlatformGetWindowSize;
      platform_io.Platform_SetWindowFocus     = &ImGui_PlatformSetWindowFocus;
      platform_io.Platform_GetWindowFocus     = &ImGui_PlatformGetWindowFocus;
      platform_io.Platform_GetWindowMinimized = &ImGui_PlatformGetWindowMinimized;
      platform_io.Platform_SetWindowTitle     = &ImGui_PlatformSetWindowTitle;
      platform_io.Platform_SetWindowAlpha     = &ImGui_PlatformSetWindowAlpha;
      platform_io.Renderer_CreateWindow       = &ImGui_RendererCreateWindow;
      platform_io.Renderer_DestroyWindow      = &ImGui_RendererDestroyWindow;
      platform_io.Renderer_SetWindowSize      = &ImGui_RendererSetWindowSize;
      platform_io.Renderer_RenderWindow       = &ImGui_RendererRenderWindow;

      ImGui_ImplGlfw_UpdateMonitors();
    }

    // Renderer Setup

    std::memset(&s_RenderData, 0x0, sizeof(s_RenderData));
    const auto device = s_RenderData.device = bfGfxGetDevice();

    s_RenderData.main_viewport_data = new UIRenderData(bfGfxContext_getFrameInfo().num_frame_indices);

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      ImGuiViewport* main_viewport     = ImGui::GetMainViewport();
      main_viewport->PlatformHandle    = window;
      main_viewport->RendererUserData  = window->renderer_data;
      main_viewport->PlatformHandleRaw = s_RenderData.main_viewport_data;
    }

    // Renderer Setup: Vertex Layout
    const auto vertex_layout = s_RenderData.vertex_layout = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(vertex_layout, 0, sizeof(ImDrawVert));
    bfVertexLayout_addVertexLayout(vertex_layout, 0, BF_VFA_FLOAT32_2, offsetof(ImDrawVert, pos));
    bfVertexLayout_addVertexLayout(vertex_layout, 0, BF_VFA_FLOAT32_2, offsetof(ImDrawVert, uv));
    bfVertexLayout_addVertexLayout(vertex_layout, 0, BF_VFA_UCHAR8_4_UNORM, offsetof(ImDrawVert, col));

    // Renderer Setup: Shaders
    bfShaderProgramCreateParams create_shader;
    create_shader.debug_name    = "ImGui.Shader";
    create_shader.num_desc_sets = 1;

    s_RenderData.vertex_shader   = bfGfxDevice_newShaderModule(device, BF_SHADER_TYPE_VERTEX);
    s_RenderData.fragment_shader = bfGfxDevice_newShaderModule(device, BF_SHADER_TYPE_FRAGMENT);
    s_RenderData.program         = bfGfxDevice_newShaderProgram(device, &create_shader);

    if (bfPlatformGetGfxAPI() == BIFROST_PLATFORM_GFX_OPENGL)
    {
      bfShaderModule_loadFile(s_RenderData.vertex_shader, "assets/shaders/es3/imgui.vert.glsl");
      bfShaderModule_loadFile(s_RenderData.fragment_shader, "assets/shaders/es3/imgui.frag.glsl");
    }
    else
    {
      bfShaderModule_loadFile(s_RenderData.vertex_shader, "assets/imgui.vert.spv");
      bfShaderModule_loadFile(s_RenderData.fragment_shader, "assets/imgui.frag.spv");
    }

    bfShaderProgram_addModule(s_RenderData.program, s_RenderData.vertex_shader);
    bfShaderProgram_addModule(s_RenderData.program, s_RenderData.fragment_shader);

    bfShaderProgram_link(s_RenderData.program);

    bfShaderProgram_addImageSampler(s_RenderData.program, "u_Texture", 0, 0, 1, BF_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addUniformBuffer(s_RenderData.program, "u_Set0", 0, 1, 1, BF_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(s_RenderData.program);

    // Renderer Setup: Font Texture

    const float font_size = 18.0f * dpi_scale_factor;

    ImGui::GetStyle().ScaleAllSizes(dpi_scale_factor);

    //io.Fonts->AddFontDefault();

    io.Fonts->AddFontFromFileTTF(
     "assets/fonts/Ropa_Sans/RopaSans-Regular.ttf",  // "assets/fonts/Abel.ttf",
     font_size,
     nullptr,
     nullptr);

    ImFontConfig config;
    config.MergeMode = true;
    // config.PixelSnapH = true;
    // config.GlyphMinAdvanceX = font_size;  // Use if you want to make the icon monospaced

    // THis is static sine ImGui says the data must persist as long as the font is alive (based on a comment in ImFontConfig::GlyphRanges)
    static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FAS, font_size - 5.0f, &config, icon_ranges);

    unsigned char* pixels;
    int            width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    bfTextureCreateParams      create_texture = bfTextureCreateParams_init2D(BF_IMAGE_FORMAT_R8G8B8A8_UNORM, width, height);
    bfTextureSamplerProperties sampler        = bfTextureSamplerProperties_init(BF_SFM_NEAREST, BF_SAM_REPEAT);

    create_texture.generate_mipmaps = bfFalse;

    s_RenderData.font = bfGfxDevice_newTexture(device, &create_texture);
    bfTexture_loadData(s_RenderData.font, reinterpret_cast<char*>(pixels), width * height * bytes_per_pixel * sizeof(pixels[0]));
    bfTexture_setSampler(s_RenderData.font, &sampler);
  }

  void onEvent(bfWindow* target_window, Event& evt)
  {
    ImGuiIO& io = ImGui::GetIO();

    switch (evt.type)
    {
      case BIFROST_EVT_ON_WINDOW_RESIZE:
      {
        // io.DisplaySize = ImVec2(float(evt.window.width), float(evt.window.height));
        break;
      }
      case BIFROST_EVT_ON_MOUSE_MOVE:
      {
        io.MousePos = ImVec2(static_cast<float>(evt.mouse.x), static_cast<float>(evt.mouse.y));

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
          int window_x, window_y;
          bfWindow_getPos(target_window, &window_x, &window_y);

          io.MousePos.x += float(window_x);
          io.MousePos.y += float(window_y);
        }

        break;
      }
      case BIFROST_EVT_ON_MOUSE_UP:
      case BIFROST_EVT_ON_MOUSE_DOWN:
      {
        const bool is_down = evt.type == BIFROST_EVT_ON_MOUSE_DOWN;

        switch (evt.mouse.target_button)
        {
          case BIFROST_BUTTON_LEFT:
          {
            io.MouseDown[0] = is_down;
            break;
          }
          case BIFROST_BUTTON_MIDDLE:
          {
            io.MouseDown[2] = is_down;
            break;
          }
          case BIFROST_BUTTON_RIGHT:
          {
            io.MouseDown[1] = is_down;
            break;
          }
          default:
            break;
        }

        break;
      }
      case BIFROST_EVT_ON_KEY_UP:
      case BIFROST_EVT_ON_KEY_DOWN:
      {
        const auto key = evt.keyboard.key;

        io.KeysDown[key] = (evt.type == BIFROST_EVT_ON_KEY_DOWN);

        io.KeyCtrl  = evt.keyboard.modifiers & BIFROST_KEY_FLAG_CONTROL;
        io.KeyShift = evt.keyboard.modifiers & BIFROST_KEY_FLAG_SHIFT;
        io.KeyAlt   = evt.keyboard.modifiers & BIFROST_KEY_FLAG_ALT;
        io.KeySuper = evt.keyboard.modifiers & BIFROST_KEY_FLAG_SUPER;
        break;
      }
      case BIFROST_EVT_ON_KEY_INPUT:
      {
        io.AddInputCharacter(evt.keyboard.codepoint);
        break;
      }
      case BIFROST_EVT_ON_SCROLL_WHEEL:
      {
        io.MouseWheelH += static_cast<float>(evt.scroll_wheel.x);
        io.MouseWheel += static_cast<float>(evt.scroll_wheel.y);
        break;
      }
      default:
        break;
    }
  }

  void beginFrame(bfTextureHandle surface, float window_width, float window_height, float delta_time)
  {
    ImGuiIO&    io                 = ImGui::GetIO();
    const float framebuffer_width  = float(bfTexture_width(surface));
    const float framebuffer_height = float(bfTexture_height(surface));

    io.DisplaySize = ImVec2(window_width, window_height);

    if (window_width > 0.0f && window_height > 0.0f)
    {
      io.DisplayFramebufferScale = ImVec2(framebuffer_width / window_width, framebuffer_height / window_height);
    }

    io.DeltaTime = delta_time;

    ImGui::NewFrame();
  }

  static void frameResetState(bfGfxCommandListHandle command_list, UIFrameData& frame, int fb_width, int fb_height)
  {
    uint64_t vertex_buffer_offset = 0;

    bfGfxCmdList_setCullFace(command_list, BF_CULL_FACE_NONE);
    bfGfxCmdList_setDynamicStates(command_list, BF_PIPELINE_DYNAMIC_VIEWPORT | BF_PIPELINE_DYNAMIC_SCISSOR);
    bfGfxCmdList_bindVertexDesc(command_list, s_RenderData.vertex_layout);
    bfGfxCmdList_bindVertexBuffers(command_list, 0, &frame.vertex_buffer, 1, &vertex_buffer_offset);
    bfGfxCmdList_bindIndexBuffer(command_list, frame.index_buffer, 0, bfIndexTypeFromT<ImDrawIdx>());
    bfGfxCmdList_bindProgram(command_list, s_RenderData.program);
    frame.setTexture(command_list, s_RenderData.font);
    bfGfxCmdList_setViewport(command_list, 0.0f, 0.0f, float(fb_width), float(fb_height), nullptr);
  }

  static void frameDraw(ImGuiViewport* viewport, ImDrawData* draw_data, bfWindowSurfaceHandle window, UIFrameData& frame)
  {
    if (draw_data)
    {
      auto& io = ImGui::GetIO();

      if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
      {
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
          //ImGuiViewport* const   main_viewport = ImGui::GetMainViewport();
          bfWindow* const        window       = static_cast<bfWindow*>(viewport->PlatformHandle);
          GLFWwindow* const      glfw_window  = static_cast<GLFWwindow*>(window->handle);
          const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();

          if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
          {
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
          }
          else
          {
            glfwSetCursor(glfw_window, s_MouseCursors[imgui_cursor] ? s_MouseCursors[imgui_cursor] : s_MouseCursors[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          }
        }
      }

      const bfGfxCommandListHandle command_list = bfGfxContext_requestCommandList(window, 0u);
      const int                    fb_width     = static_cast<int>(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
      const int                    fb_height    = static_cast<int>(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);

      if (fb_width <= 0 || fb_height <= 0)
        return;

      const size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
      const size_t index_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

      if (vertex_size == 0 || index_size == 0)
      {
        return;
      }

      frame.checkSizes(s_RenderData.device, vertex_size, index_size);

      ImDrawVert* vertex_buffer_ptr  = static_cast<ImDrawVert*>(bfBuffer_map(frame.vertex_buffer, 0, k_bfBufferWholeSize));
      ImDrawIdx*  index_buffer_ptr   = static_cast<ImDrawIdx*>(bfBuffer_map(frame.index_buffer, 0, k_bfBufferWholeSize));
      Mat4x4*     uniform_buffer_ptr = static_cast<Mat4x4*>(bfBuffer_map(frame.uniform_buffer, 0, sizeof(Mat4x4)));

      for (int i = 0; i < draw_data->CmdListsCount; ++i)
      {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];

        std::memcpy(vertex_buffer_ptr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(index_buffer_ptr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));

        vertex_buffer_ptr += cmd_list->VtxBuffer.Size;
        index_buffer_ptr += cmd_list->IdxBuffer.Size;
      }

      const ImVec2 tl = draw_data->DisplayPos;
      const ImVec2 br = {tl.x + draw_data->DisplaySize.x, tl.y + draw_data->DisplaySize.y};

      if (bfPlatformGetGfxAPI() == BIFROST_PLATFORM_GFX_OPENGL)
      {
        Mat4x4_ortho(uniform_buffer_ptr, tl.x, br.x, br.y, tl.y, 0.0f, 1.0f);
      }
      else
      {
        Mat4x4_orthoVk(uniform_buffer_ptr, tl.x, br.x, br.y, tl.y, 0.0f, 1.0f);
      }

      bfBuffer_unMap(frame.vertex_buffer);
      bfBuffer_unMap(frame.index_buffer);
      bfBuffer_unMap(frame.uniform_buffer);

      frameResetState(command_list, frame, fb_width, fb_height);

      const ImVec2 clip_off          = draw_data->DisplayPos;
      const ImVec2 clip_scale        = draw_data->FramebufferScale;
      int          global_vtx_offset = 0;
      int          global_idx_offset = 0;

      for (int n = 0; n < draw_data->CmdListsCount; ++n)
      {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i)
        {
          const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

          frame.setTexture(command_list, pcmd->TextureId ? (bfTextureHandle)pcmd->TextureId : s_RenderData.font);

          if (pcmd->UserCallback)
          {
            if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            {
              frameResetState(command_list, frame, fb_width, fb_height);
            }
            else
            {
              pcmd->UserCallback(cmd_list, pcmd);
            }
          }
          else
          {
            ImVec4 clip_rect;
            clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
            clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
            clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
            clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

            if (clip_rect.x < float(fb_width) && clip_rect.y < float(fb_height) && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
            {
              clip_rect.x = std::max(0.0f, clip_rect.x);
              clip_rect.y = std::max(0.0f, clip_rect.y);

              bfGfxCmdList_setScissorRect(
               command_list,
               int32_t(clip_rect.x),
               int32_t(clip_rect.y),
               uint32_t(clip_rect.z - clip_rect.x),
               uint32_t(clip_rect.w - clip_rect.y));

              bfGfxCmdList_drawIndexed(
               command_list,
               pcmd->ElemCount,
               pcmd->IdxOffset + global_idx_offset,
               pcmd->VtxOffset + global_vtx_offset);
            }
          }
        }

        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
      }
    }
  }

  void endFrame()
  {
    ImGuiViewport* const main_viewport = ImGui::GetMainViewport();
    const bfGfxFrameInfo info          = bfGfxContext_getFrameInfo();

    ImGui::Render();
    frameDraw(main_viewport, ImGui::GetDrawData(), (bfWindowSurfaceHandle)main_viewport->RendererUserData, s_RenderData.main_viewport_data->grabFrameData(info.frame_index));

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
    }
  }

  void shutdown()
  {
    bfGfxDevice_flush(s_RenderData.device);
    bfVertexLayout_delete(s_RenderData.vertex_layout);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.vertex_shader);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.fragment_shader);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.program);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.font);

    for (auto& cursor : s_MouseCursors)
    {
      glfwDestroyCursor(cursor);
      cursor = nullptr;
    }

    ImGui::DestroyContext(nullptr);
  }

  void setupDefaultRenderPass(bfGfxCommandListHandle command_list, bfTextureHandle surface)
  {
    bfAttachmentInfo main_surface;
    main_surface.texture      = surface;
    main_surface.final_layout = BF_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    main_surface.may_alias    = bfFalse;

    auto renderpass_info = bfRenderpassInfo_init(1);
    bfRenderpassInfo_setLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStencilLoadOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setClearOps(&renderpass_info, bfBit(0));
    bfRenderpassInfo_setStencilClearOps(&renderpass_info, 0x0);
    bfRenderpassInfo_setStoreOps(&renderpass_info, bfBit(0));
    bfRenderpassInfo_setStencilStoreOps(&renderpass_info, 0x0);
    bfRenderpassInfo_addAttachment(&renderpass_info, &main_surface);
    bfRenderpassInfo_addColorOut(&renderpass_info, 0, 0, BF_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    bfClearValue clear_colors[1];
    clear_colors[0].color.float32[0] = 0.6f;
    clear_colors[0].color.float32[1] = 0.6f;
    clear_colors[0].color.float32[2] = 0.75f;
    clear_colors[0].color.float32[3] = 1.0f;

    bfTextureHandle attachments[] = {surface};

    bfGfxCmdList_setDepthTesting(command_list, bfFalse);
    bfGfxCmdList_setDepthWrite(command_list, bfFalse);

    bfGfxCmdList_setRenderpassInfo(command_list, &renderpass_info);
    bfGfxCmdList_setClearValues(command_list, clear_colors);
    bfGfxCmdList_setAttachments(command_list, attachments);
    bfGfxCmdList_setRenderAreaRel(command_list, 0.0f, 0.0f, 1.0f, 1.0f);
    bfGfxCmdList_beginRenderpass(command_list);
  }

  static const char* GLFW_ClipboardGet(void* user_data)
  {
    (void)user_data;
    return glfwGetClipboardString(NULL);
  }

  static void GLFW_ClipboardSet(void* user_data, const char* text)
  {
    (void)user_data;
    glfwSetClipboardString(NULL, text);
  }

  static uint32_t convertVpFlag(ImGuiViewport* vp, ImGuiViewportFlags_ im_flag, uint32_t bf_flag)
  {
    return (vp->Flags & im_flag) ? bf_flag : 0x0;
  }

  static void ImGui_PlatformCreateWindow(ImGuiViewport* vp)
  {
    bfWindow* const window = bfPlatformCreateWindow(
     "__Untitled__",
     int(vp->Size.x),
     int(vp->Size.y),
     convertVpFlag(vp, ImGuiViewportFlags_NoDecoration, k_bfWindowFlagIsDecorated) |
      convertVpFlag(vp, ImGuiViewportFlags_TopMost, k_bfWindowFlagIsFloating));

    window->event_fn = [](bfWindow* window, bfEvent* event) {
      onEvent(window, *event);
    };

    window->frame_fn = [](bfWindow* window) {
      ImGuiViewport* vp = ImGui::FindViewportByPlatformHandle(window->user_data);

      if (vp)
      {
        ImGui_RendererRenderWindow(vp, nullptr);
      }
    };

    vp->PlatformHandle    = window;
    vp->PlatformHandleRaw = nullptr;
    vp->RendererUserData  = nullptr;

    bfWindow_setPos(window, (int)vp->Pos.x, (int)vp->Pos.y);
  }

  static void ImGui_PlatformDestroyWindow(ImGuiViewport* vp)
  {
    if (ImGui::GetMainViewport() != vp)
    {
      bfPlatformDestroyWindow((bfWindow*)vp->PlatformHandle);
    }

    vp->PlatformHandle = NULL;
  }

  static void ImGui_PlatformShowWindow(ImGuiViewport* vp)
  {
    bfWindow_show((bfWindow*)vp->PlatformHandle);
  }

  static void ImGui_PlatformSetWindowPos(ImGuiViewport* vp, ImVec2 pos)
  {
    bfWindow_setPos((bfWindow*)vp->PlatformHandle, (int)pos.x, (int)pos.y);
  }

  static ImVec2 ImGui_PlatformGetWindowPos(ImGuiViewport* vp)
  {
    int x, y;
    bfWindow_getPos((bfWindow*)vp->PlatformHandle, &x, &y);
    return {(float)x, float(y)};
  }

  static void ImGui_PlatformSetWindowSize(ImGuiViewport* vp, ImVec2 size)
  {
    bfWindow_setSize((bfWindow*)vp->PlatformHandle, (int)size.x, (int)size.y);
  }

  static ImVec2 ImGui_PlatformGetWindowSize(ImGuiViewport* vp)
  {
    int x, y;
    bfWindow_getSize((bfWindow*)vp->PlatformHandle, &x, &y);
    return {(float)x, float(y)};
  }

  static void ImGui_PlatformSetWindowFocus(ImGuiViewport* vp)
  {
    bfWindow_focus((bfWindow*)vp->PlatformHandle);
  }

  static bool ImGui_PlatformGetWindowFocus(ImGuiViewport* vp)
  {
    return bfWindow_isFocused((bfWindow*)vp->PlatformHandle);
  }

  static bool ImGui_PlatformGetWindowMinimized(ImGuiViewport* vp)
  {
    return bfWindow_isMinimized((bfWindow*)vp->PlatformHandle);
  }

  static void ImGui_PlatformSetWindowTitle(ImGuiViewport* vp, const char* str)
  {
    bfWindow_setTitle((bfWindow*)vp->PlatformHandle, str);
  }

  static void ImGui_PlatformSetWindowAlpha(ImGuiViewport* vp, float alpha)
  {
    bfWindow_setAlpha((bfWindow*)vp->PlatformHandle, alpha);
  }

  static void ImGui_RendererCreateWindow(ImGuiViewport* vp)
  {
    bfWindow* const             bf_window      = (bfWindow*)vp->PlatformHandle;
    const bfWindowSurfaceHandle surface        = bfGfxContext_createWindow(bf_window);
    const bfGfxFrameInfo        info           = bfGfxContext_getFrameInfo();
    UIRenderData* const         ui_render_data = new UIRenderData(info.num_frame_indices);

    vp->RendererUserData  = surface;
    vp->PlatformHandleRaw = ui_render_data;
    bf_window->user_data  = ui_render_data;
  }

  static void ImGui_RendererDestroyWindow(ImGuiViewport* vp)
  {
    bfGfxDevice_flush(s_RenderData.device);

    UIRenderData* const         ui_render_data = (UIRenderData*)vp->PlatformHandleRaw;
    const bfWindowSurfaceHandle surface        = (bfWindowSurfaceHandle)vp->RendererUserData;

    delete ui_render_data;

    if (ImGui::GetMainViewport() != vp)
    {
      bfGfxContext_destroyWindow(surface);
    }

    vp->PlatformHandleRaw = NULL;
    vp->RendererUserData  = NULL;
  }

  static void ImGui_RendererSetWindowSize(ImGuiViewport* vp, ImVec2 size)
  {
    // TODO(SR): ?
  }

  static void ImGui_RendererRenderWindow(ImGuiViewport* vp, void* render_arg)
  {
    (void)render_arg;

    UIRenderData* const         ui_render_data = (UIRenderData*)vp->PlatformHandleRaw;
    const bfWindowSurfaceHandle surface        = (bfWindowSurfaceHandle)vp->RendererUserData;

    if (bfGfxContext_beginFrame(surface))
    {
      const bfGfxCommandListHandle command_list = bfGfxContext_requestCommandList(surface, 0u);

      if (bfGfxCmdList_begin(command_list))
      {
        const bfTextureHandle surface_tex = bfGfxDevice_requestSurface(surface);
        const bfGfxFrameInfo  info        = bfGfxContext_getFrameInfo();

        setupDefaultRenderPass(command_list, surface_tex);
        frameDraw(vp, vp->DrawData, surface, ui_render_data->grabFrameData(info.frame_index));
        bfGfxCmdList_endRenderpass(command_list);

        bfGfxCmdList_end(command_list);
        bfGfxCmdList_submit(command_list);
      }
    }
  }
}  // namespace bf::imgui
