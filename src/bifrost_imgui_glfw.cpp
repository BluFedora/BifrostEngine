#include "bifrost_editor/bifrost_imgui_glfw.hpp"

#include "bifrost/graphics/bifrost_gfx_api.h"
#include "bifrost/math/bifrost_mat4x4.h"
#include <algorithm>
#include <glfw/glfw3.h>
#include <imgui/imgui.h>

namespace bifrost::editor::imgui
{
  struct UIFrameData
  {
    bfBufferHandle        vertex_buffer;
    bfBufferHandle        index_buffer;
    bfBufferHandle        uniform_buffer;
    bfDescriptorSetHandle descriptor_set;

    void create(bfGfxDeviceHandle device, bfShaderProgramHandle program, bfTextureHandle font)
    {
      bfBufferCreateParams buffer_params;
      buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
      buffer_params.allocation.size       = 0x100 * 2;
      buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_UNIFORM_BUFFER;

      uniform_buffer = bfGfxDevice_newBuffer(device, &buffer_params);

      uint64_t offset = 0;
      uint64_t sizes  = sizeof(Mat4x4);

      descriptor_set = bfShaderProgram_createDescriptorSet(program, 0);
      bfDescriptorSet_setCombinedSamplerTextures(descriptor_set, 0, 0, &font, 1);
      bfDescriptorSet_setUniformBuffers(descriptor_set, 1, 0, &offset, &sizes, &uniform_buffer, 1);
      bfDescriptorSet_flushWrites(descriptor_set);
    }

    void checkSizes(bfGfxDeviceHandle device, size_t vertex_size, size_t indices_size)
    {
      if (!vertex_buffer || bfBuffer_size(vertex_buffer) < vertex_size)
      {
        bfGfxDevice_release(device, vertex_buffer);

        bfBufferCreateParams buffer_params;
        buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
        buffer_params.allocation.size       = vertex_size;
        buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_VERTEX_BUFFER;

        vertex_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
      }

      if (!index_buffer || bfBuffer_size(index_buffer) < indices_size)
      {
        bfGfxDevice_release(device, index_buffer);

        bfBufferCreateParams buffer_params;
        buffer_params.allocation.properties = BIFROST_BPF_HOST_MAPPABLE | BIFROST_BPF_HOST_CACHE_MANAGED;
        buffer_params.allocation.size       = indices_size;
        buffer_params.usage                 = BIFROST_BUF_TRANSFER_DST | BIFROST_BUF_INDEX_BUFFER;

        index_buffer = bfGfxDevice_newBuffer(device, &buffer_params);
      }
    }

    void destroy(bfGfxDeviceHandle device) const
    {
      bfGfxDevice_release(device, vertex_buffer);
      bfGfxDevice_release(device, index_buffer);
      bfGfxDevice_release(device, uniform_buffer);
      bfGfxDevice_release(device, descriptor_set);
    }
  };

  struct UIRenderer
  {
    bfGfxContextHandle      ctx;
    bfGfxDeviceHandle       device;
    bfVertexLayoutSetHandle vertex_layout;
    bfShaderModuleHandle    vertex_shader;
    bfShaderModuleHandle    fragment_shader;
    UIFrameData             buffers[2];
    bfTextureHandle         font;
    bfShaderProgramHandle   program;
  };

  static GLFWcursor* s_MouseCursors[ImGuiMouseCursor_COUNT] = {};
  static UIRenderer  s_RenderData                           = {};
  static float       s_Time                                 = 0.0f;

  static const char* GLFW_ClipboardGet(void* user_data);
  static void        GLFW_ClipboardSet(void* user_data, const char* text);

