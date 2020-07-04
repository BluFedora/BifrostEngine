#include "bifrost/editor/bifrost_editor_scene.hpp"
#include "bifrost/memory/bifrost_stl_allocator.hpp"

namespace bifrost::editor
{
  static const float k_SceneViewPadding = 1.0f;
  static const float k_InvalidMousePos  = -1.0f;

  SceneView::SceneView() :
    m_SceneViewViewport{},
    m_IsSceneViewHovered{false},
    m_Camera{nullptr},
    m_OldMousePos{k_InvalidMousePos, k_InvalidMousePos},
    m_MousePos{k_InvalidMousePos, k_InvalidMousePos},
    m_IsDraggingMouse{false},
    m_MouseLookSpeed{0.01f},
    m_Editor{nullptr}
  {
  }

  SceneView::~SceneView()
  {
    if (m_Camera)
    {
      m_Editor->engine().returnCamera(m_Camera);
    }
  }

  void SceneView::onPreDrawGUI(EditorOverlay& editor)
  {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_SceneViewPadding, k_SceneViewPadding));
  }

  void SceneView::onDrawGUI(EditorOverlay& editor)
  {
    static float Rounding = 5.0f;

    auto& engine       = editor.engine();
    auto& open_project = editor.currentlyOpenProject();

    if (!m_Camera)
    {
      m_Editor = &editor;
      m_Camera = engine.borrowCamera({1, 1});
    }

    if (!m_Camera)
    {
      ImGui::PopStyleVar(3);
      return;
    }

    if (open_project)
    {
      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("Camera"))
        {
          ImGui::DragFloat("Rounding", &Rounding, 1.0f, k_Epsilon, 100.0f);

          ImGui::DragFloat3("Ambient Color", &engine.renderer().AmbientColor.x);

          if (ImGui::DragFloat3("Position", &m_Camera->cpu_camera.position.x))
          {
            Camera_setViewModified(&m_Camera->cpu_camera);
          }

          ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
      }

      const auto           color_buffer        = m_Camera->gpu_camera.composite_buffer;
      const auto           color_buffer_width  = bfTexture_width(color_buffer);
      const auto           color_buffer_height = bfTexture_height(color_buffer);
      const auto           content_area        = ImGui::GetContentRegionAvail();
      const auto           draw_region         = rect::aspectRatioDrawRegion(color_buffer_width, color_buffer_height, uint32_t(content_area.x), uint32_t(content_area.y));
      auto* const          window_draw         = ImGui::GetWindowDrawList();
      const ImVec2         window_pos          = ImGui::GetWindowPos();
      const ImVec2         cursor_offset       = ImGui::GetCursorPos();
      const ImVec2         full_offset         = window_pos + cursor_offset;
      const ImVec2         position_min        = ImVec2{float(draw_region.left()), float(draw_region.top())} + full_offset;
      const ImVec2         position_max        = ImVec2{float(draw_region.right()), float(draw_region.bottom())} + full_offset;
      ImGuiViewport* const viewport            = ImGui::GetWindowViewport();

      m_IsSceneViewHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

      m_SceneViewViewport.setLeft(int(position_min.x - viewport->Pos.x));
      m_SceneViewViewport.setTop(int(position_min.y - viewport->Pos.y));
      m_SceneViewViewport.setRight(int(position_max.x - viewport->Pos.x));
      m_SceneViewViewport.setBottom(int(position_max.y - viewport->Pos.y));

      if (m_Camera->old_width != (int)content_area.x || m_Camera->old_height != (int)content_area.y)
      {
        engine.resizeCamera(m_Camera, std::max(int(content_area.x), 1), std::max(int(content_area.y), 1));
      }

      window_draw->AddImageRounded(
       color_buffer,
       position_min,
       position_max,
       ImVec2(0.0f, 0.0f),
       ImVec2(1.0f, 1.0f),
       0xFFFFFFFF,
       Rounding,
       ImDrawCornerFlags_All);
    }
    else
    {
      static const char* s_StrNoProjectOpen = "No Project Open";
      static const char* s_StrNewProject    = "  New Project  ";
      static const char* s_StrOpenProject   = "  Open Project ";

      const auto text_size  = ImGui::CalcTextSize(s_StrNoProjectOpen);
      const auto mid_screen = (ImGui::GetWindowSize() - text_size) * 0.5f;

      ImGui::SetCursorPos(
       (ImGui::GetWindowSize() - text_size) * 0.5f);

      ImGui::Text("%s", s_StrNoProjectOpen);

      ImGui::SetCursorPosX(mid_screen.x);

      editor.buttonAction({&editor}, "File.New.Project", s_StrNewProject);

      ImGui::SetCursorPosX(mid_screen.x);

      editor.buttonAction({&editor}, "File.Open.Project", s_StrOpenProject);
    }
  }

  void SceneView::onPostDrawGUI(EditorOverlay& editor)
  {
    ImGui::PopStyleVar(3);
  }

  // TODO(SR): This should be in a public header along with other std:: data structure typedefs.
  template<typename T>
  using StdList = std::list<T, StlAllocator<T>>;

  using ClickResult = std::pair<const BVHNode*, bfRayCastResult>;

  void SceneView::onEvent(EditorOverlay& editor, Event& event)
  {
    auto& mouse_evt = event.mouse;

    if (event.type == BIFROST_EVT_ON_MOUSE_DOWN || event.type == BIFROST_EVT_ON_MOUSE_UP)
    {
      m_OldMousePos = {k_InvalidMousePos, k_InvalidMousePos};

      if (event.type == BIFROST_EVT_ON_MOUSE_DOWN)
      {
        if (isPointOverSceneView({mouse_evt.x, mouse_evt.y}))
        {
          auto&      engine = editor.engine();
          const auto scene  = engine.currentScene();

          if (scene)
          {
            // Ray cast Hit Begin
            auto&       io           = ImGui::GetIO();
            const auto& window_mouse = io.MousePos;
            Vector2i    local_mouse  = Vector2i(int(window_mouse.x), int(window_mouse.y)) - m_SceneViewViewport.topLeft();

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
              ImGuiViewport* const main_vp = ImGui::GetMainViewport();

              local_mouse -= Vector2i(int(main_vp->Pos.x), int(main_vp->Pos.y));
            }

            local_mouse.y = m_SceneViewViewport.height() - local_mouse.y;

            const bfRay3D ray = bfRay3D_make(
             m_Camera->cpu_camera.position,
             Camera_castRay(&m_Camera->cpu_camera, local_mouse, m_SceneViewViewport.size()));

            StdList<ClickResult> clicked_nodes{engine.tempMemoryNoFree()};

            scene->bvh().traverseConditionally([&ray, &clicked_nodes](const BVHNode& node) -> bool {
              Vec3f aabb_min;
              Vec3f aabb_max;

              memcpy(&aabb_min, &node.bounds.min, sizeof(float) * 3);
              memcpy(&aabb_max, &node.bounds.max, sizeof(float) * 3);

              const auto ray_cast_result = bfRay3D_intersectsAABB(&ray, aabb_min, aabb_max);

              if (ray_cast_result.did_hit && (ray_cast_result.min_time >= 0.0f || ray_cast_result.max_time >= 0) && bvh_node::isLeaf(node))
              {
                clicked_nodes.emplace_back(&node, ray_cast_result);
                return false;
              }

              return ray_cast_result.did_hit;
            });

            if (!clicked_nodes.empty())
            {
              clicked_nodes.sort([](const ClickResult& a, const ClickResult& b) {
                return a.second.min_time < b.second.min_time;
              });

              const ClickResult best_click = clicked_nodes.front();

              editor.select(static_cast<Entity*>(best_click.first->user_data));
            }
            else
            {
              editor.select(nullptr);
            }
          }
          // Ray cast Hit End

          m_IsDraggingMouse = true;
        }
      }
      else
      {
        m_IsDraggingMouse = false;
        event.accept();
      }
    }
    else if (event.type == BIFROST_EVT_ON_MOUSE_MOVE)
    {
      m_MousePos = {float(mouse_evt.x), float(mouse_evt.y)};

      if (m_IsDraggingMouse && mouse_evt.button_state & BIFROST_BUTTON_LEFT)
      {
        const float newx = float(mouse_evt.x);
        const float newy = float(mouse_evt.y);

        if (m_OldMousePos.x == k_InvalidMousePos)
        {
          m_OldMousePos.x = newx;
        }

        if (m_OldMousePos.y == k_InvalidMousePos)
        {
          m_OldMousePos.y = newy;
        }

        if (m_Camera)
        {
          Camera_mouse(
           &m_Camera->cpu_camera,
           (newx - m_OldMousePos.x) * m_MouseLookSpeed,
           (newy - m_OldMousePos.y) * -m_MouseLookSpeed);
        }

        m_OldMousePos.x = newx;
        m_OldMousePos.y = newy;
      }
    }

    if (m_IsDraggingMouse && event.isMouseEvent())
    {
      event.accept();
    }
  }

  void SceneView::onUpdate(EditorOverlay& editor, float dt)
  {
    if (m_Camera)
    {
      auto&       engine       = editor.engine();
      auto&       io           = ImGui::GetIO();
      const auto& window_mouse = io.MousePos;
      Vector2i    local_mouse  = Vector2i(int(window_mouse.x), int(window_mouse.y)) - m_SceneViewViewport.topLeft();

      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      {
        ImGuiViewport* const main_vp = ImGui::GetMainViewport();

        local_mouse -= Vector2i(int(main_vp->Pos.x), int(main_vp->Pos.y));
      }

      local_mouse.y = m_SceneViewViewport.height() - local_mouse.y;

      bfRay3D ray = bfRay3D_make(
       m_Camera->cpu_camera.position,
       Camera_castRay(&m_Camera->cpu_camera, local_mouse, m_SceneViewViewport.size()));

      const auto scene = engine.currentScene();

      if (scene)
      {
        ClickResult highlighted_node = {nullptr, {}};

        scene->bvh().traverseConditionally([&ray, &highlighted_node](const BVHNode& node) {
          Vec3f aabb_min;
          Vec3f aabb_max;

          memcpy(&aabb_min, &node.bounds.min, sizeof(float) * 3);
          memcpy(&aabb_max, &node.bounds.max, sizeof(float) * 3);

          const auto ray_cast_result = bfRay3D_intersectsAABB(&ray, aabb_min, aabb_max);

          if (ray_cast_result.did_hit && bvh_node::isLeaf(node) && ray_cast_result.min_time > 0.0f && (!highlighted_node.first || highlighted_node.second.min_time > ray_cast_result.min_time))
          {
            highlighted_node.first  = &node;
            highlighted_node.second = ray_cast_result;
          }

          return ray_cast_result.did_hit;
        });

        if (highlighted_node.first)
        {
          const BVHNode& node = *highlighted_node.first;

          engine.debugDraw().addAABB(
           (Vector3f(node.bounds.max[0], node.bounds.max[1], node.bounds.max[2]) + Vector3f(node.bounds.min[0], node.bounds.min[1], node.bounds.min[2])) * 0.5f,
           Vector3f(node.bounds.max[0], node.bounds.max[1], node.bounds.max[2]) - Vector3f(node.bounds.min[0], node.bounds.min[1], node.bounds.min[2]),
           bfColor4u_fromUint32(BIFROST_COLOR_FIREBRICK),
           0.0f,
           true);
        }
      }

      if (isFocused())
      {
        const auto camera_move_speed = (editor.isShiftDown() ? 2.2f : 1.0f) * dt;

        const std::tuple<int, void (*)(::BifrostCamera*, float), float> camera_controls[] =
         {
          {BIFROST_KEY_W, &Camera_moveForward, camera_move_speed},
          {BIFROST_KEY_A, &Camera_moveLeft, camera_move_speed},
          {BIFROST_KEY_S, &Camera_moveBackward, camera_move_speed},
          {BIFROST_KEY_D, &Camera_moveRight, camera_move_speed},
          {BIFROST_KEY_Q, &Camera_moveUp, camera_move_speed},
          {BIFROST_KEY_E, &Camera_moveDown, camera_move_speed},
          {BIFROST_KEY_R, &Camera_addPitch, -0.01f},
          {BIFROST_KEY_F, &Camera_addPitch, +0.01f},
          {BIFROST_KEY_H, &Camera_addYaw, +0.01f},
          {BIFROST_KEY_G, &Camera_addYaw, -0.01f},
         };

        for (const auto& control : camera_controls)
        {
          if (editor.isKeyDown(std::get<0>(control)))
          {
            std::get<1>(control)(&m_Camera->cpu_camera, std::get<2>(control));
          }
        }
      }
    }
  }
}  // namespace bifrost::editor
