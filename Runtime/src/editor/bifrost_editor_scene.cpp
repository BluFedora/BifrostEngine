#include "bf/editor/bifrost_editor_scene.hpp"

#include "bf/StlAllocator.hpp"  // StlAllocator
#include "bf/asset_io/bf_model_loader.hpp"
#include "bf/bf_ui.hpp"
#include "bf/graphics/bifrost_component_renderer.hpp"

#include <ImGuizmo/ImGuizmo.h>

#include <list>

#include "bf/text/bf_text.hpp"

extern bf::PainterFont* TEST_FONT;

namespace bf::editor
{
  static AABB s_XXXSelectedAABB;

  static const float        k_SceneViewPadding = 1.0f;
  static const float        k_InvalidMousePos  = -1.0f;
  const ImGuizmo::OPERATION gizmo_op           = ImGuizmo::TRANSLATE;

  SceneView::SceneView() :
    m_SceneViewViewport{},
    m_IsSceneViewHovered{false},
    m_Camera{nullptr},
    m_OldMousePos{k_InvalidMousePos, k_InvalidMousePos},
    m_MousePos{k_InvalidMousePos, k_InvalidMousePos},
    m_IsDraggingMouse{false},
    m_MouseLookSpeed{0.01f},
    m_Editor{nullptr},
    m_OldWindowPadding{}
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
    m_OldWindowPadding = ImGui::GetStyle().WindowPadding;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(k_SceneViewPadding, k_SceneViewPadding));

    if (m_Camera)
    {
      std::uint8_t view_flags = 0x0;

      if (isVisible())
      {
        view_flags |= RenderView::DO_DRAW;
      }

      m_Camera->flags = view_flags;
    }
  }

  void SceneView::onDrawGUI(EditorOverlay& editor)
  {
    static float Rounding = 5.0f;

    auto& engine       = editor.engine();
    auto& open_project = editor.currentlyOpenProject();

    if (open_project)
    {
      const auto scene = engine.currentScene();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, m_OldWindowPadding);

      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("Camera"))
        {
          ImGui::DragFloat("Rounding", &Rounding, 1.0f, k_Epsilon, 100.0f);

          ImGui::DragFloat3("Ambient Color", &engine.renderer().AmbientColor.x);

          if (m_Camera)
          {
            if (ImGui::DragFloat3("Position", &m_Camera->cpu_camera.position.x))
            {
              Camera_setViewModified(&m_Camera->cpu_camera);
            }
          }

          if (scene)
          {
            if (ImGui::Checkbox("SceneDebugDraw", &scene->m_DoDebugDraw))
            {
            }
          }

          ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
      }

      ImGui::PopStyleVar(1);

      const auto           content_area  = ImGui::GetContentRegionAvail();
      auto* const          window_draw   = ImGui::GetWindowDrawList();
      const auto           draw_region   = Rect2i(0, 0, int(content_area.x), int(content_area.y));
      const ImVec2         window_pos    = ImGui::GetWindowPos();
      const ImVec2         cursor_offset = ImGui::GetCursorPos();
      const ImVec2         full_offset   = window_pos + cursor_offset;
      const ImVec2         position_min  = ImVec2{float(draw_region.left()), float(draw_region.top())} + full_offset;
      const ImVec2         position_max  = ImVec2{float(draw_region.right()), float(draw_region.bottom())} + full_offset;
      ImGuiViewport* const viewport      = ImGui::GetWindowViewport();

      m_IsSceneViewHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_None);

      m_SceneViewViewport.setLeft(int(position_min.x - viewport->Pos.x));
      m_SceneViewViewport.setTop(int(position_min.y - viewport->Pos.y));
      m_SceneViewViewport.setRight(int(position_max.x - viewport->Pos.x));
      m_SceneViewViewport.setBottom(int(position_max.y - viewport->Pos.y));

      {
        Vector2f scene_offset = {float(m_SceneViewViewport.left()), float(m_SceneViewViewport.top())};
        ImGuiIO& io           = ImGui::GetIO();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
          ImGuiViewport* const main_vp = ImGui::GetMainViewport();

          scene_offset += Vector2f(main_vp->Pos.x, main_vp->Pos.y);
        }

        ImGuizmo::SetRect(scene_offset.x, scene_offset.y, float(m_SceneViewViewport.width()), float(m_SceneViewViewport.height()));
      }

      if (!m_Camera)
      {
        m_Editor = &editor;
        m_Camera = engine.borrowCamera({int(content_area.x), int(content_area.y)});
      }

      if (!m_Camera)
      {
        return;
      }

      auto* const color_buffer = m_Camera->gpu_camera.composite_buffer;

      if (m_Camera->old_width != int(content_area.x) ||
          m_Camera->old_height != int(content_area.y))
      {
        engine.resizeCamera(m_Camera, std::max(int(content_area.x), 1), std::max(int(content_area.y), 1));
      }