  void startup(bfGfxContextHandle graphics, IBaseWindow& window)
  {
    ImGui::CreateContext(nullptr);

    ImGuiIO& io = ImGui::GetIO();

    // Basic Setup
    io.BackendPlatformName               = "Bifrost GLFW Backend";
    io.BackendRendererName               = "Bifrost Vulkan Backend";
    io.IniFilename                       = nullptr;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    // Keyboard Setup
    io.KeyMap[ImGuiKey_Tab]         = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow]   = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow]  = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]     = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow]   = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp]      = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown]    = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home]        = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End]         = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert]      = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete]      = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace]   = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space]       = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter]       = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape]      = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A]           = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C]           = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V]           = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X]           = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y]           = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z]           = GLFW_KEY_Z;

    // Clipboard
    io.GetClipboardTextFn = GLFW_ClipboardGet;
    io.SetClipboardTextFn = GLFW_ClipboardSet;
    io.ClipboardUserData  = &window;

    // Cursors
    s_MouseCursors[ImGuiMouseCursor_Arrow]      = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_TextInput]  = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNS]   = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeEW]   = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_Hand]       = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeAll]  = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    s_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);

    // Renderer Setup

    std::memset(&s_RenderData, 0x0, sizeof(s_RenderData));
    s_RenderData.ctx  = graphics;
    const auto device = s_RenderData.device = bfGfxContext_device(graphics);

    // Renderer Setup: Vertex Layout
    const auto vertex_layout = s_RenderData.vertex_layout = bfVertexLayout_new();
    bfVertexLayout_addVertexBinding(vertex_layout, 0, sizeof(ImDrawVert));
    bfVertexLayout_addVertexLayout(vertex_layout, 0, BIFROST_VFA_FLOAT32_2, offsetof(ImDrawVert, pos));
    bfVertexLayout_addVertexLayout(vertex_layout, 0, BIFROST_VFA_FLOAT32_2, offsetof(ImDrawVert, uv));
    bfVertexLayout_addVertexLayout(vertex_layout, 0, BIFROST_VFA_UCHAR8_4_UNORM, offsetof(ImDrawVert, col));

    // Renderer Setup: Shaders
    bfShaderProgramCreateParams create_shader;
    create_shader.debug_name    = "ImGui Shader";
    create_shader.num_desc_sets = 1;

    s_RenderData.vertex_shader   = bfGfxDevice_newShaderModule(device, BIFROST_SHADER_TYPE_VERTEX);
    s_RenderData.fragment_shader = bfGfxDevice_newShaderModule(device, BIFROST_SHADER_TYPE_FRAGMENT);
    s_RenderData.program         = bfGfxDevice_newShaderProgram(device, &create_shader);

    bfShaderModule_loadFile(s_RenderData.vertex_shader, "../assets/imgui.vert.spv");
    bfShaderModule_loadFile(s_RenderData.fragment_shader, "../assets/imgui.frag.spv");

    bfShaderProgram_addModule(s_RenderData.program, s_RenderData.vertex_shader);
    bfShaderProgram_addModule(s_RenderData.program, s_RenderData.fragment_shader);

    bfShaderProgram_addImageSampler(s_RenderData.program, "u_Texture", 0, 0, 1, BIFROST_SHADER_STAGE_FRAGMENT);
    bfShaderProgram_addUniformBuffer(s_RenderData.program, "u_Projection", 0, 1, 1, BIFROST_SHADER_STAGE_VERTEX);

    bfShaderProgram_compile(s_RenderData.program);

    // Renderer Setup: Font Texture
    unsigned char* pixels;
    int            width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    bfTextureCreateParams      create_texture = bfTextureCreateParams_init2D(width, height, BIFROST_IMAGE_FORMAT_R8G8B8A8_UNORM);
    bfTextureSamplerProperties sampler        = bfTextureSamplerProperties_init(BIFROST_SFM_NEAREST, BIFROST_SAM_REPEAT);

    create_texture.generate_mipmaps = bfFalse;

    s_RenderData.font = bfGfxDevice_newTexture(device, &create_texture);
    bfTexture_loadData(s_RenderData.font, reinterpret_cast<char*>(pixels), width * height * bytes_per_pixel * sizeof(pixels[0]));
    bfTexture_setSampler(s_RenderData.font, &sampler);

    // Renderer Setup: Descriptor Set

    for (int i = 0; i < 2; ++i)
    {
      s_RenderData.buffers[i].create(device, s_RenderData.program, s_RenderData.font);
    }
  }

  void onEvent(const Event& evt)
  {
    ImGuiIO& io = ImGui::GetIO();

    switch (evt.type)
    {
      case EventType::ON_WINDOW_RESIZE:
      {
        io.DisplaySize = ImVec2(float(evt.window.width), float(evt.window.height));
        break;
      }
      case EventType::ON_MOUSE_MOVE:
      {
        io.MousePos = ImVec2(static_cast<float>(evt.mouse.x), static_cast<float>(evt.mouse.y));
        break;
      }
      case EventType::ON_MOUSE_UP:
      case EventType::ON_MOUSE_DOWN:
      {
        const bool is_down = evt.type == EventType::ON_MOUSE_DOWN;

        switch (evt.mouse.target_button)
        {
          case MouseEvent::BUTTON_LEFT:
          {
            io.MouseDown[0] = is_down;
            break;
          }
          case MouseEvent::BUTTON_MIDDLE:
          {
            io.MouseDown[2] = is_down;
            break;
          }
          case MouseEvent::BUTTON_RIGHT:
          {
            io.MouseDown[1] = is_down;
            break;
          }
          default:
            break;
        }

        break;
      }
      case EventType::ON_KEY_UP:
      case EventType::ON_KEY_DOWN:
      {
        const auto key = evt.keyboard.key;

        io.KeysDown[key] = (evt.type == EventType::ON_KEY_DOWN);

        io.KeyCtrl  = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt   = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
        break;
      }
      case EventType::ON_KEY_INPUT:
      {
        io.AddInputCharacter(evt.keyboard.codepoint);
        break;
      }
      case EventType::ON_SCROLL_WHEEL:
      {
        io.MouseWheelH += static_cast<float>(evt.scroll_wheel.x);
        io.MouseWheel += static_cast<float>(evt.scroll_wheel.y);
        break;
      }
      default:
        break;
    }
  }

  void beginFrame(bfTextureHandle surface, float window_width, float window_height, float current_time)
  {
    ImGuiIO&    io                 = ImGui::GetIO();
    const float framebuffer_width  = float(bfTexture_width(surface));
    const float framebuffer_height = float(bfTexture_height(surface));

    io.DisplaySize = ImVec2(window_width, window_height);

    if (window_width > 0.0f && window_height > 0.0f)
    {
      io.DisplayFramebufferScale = ImVec2(framebuffer_width / window_width, framebuffer_height / window_height);
    }

    io.DeltaTime = s_Time > 0.0 ? current_time - s_Time : 1.0f / 60.0f;
    s_Time       = current_time;

    ImGui::NewFrame();

    if (ImGui::Begin("Test GUI"))
    {
      static float t         = 0.5f;
      static char  text[500] = "Hello";

      ImGui::DragFloat("Hmmm", &t);
      ImGui::InputText("Text uwu", text, sizeof(text) - 1);
    }
    ImGui::End();
  }

  static void frameResetState(bfGfxCommandListHandle command_list, UIFrameData& frame, int fb_width, int fb_height)
  {
    uint64_t buffer_offset = 0;

    bfGfxCmdList_setDynamicStates(command_list, BIFROST_PIPELINE_DYNAMIC_VIEWPORT | BIFROST_PIPELINE_DYNAMIC_SCISSOR);
    bfGfxCmdList_bindVertexDesc(command_list, s_RenderData.vertex_layout);
    bfGfxCmdList_bindVertexBuffers(command_list, 0, &frame.vertex_buffer, 1, &buffer_offset);
    bfGfxCmdList_bindIndexBuffer(command_list, frame.index_buffer, 0, sizeof(ImDrawIdx) == 2 ? BIFROST_INDEX_TYPE_UINT16 : BIFROST_INDEX_TYPE_UINT32);
    bfGfxCmdList_bindProgram(command_list, s_RenderData.program);
    bfGfxCmdList_bindDescriptorSets(command_list, 0, &frame.descriptor_set, 1);
    bfGfxCmdList_setViewport(command_list, 0.0f, 0.0f, float(fb_width), float(fb_height), nullptr);
  }

  static void frameDraw(ImDrawData* draw_data, bfGfxCommandListHandle command_list, int window)
  {
    if (draw_data)
    {
      ImGuiIO&   io        = ImGui::GetIO();
      const auto fb_width  = static_cast<int>(draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
      const auto fb_height = static_cast<int>(draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
      if (fb_width <= 0 || fb_height <= 0)
        return;

      const bfGfxFrameInfo info = bfGfxContext_getFrameInfo(s_RenderData.ctx, window);

      UIFrameData& frame = s_RenderData.buffers[info.frame_index];

      const size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
      const size_t index_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

      if (vertex_size == 0 || index_size == 0)
      {
        return;
      }

      frame.checkSizes(s_RenderData.device, vertex_size, index_size);

      ImDrawVert* vertex_buffer_ptr  = static_cast<ImDrawVert*>(bfBuffer_map(frame.vertex_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE));
      ImDrawIdx*  index_buffer_ptr   = static_cast<ImDrawIdx*>(bfBuffer_map(frame.index_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE));
      Mat4x4*     uniform_buffer_ptr = static_cast<Mat4x4*>(bfBuffer_map(frame.uniform_buffer, 0, BIFROST_BUFFER_WHOLE_SIZE));

      for (int i = 0; i < draw_data->CmdListsCount; ++i)
      {
        const ImDrawList* cmd_list = draw_data->CmdLists[i];

        std::memcpy(vertex_buffer_ptr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        std::memcpy(index_buffer_ptr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));

        vertex_buffer_ptr += cmd_list->VtxBuffer.Size;
        index_buffer_ptr += cmd_list->IdxBuffer.Size;
      }

      Mat4x4_ortho(uniform_buffer_ptr, 0.0f, float(fb_width), float(fb_height), 0.0f, 0.0f, 1.0f);

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

            if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
            {
              clip_rect.x = std::max(0.0f, clip_rect.x);
              clip_rect.y = std::max(0.0f, clip_rect.y);
              ;

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

  void endFrame(bfGfxCommandListHandle command_list)
  {
    ImGui::Render();
    frameDraw(ImGui::GetDrawData(), command_list, -1);
  }

  void shutdown()
  {
    bfGfxDevice_flush(s_RenderData.device);
    bfVertexLayout_delete(s_RenderData.vertex_layout);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.vertex_shader);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.fragment_shader);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.program);
    bfGfxDevice_release(s_RenderData.device, s_RenderData.font);

    for (int i = 0; i < 2; ++i)
    {
      s_RenderData.buffers[i].destroy(s_RenderData.device);
    }

    for (auto& cursor : s_MouseCursors)
    {
      glfwDestroyCursor(cursor);
      cursor = nullptr;
    }

    ImGui::DestroyContext(nullptr);
  }

  static const char* GLFW_ClipboardGet(void* user_data)
  {
    return glfwGetClipboardString(static_cast<GLFWwindow*>(user_data));
  }

  static void GLFW_ClipboardSet(void* user_data, const char* text)
  {
    glfwSetClipboardString(static_cast<GLFWwindow*>(user_data), text);
  }
}  // namespace bifrost::editor::imgui