#if 1
      window_draw->AddImage(
       color_buffer,
       position_min,
       position_max,
       ImVec2(0.0f, 0.0f),
       ImVec2(1.0f, 1.0f),
       0xFFFFFFFF);
#else
      window_draw->AddImageRounded(
       color_buffer,
       position_min,
       position_max,
       ImVec2(0.0f, 0.0f),
       ImVec2(1.0f, 1.0f),
       0xFFFFFFFF,
       Rounding,
       ImDrawCornerFlags_All);
#endif
    }
    else
    {
      static const char* s_StrNewProject  = " New  Project ";
      static const char* s_StrOpenProject = " Open Project ";

      const auto text_size  = ImGui::CalcTextSize(s_StrNewProject);
      auto       mid_screen = (ImGui::GetWindowSize() - text_size) * 0.5f;

      mid_screen.x = std::round(mid_screen.x);
      mid_screen.y = std::round(mid_screen.y);

      ImGui::SetCursorPos(mid_screen);

      editor.buttonAction({&editor}, "File.New.Project", s_StrNewProject);

      ImGui::SetCursorPosX(mid_screen.x);

      editor.buttonAction({&editor}, "File.Open.Project", s_StrOpenProject);
    }

    if (isFocused())
    {
      auto&       dbg_rendrer = engine.debugDraw();
      const auto& selection   = editor.selection();

      if (selection.hasType<Entity*>())
      {
        ImGuizmo::SetDrawlist();

        const bool was_using_gizmo = ImGuizmo::IsUsing();

        Mat4x4 delta_mat;

        selection.firstOfType<Entity*>([this, &delta_mat](Entity* entity) {
          auto&  cam              = m_Camera->cpu_camera;
          auto&  entity_transform = entity->transform();
          Mat4x4 entity_mat       = entity_transform.world_transform;
          Mat4x4 projection_ogl;

          bfCamera_openGLProjection(&cam, &projection_ogl);

          ImGuizmo::Manipulate(cam.view_cache.data, projection_ogl.data, gizmo_op, ImGuizmo::WORLD, entity_mat.data, delta_mat.data, nullptr);
        });

        const bool is_using_gizmo = ImGuizmo::IsUsing();

        if (was_using_gizmo != is_using_gizmo)
        {
          if (is_using_gizmo)
          {
            bfLogPrint("Started using the Gizmo");
          }
          else
          {
            bfLogPrint("Stopped using the Gizmo");
          }
        }

        selection.forEachOfType<Entity*>([this, &dbg_rendrer, &delta_mat](Entity* entity) {
          auto&  entity_transform = entity->transform();
          Mat4x4 entity_mat       = entity_transform.world_transform;

          dbg_rendrer.addAABB(
           entity_transform.world_position,
           Vector3f{0.1f},
           bfColor4u_fromUint32(BIFROST_COLOR_DEEPPINK),
           0.0f,
           true);

          Mat4x4_mult(&delta_mat, &entity_mat, &entity_mat);

          auto* const transform_parent = entity_transform.parent;

          if (transform_parent)
          {
            Mat4x4 inv_parent_world = transform_parent->world_transform;

            if (Mat4x4_inverse(&inv_parent_world, &inv_parent_world))
            {
              Mat4x4_mult(&inv_parent_world, &entity_mat, &entity_mat);
            }
          }

          Vec3f translation;
          Vec3f rotation;
          Vec3f scale;

          ImGuizmo::DecomposeMatrixToComponents(entity_mat.data, &translation.x, &rotation.x, &scale.x);

          switch (gizmo_op)
          {
            case ImGuizmo::TRANSLATE:
            {
              translation.w = 1.0f;
              // ReSharper disable once CppSomeObjectMembersMightNotBeInitialized
              entity_transform.local_position = translation;
              break;
            }
            case ImGuizmo::ROTATE:
              // ReSharper disable once CppObjectMemberMightNotBeInitialized
              entity_transform.local_rotation = bfQuaternionf_fromEulerDeg(-rotation.y, -rotation.z, rotation.x);
              break;
            case ImGuizmo::SCALE:
              scale.w = 0.0f;
              // ReSharper disable once CppSomeObjectMembersMightNotBeInitialized
              entity_transform.local_scale = scale;
              break;
            case ImGuizmo::BOUNDS:
            default:
              break;
          }

          bfTransform_flushChanges(&entity_transform);
        });
      }
    }
  }

  void SceneView::onPostDrawGUI(EditorOverlay& editor)
  {
    ImGui::PopStyleVar(1);
  }

  // TODO(SR): This should be in a public header along with other std:: data structure typedefs.
  template<typename T>
  using StdList = std::list<T, StlAllocator<T>>;

  using ClickResult = std::pair<const BVHNode*, bfRaycastAABBResult>;

  void SceneView::onEvent(EditorOverlay& editor, Event& event)
  {
    if (!m_Camera)
    {
      return;
    }

    ImGuiIO& io        = ImGui::GetIO();
    auto&    mouse_evt = event.mouse;

    if (!ImGuizmo::IsUsing() && !isGizmoOver(editor.selection()))
    {
      if (event.type == BIFROST_EVT_ON_MOUSE_DOWN || event.type == BIFROST_EVT_ON_MOUSE_UP)
      {
        m_OldMousePos = {k_InvalidMousePos, k_InvalidMousePos};

        if (event.type == BIFROST_EVT_ON_MOUSE_DOWN)
        {
          if (isPointOverSceneView({mouse_evt.x, mouse_evt.y}))
          {
            if (mouse_evt.target_button == BIFROST_BUTTON_LEFT)
            {
              auto&      engine = editor.engine();
              const auto scene  = engine.currentScene();

              if (scene)
              {
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

                StdList<ClickResult> clicked_nodes{engine.tempMemory()};

                scene->bvh().traverseConditionally([&ray, &clicked_nodes](const BVHNode& node) -> bool {
                  Vec3f aabb_min;
                  Vec3f aabb_max;

                  memcpy(&aabb_min, &node.bounds.min, sizeof(float) * 3);
                  memcpy(&aabb_max, &node.bounds.max, sizeof(float) * 3);

                  const auto ray_cast_result = bfRay3D_intersectsAABB(&ray, aabb_min, aabb_max);

                  if (bvh_node::isLeaf(node))
                  {
                    const Entity* const entity = (Entity*)node.user_data;

                    // The Entity must have a visual component
                    if (entity->has<MeshRenderer>() || entity->has<SkinnedMeshRenderer>() || entity->has<SpriteRenderer>() || entity->has<Light>())
                    {
                      if (ray_cast_result.did_hit && (ray_cast_result.min_time >= 0.0f || ray_cast_result.max_time >= 0.0f))
                      {
                        clicked_nodes.emplace_back(&node, ray_cast_result);
                        return false;
                      }
                    }
                  }

                  return ray_cast_result.did_hit;
                });

                auto& selection = editor.selection();

                if (!editor.isShiftDown())
                {
                  selection.clear();
                }

                if (!clicked_nodes.empty())
                {
                  clicked_nodes.sort([](const ClickResult& a, const ClickResult& b) {
                    return a.second.min_time < b.second.min_time;
                  });

                  const ClickResult best_click = clicked_nodes.front();

                  s_XXXSelectedAABB = best_click.first->bounds;

                  selection.toggle(static_cast<Entity*>(best_click.first->user_data));
                }
              }
            }
            else if (mouse_evt.target_button == BIFROST_BUTTON_MIDDLE)
            {
              m_IsDraggingMouse = true;
            }
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

        if (m_IsDraggingMouse && mouse_evt.button_state & BIFROST_BUTTON_MIDDLE)
        {
          const float newx = float(mouse_evt.x);
          const float newy = float(mouse_evt.y);

          if (m_OldMousePos.x == k_InvalidMousePos) { m_OldMousePos.x = newx; }
          if (m_OldMousePos.y == k_InvalidMousePos) { m_OldMousePos.y = newy; }

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
    }

    if (event.isKeyEvent())
    {
      if (event.type == BIFROST_EVT_ON_KEY_DOWN)
      {
        if (event.keyboard.modifiers & BIFROST_KEY_FLAG_CONTROL && event.keyboard.key == BIFROST_KEY_D)
        {
          auto& engine    = editor.engine();
          auto& selection = editor.selection();

          if (selection.hasType<Entity*>())
          {
            Array<Entity*> clones{engine.tempMemory()};

            selection.forEachOfType<Entity*>([&clones](Entity* const copied_entity) {
              clones.push(copied_entity->clone());
            });

            selection.clear();
            for (Entity* clone : clones)
            {
              selection.select(clone);
            }
          }
        }
      }
    }

    if (event.isMouseEvent())
    {
      const Vector2i ui_local_mouse = Vector2i(int(event.mouse.x), int(event.mouse.y)) - m_SceneViewViewport.topLeft();
      bfEvent        ui_event       = event;
      ui_event.mouse.x              = ui_local_mouse.x;
      ui_event.mouse.y              = ui_local_mouse.y;

      UI::PumpEvents(&ui_event);
    }

    if ((m_IsDraggingMouse || isGizmoOver(editor.selection())) && event.isMouseEvent())
    {
      event.accept();
    }
  }

  void SceneView::onUpdate(EditorOverlay& editor, float dt)
  {
    if (m_Camera)
    {
      auto&      engine   = editor.engine();
      auto&      dbg_draw = engine.debugDraw();
      const auto scene    = engine.currentScene();

      if (scene)
      {
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

        if (m_IsSceneViewHovered)
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
          /*
          scene->bvh().traverseConditionally([&ray, &dbg_draw](const BVHNode& node) {
            static bfColor4u k_DebugColors[] =
             {
              {255, 0, 0, 255},
              {0, 255, 0, 255},
              {0, 0, 255, 255},
              {255, 0, 255, 255},
              {255, 255, 0, 255},
              {0, 255, 255, 255},
              {255, 255, 255, 255},
             };

            const bfColor4u color = k_DebugColors[node.depth % std::size(k_DebugColors)];

            
            Vec3f aabb_min;
            Vec3f aabb_max;

            memcpy(&aabb_min, &node.bounds.min, sizeof(float) * 3);
            memcpy(&aabb_max, &node.bounds.max, sizeof(float) * 3);

            const auto ray_cast_result = bfRay3D_intersectsAABB(&ray, aabb_min, aabb_max);

            if (ray_cast_result.did_hit)
            {
              dbg_draw.addAABB(
               node.bounds.center(),
               node.bounds.dimensions(),
               color);  
            }

            return ray_cast_result.did_hit;
          });
          */
#if 0
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
#endif
        }

        //
        // Mesh Bounds Test
        //

        scene->bvh().traverseConditionally([&ray, &dbg_draw](const BVHNode& node) {
          Vec3f aabb_min;
          Vec3f aabb_max;

          memcpy(&aabb_min, &node.bounds.min, sizeof(float) * 3);
          memcpy(&aabb_max, &node.bounds.max, sizeof(float) * 3);

          const auto ray_cast_result = bfRay3D_intersectsAABB(&ray, aabb_min, aabb_max);

          if (ray_cast_result.did_hit && bvh_node::isLeaf(node))
          {
            Entity*             entity   = (Entity*)node.user_data;
            const MeshRenderer* renderer = entity->get<MeshRenderer>();

            if (renderer && renderer->m_Model)
            {
              const auto& transform = &entity->transform();
              auto&       model     = *renderer->m_Model;

              bfRay3D local_ray = ray;
              Mat4x4_multVec(&transform->inv_world_transform, &local_ray.origin, &local_ray.origin);
              Mat4x4_multVec(&transform->inv_world_transform, &local_ray.direction, &local_ray.direction);

              const auto num_indices = model.m_Triangles.size();

              for (AssetIndexType face = 0u; face < num_indices; face += 3)
              {
#if 0
                auto v0 = model.m_Vertices[model.m_Triangles[face + 0u]].pos;
                auto v1 = model.m_Vertices[model.m_Triangles[face + 1u]].pos;
                auto v2 = model.m_Vertices[model.m_Triangles[face + 2u]].pos;
                const auto raycast = bfRay3D_intersectsTriangle(&local_ray, v2, v0, v1);

                if (raycast.did_hit)
                {
                  Mat4x4_multVec(&transform->world_transform, &v0, &v0);
                  Mat4x4_multVec(&transform->world_transform, &v1, &v1);
                  Mat4x4_multVec(&transform->world_transform, &v2, &v2);

                  dbg_draw.addLine(v0, v1, bfColor4u_fromUint32(BIFROST_COLOR_SILVER));
                  dbg_draw.addLine(v1, v2, bfColor4u_fromUint32(BIFROST_COLOR_SILVER));
                  dbg_draw.addLine(v2, v0, bfColor4u_fromUint32(BIFROST_COLOR_SILVER));
                }
#endif
              }
            }
          }

          return ray_cast_result.did_hit;
        });

        {
          for (const SpriteRenderer& sprite : scene->components<SpriteRenderer>())
          {
            // Ray Picking Code

            const auto&                transform               = sprite.owner().transform();
            const Vector3f             center_position         = transform.world_position;
            const Vector3f             forward                 = bfQuaternionf_forward(&transform.world_rotation);
            const float                signed_dist_from_origin = bfPlane_dot(bfPlane_make(forward, 0.0f), center_position);
            const bfRaycastPlaneResult sprite_ray_cast         = bfRay3D_intersectsPlane(&ray, bfPlane_make(forward, signed_dist_from_origin));

            if (sprite_ray_cast.did_hit)
            {
              const Vector2f half_local_x_axis = Vector2f{sprite.m_Size.x, 0.0f} * 0.5f;
              const Vector2f half_local_y_axis = Vector2f{0.0f, sprite.m_Size.y} * 0.5f;
              const Vector3f hit_point         = Vector3f(ray.origin) + Vector3f(ray.direction) * sprite_ray_cast.time;
              Vector3f       local_hit_point_v3;

              Mat4x4_multVec(&transform.inv_world_transform, &hit_point, &local_hit_point_v3);

              const Vector2f local_hit_point = {local_hit_point_v3.x, local_hit_point_v3.y};

              if (math::isInbetween(0.0f, vec::inverseLerp(-half_local_x_axis, half_local_x_axis, local_hit_point), 1.0f) &&
                  math::isInbetween(0.0f, vec::inverseLerp(-half_local_y_axis, half_local_y_axis, local_hit_point), 1.0f))
              {
                dbg_draw.addAABB(hit_point, Vector3f{0.1f}, bfColor4u_fromUint32(BIFROST_COLOR_GOLD));
              }
            }

            // Calculating the Bounds

            //const float    half_thickness      = 0.05f;  // Needed since an aabb of zero volume is a bad idea.
            //const Vector3f half_extents        = Vector3f{sprite.m_Size.x, sprite.m_Size.y, half_thickness, 0.0f} * 0.5f;
            //const AABB     object_space_bounds = {-half_extents, half_extents};
            //const AABB     world_space_bounds  = aabb::transform(object_space_bounds, transform.world_transform);
            //
            //dbg_draw.addAABB(world_space_bounds.center(), world_space_bounds.dimensions(), bfColor4u_fromUint32(BIFROST_COLOR_DEEPPINK));

            Vector3f ss_pos = bfCamera_worldToScreenSpace(&m_Camera->cpu_camera, center_position);

            if (math::isInbetween(0.0f, ss_pos.z, 1.0f))
            {
              auto& gfx2D = engine.gfx2D();

              ss_pos.x = bfMathRemapf(-1.0f, 1.0f, 0.0f, float(m_Camera->cpu_camera.width), ss_pos.x);
              ss_pos.y = bfMathRemapf(-1.0f, 1.0f, 0.0f, float(m_Camera->cpu_camera.height), ss_pos.y);

              Brush* const b    = gfx2D.makeBrush({0.0f, 1.0f, 0.0f, 1.0f});
              Brush* const font = gfx2D.makeBrush(TEST_FONT, {1.0f, 0.0f, 1.0f, 1.0f});

              const auto baseline_info = fontBaselineInfo(TEST_FONT->font);

              auto rect = engine.gfx2D().fillRect(b, AxisQuad::make({ss_pos.x, ss_pos.y}));
              auto text = engine.gfx2D().text(font, {ss_pos.x, ss_pos.y}, "Some In Scene Text My Guy I");

              rect->rect.x_axis = {text->bounds_size.x, 0.0f};
              rect->rect.y_axis = {0.0f, text->bounds_size.y};
              rect->rect.position.y -= baseline_info.ascent_px;
            }
          }
        }

#if 0
        for (const MeshRenderer& renderer : scene->components<MeshRenderer>())
        {
          if (renderer.m_Model)
          {
            auto&       model     = *renderer.m_Model;
            const auto& transform = &renderer.owner().transform();
            const auto  bounds    = aabb::transform(model.m_ObjectSpaceBounds, transform->world_transform);
            const auto  size      = bounds.dimensions();

            engine.debugDraw().addAABB(bounds.center(), size, bfColor4u_fromUint32(BIFROST_COLOR_INDIGO));

            if (m_IsSceneViewHovered)
            {
              for (AssetIndexType face = 0u; face < model.m_Triangles.size(); face += 3)
              {
                auto v0 = model.m_Vertices[model.m_Triangles[face + 0u]].pos;
                auto v1 = model.m_Vertices[model.m_Triangles[face + 1u]].pos;
                auto v2 = model.m_Vertices[model.m_Triangles[face + 2u]].pos;

                bfRay3D local_ray = ray;

#if 1
                Mat4x4_multVec(&transform->inv_world_transform, &local_ray.origin, &local_ray.origin);
                Mat4x4_multVec(&transform->inv_world_transform, &local_ray.direction, &local_ray.direction);
#else
                Mat4x4_multVec(&transform->world_transform, &v0, &v0);
                Mat4x4_multVec(&transform->world_transform, &v1, &v1);
                Mat4x4_multVec(&transform->world_transform, &v2, &v2);
#endif

                const auto raycast = bfRay3D_intersectsTriangle(&local_ray, v2, v0, v1);

                if (raycast.did_hit)
                {
                  Mat4x4_multVec(&transform->world_transform, &v0, &v0);
                  Mat4x4_multVec(&transform->world_transform, &v1, &v1);
                  Mat4x4_multVec(&transform->world_transform, &v2, &v2);

                  dbg_draw.addLine(v0, v1, bfColor4u_fromUint32(BIFROST_COLOR_SILVER));
                  dbg_draw.addLine(v1, v2, bfColor4u_fromUint32(BIFROST_COLOR_SILVER));
                  dbg_draw.addLine(v2, v0, bfColor4u_fromUint32(BIFROST_COLOR_SILVER));
                }
              }
            }
          }
        }
#endif

        // Light Icon Rendering

        /* const ??? */ auto& lights   = scene->components<Light>();
        const auto&           renderer = engine.rendererSys();

        Renderable2DPrimitive primitive_proto;

        const Vector3f camera_right = m_Camera->cpu_camera._right;
        const Vector3f camera_up    = m_Camera->cpu_camera.up;

        Mat4x4 transform;
        Mat4x4_identity(&transform);

        *Mat4x4_get(&transform, 0, 0) = camera_right.x;
        *Mat4x4_get(&transform, 0, 1) = camera_right.y;
        *Mat4x4_get(&transform, 0, 2) = camera_right.z;

        *Mat4x4_get(&transform, 1, 0) = camera_up.x;
        *Mat4x4_get(&transform, 1, 1) = camera_up.y;
        *Mat4x4_get(&transform, 1, 2) = camera_up.z;

        primitive_proto.material  = &editor.m_SceneLightMaterial;
        primitive_proto.size      = {0.5f, 0.5f};
        primitive_proto.uv_rect   = {0.0f, 0.0f, 1.0f, 1.0};
        primitive_proto.color     = bfColor4u_fromUint32(0xFFFFFFFF);
        primitive_proto.transform = transform;

        for (const Light& light : lights)
        {
          const auto&    light_transform = light.owner().transform();
          const Vector3f pos_to_cam      = Vector3f(m_Camera->cpu_camera.position) - Vector3f(light_transform.world_position);

          // Being right on top of the light is kinda jank so move it towards the cam a tad bit.
          primitive_proto.origin = Vector3f(light_transform.world_position) + vec::normalized(pos_to_cam) * 0.2f;

          renderer.pushSprite(primitive_proto);
        }
      }

      // Highlight Selected Objects
#if 0
      const auto& selection = editor.selection();

      selection.forEachOfType<Entity*>([this, &dbg_rendrer](Entity* entity) {
        auto&       entity_transform = entity->transform();
        const AABB& bounds           = entity_transform;

        dbg_rendrer.addAABB(
         entity_transform.world_position,
         bounds.dimensions(),
         bfColor4u_fromUint32(BIFROST_COLOR_LAVENDER),
         0.0f,
         true);
      });
#endif

      dbg_draw.addAABB(
       s_XXXSelectedAABB.center(),
       s_XXXSelectedAABB.dimensions(),
       bfColor4u_fromUint32(BIFROST_COLOR_LAVENDER),
       0.0f,
       true);

      updateCameraMovement(editor, dt);
    }
  }

  bool SceneView::isGizmoOver(const Selection& selection) const
  {
    return selection.hasType<Entity*>() && ImGuizmo::IsOver();
  }

  void SceneView::updateCameraMovement(EditorOverlay& editor, float dt) const
  {
    if (isFocused() && !editor.isControlDown())
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
}  // namespace bf::editor
