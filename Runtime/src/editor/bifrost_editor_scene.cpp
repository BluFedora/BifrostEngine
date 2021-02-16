#include "bf/editor/bifrost_editor_scene.hpp"

#include "bf/StlAllocator.hpp"  // StlAllocator
#include "bf/asset_io/bf_model_loader.hpp"
#include "bf/bf_ui.hpp"
#include "bf/graphics/bifrost_component_renderer.hpp"
#include "bf/text/bf_text.hpp"

#include <ImGuizmo/ImGuizmo.h>

#include <list>

extern bf::PainterFont* TEST_FONT;

namespace bf::editor
{
  static const float        k_SceneViewPadding = 1.0f;
  static const float        k_LightIconSize    = 0.5f;
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
    auto&       engine       = editor.engine();
    const auto& open_project = editor.currentlyOpenProject();

    if (open_project)
    {
      const auto scene = engine.currentScene();

      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, m_OldWindowPadding);

      if (ImGui::BeginMenuBar())
      {
        if (ImGui::BeginMenu("Camera"))
        {
          // ImGui::DragFloat("Rounding", &Rounding, 1.0f, k_Epsilon, 100.0f);

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

    const auto& selection = editor.selection();

    if (selection.hasType<Entity*>())
    {
      auto& cam = m_Camera->cpu_camera;

      ImGuizmo::SetDrawlist();

      const bool was_using_gizmo = ImGuizmo::IsUsing();

      Mat4x4 delta_mat;

      Mat4x4 projection_ogl;
      bfCamera_openGLProjection(&cam, &projection_ogl);

      selection.forEachOfType<Entity*>([this, &delta_mat, &cam, &projection_ogl](Entity* entity) {
        auto&  entity_transform = entity->transform();
        Mat4x4 entity_mat       = entity_transform.world_transform;

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

      selection.forEachOfType<Entity*>([this, &delta_mat](Entity* entity) {
        auto&  entity_transform = entity->transform();
        Mat4x4 entity_mat       = entity_transform.world_transform;

        Mat4x4_mult(&delta_mat, &entity_mat, &entity_mat);

        auto* const transform_parent = entity_transform.parent;

        if (transform_parent)
        {
          Mat4x4_mult(&transform_parent->inv_world_transform, &entity_mat, &entity_mat);
        }

        Vec3f translation = {};
        Vec3f rotation    = {};
        Vec3f scale       = {};

        ImGuizmo::DecomposeMatrixToComponents(entity_mat.data, &translation.x, &rotation.x, &scale.x);

        switch (gizmo_op)
        {
          case ImGuizmo::TRANSLATE:
          {
            translation.w                   = 1.0f;
            entity_transform.local_position = translation;
            break;
          }
          case ImGuizmo::ROTATE:
            entity_transform.local_rotation = bfQuaternionf_fromEulerDeg(-rotation.y, -rotation.z, rotation.x);
            break;
          case ImGuizmo::SCALE:
            scale.w                      = 0.0f;
            entity_transform.local_scale = scale;
            break;
          case ImGuizmo::BOUNDS:
            break;
        }

        bfTransform_flushChanges(&entity_transform);
      });
    }
  }

  void SceneView::onPostDrawGUI(EditorOverlay& editor)
  {
    ImGui::PopStyleVar(1);
  }

  // TODO(SR): This should be in a public header along with other std:: data structure typedefs.
  template<typename T>
  using StdList = std::list<T, StlAllocator<T>>;

  struct HitTestResult
  {
    bool    did_hit = false;
    float   t       = INFINITY;
    Entity* entity;
  };

  static HitTestResult hitTestModel(const bfRay3D& ray, const ModelAsset& model, const Mat4x4& inv_world_transform)
  {
    bfRay3D local_ray = ray;
    Mat4x4_multVec(&inv_world_transform, &local_ray.origin, &local_ray.origin);
    Mat4x4_multVec(&inv_world_transform, &local_ray.direction, &local_ray.direction);

    HitTestResult result = {};

    for (AssetIndexType face = 0u; face < model.m_Triangles.size(); face += 3)
    {
      const auto& v0      = model.m_Vertices[model.m_Triangles[face + 0u]].pos;
      const auto& v1      = model.m_Vertices[model.m_Triangles[face + 1u]].pos;
      const auto& v2      = model.m_Vertices[model.m_Triangles[face + 2u]].pos;
      const auto  raycast = bfRay3D_intersectsTriangle(&local_ray, v0, v1, v2);

      if (raycast.did_hit && (!result.did_hit || raycast.time < result.t))
      {
        result.did_hit = true;
        result.t       = raycast.time;
      }
    }

    return result;
  }

  //
  // normal must be normalized.
  //
  static HitTestResult hitTestQuad(const bfRay3D& ray, const Vector3f center_position, const Vector3f normal, const Vector2f size, const Mat4x4& inv_world_transform)
  {
    const float                signed_dist_from_origin = bfPlane_dot(bfPlane_make(normal, 0.0f), center_position);
    const bfRaycastPlaneResult sprite_ray_cast         = bfRay3D_intersectsPlane(&ray, bfPlane_make(normal, signed_dist_from_origin));
    HitTestResult              result                  = {};

    if (sprite_ray_cast.did_hit)
    {
      const Vector3f hit_point         = Vector3f(ray.origin) + Vector3f(ray.direction) * sprite_ray_cast.time;
      const Vector2f half_local_x_axis = Vector2f{size.x, 0.0f} * 0.5f;
      const Vector2f half_local_y_axis = Vector2f{0.0f, size.y} * 0.5f;
      Vector3f       local_hit_point_v3;

      Mat4x4_multVec(&inv_world_transform, &hit_point, &local_hit_point_v3);

      const Vector2f local_hit_point = {local_hit_point_v3.x, local_hit_point_v3.y};
      const float    inv_lerp_x      = vec::inverseLerp(-half_local_x_axis, half_local_x_axis, local_hit_point);
      const float    inv_lerp_y      = vec::inverseLerp(-half_local_y_axis, half_local_y_axis, local_hit_point);

      if (math::isInbetween(0.0f, inv_lerp_x, 1.0f) && math::isInbetween(0.0f, inv_lerp_y, 1.0f))
      {
        result.did_hit = true;
        result.t       = sprite_ray_cast.time;
      }
    }

    return result;
  }

  static HitTestResult hitTestSprite(const bfRay3D& ray, const SpriteRenderer& sprite, const bfTransform& transform)
  {
    return hitTestQuad(
     ray,
     transform.world_position,
     bfQuaternionf_forward(&transform.world_rotation),
     sprite.m_Size,
     transform.inv_world_transform);
  }

  static HitTestResult hitTestEntity(const bfRay3D& ray, Entity* entity)
  {
    auto* const   mesh         = entity->get<MeshRenderer>();
    auto* const   skinned_mesh = entity->get<SkinnedMeshRenderer>();
    auto* const   sprite       = entity->get<SpriteRenderer>();
    HitTestResult result       = {};

    if (mesh && mesh->m_Model)
    {
      const HitTestResult mesh_hit = hitTestModel(ray, *mesh->m_Model, entity->transform().inv_world_transform);

      if (mesh_hit.did_hit && mesh_hit.t < result.t)
      {
        result = mesh_hit;
      }
    }

    if (skinned_mesh && skinned_mesh->m_Model)
    {
      const HitTestResult skinned_mesh_hit = hitTestModel(ray, *skinned_mesh->m_Model, entity->transform().inv_world_transform);

      if (skinned_mesh_hit.did_hit && skinned_mesh_hit.t < result.t)
      {
        result = skinned_mesh_hit;
      }
    }

    if (sprite)
    {
      const HitTestResult sprite_mesh_hit = hitTestSprite(ray, *sprite, entity->transform());

      if (sprite_mesh_hit.did_hit && sprite_mesh_hit.t < result.t)
      {
        result = sprite_mesh_hit;
      }
    }

    result.entity = entity;

    return result;
  }

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

                StdList<HitTestResult> clicked_nodes{engine.tempMemory()};

                scene->bvh().traverseConditionally([this, &ray, &clicked_nodes](const BVHNode& node) -> bool {
                  Vec3f aabb_min;
                  Vec3f aabb_max;

                  memcpy(&aabb_min, &node.bounds.min, sizeof(float) * 3);
                  memcpy(&aabb_max, &node.bounds.max, sizeof(float) * 3);

                  const auto ray_cast_result = bfRay3D_intersectsAABB(&ray, aabb_min, aabb_max);

                  if (bvh_node::isLeaf(node))
                  {
                    Entity* const entity = static_cast<Entity*>(node.user_data);

                    if (ray_cast_result.did_hit && (ray_cast_result.min_time >= 0.0f || ray_cast_result.max_time >= 0.0f))
                    {
                      const auto accurate_hit_test = hitTestEntity(ray, entity);

                      if (accurate_hit_test.did_hit)
                      {
                        clicked_nodes.emplace_back(accurate_hit_test);
                      }
                    }

                    return false;
                  }

                  return ray_cast_result.did_hit;
                });

                for (const Light& light : scene->components<Light>())
                {
                  Entity* const  entity            = &light.owner();
                  const Vector3f pos_to_cam        = vec::normalized(Vector3f(m_Camera->cpu_camera.position) - Vector3f(entity->transform().world_position));
                  const auto     center_pos        = Vector3f(entity->transform().world_position) + pos_to_cam * 0.2f;
                  auto           accurate_hit_test = hitTestQuad(ray, center_pos, pos_to_cam, Vector2f{k_LightIconSize}, entity->transform().inv_world_transform);

                  if (accurate_hit_test.did_hit)
                  {
                    accurate_hit_test.entity = entity;
                    clicked_nodes.emplace_back(accurate_hit_test);
                  }
                }

                auto& selection = editor.selection();

                if (!editor.isShiftDown())
                {
                  selection.clear();
                }

                if (!clicked_nodes.empty())
                {
                  clicked_nodes.sort([](const HitTestResult& a, const HitTestResult& b) {
                    return a.t < b.t;
                  });

                  const HitTestResult best_click = clicked_nodes.front();

                  selection.toggle(static_cast<Entity*>(best_click.entity));
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
      auto&      engine = editor.engine();
      const auto scene  = engine.currentScene();

      if (scene)
      {
        {
          for (const SpriteRenderer& sprite : scene->components<SpriteRenderer>())
          {
            // Screen space pos testing.

            const auto&    transform       = sprite.owner().transform();
            const Vector3f center_position = transform.world_position;
            Vector3f       ss_pos          = bfCamera_worldToScreenSpace(&m_Camera->cpu_camera, center_position);

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

        primitive_proto.material  = editor.m_SceneLightMaterial.typedHandle();
        primitive_proto.size      = {k_LightIconSize, k_LightIconSize};
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

          for (const Light& light : scene->components<Light>())
          {
            Entity* const  entity     = &light.owner();
            const Vector3f pos_to_cam = vec::normalized(Vector3f(m_Camera->cpu_camera.position) - Vector3f(entity->transform().world_position));
            const auto     center_pos = Vector3f(entity->transform().world_position) + pos_to_cam * 0.2f;

            Mat4x4 inv_world_mat = transform;

            *Mat4x4_get(&inv_world_mat, 3, 0) = center_pos.x;
            *Mat4x4_get(&inv_world_mat, 3, 1) = center_pos.y;
            *Mat4x4_get(&inv_world_mat, 3, 2) = center_pos.z;

            Mat4x4_inverse(&inv_world_mat, &inv_world_mat);

            auto accurate_hit_test = hitTestQuad(ray, center_pos, pos_to_cam, Vector2f{k_LightIconSize}, inv_world_mat);

            if (accurate_hit_test.did_hit)
            {
              engine.debugDraw().addAABB(
               Vector3f(m_Camera->cpu_camera.position) + accurate_hit_test.t * Vector3f(ray.direction),
               Vector3f{0.2f},
               bfColor4u_fromUint32(0xFFFFFFFF));
            }
          }
        }
      }

      updateCameraMovement(editor, dt);
    }
  }

  void SceneView::onDraw(EditorOverlay& editor, Engine& engine, RenderView& camera, float alpha)
  {
    // Highlight Selected Objects

    const auto& selection       = editor.selection();
    auto&       engine_renderer = engine.renderer();

    bfDrawCallPipeline pipeline;
    bfDrawCallPipeline_defaultOpaque(&pipeline);

    pipeline.program         = engine_renderer.m_GBufferSelectionShader;
    pipeline.vertex_layout   = engine_renderer.m_StandardVertexLayout;
    pipeline.state.fill_mode = BF_POLYGON_MODE_LINE;

    selection.forEachOfType<Entity*>([this, &camera, &pipeline, &engine_renderer](Entity* entity) {
      MeshRenderer* const mesh_renderer = entity->get<MeshRenderer>();

      if (mesh_renderer && mesh_renderer->material() && mesh_renderer->model())
      {
        ComponentRenderer::pushModel(
         camera,
         entity,
         *mesh_renderer->model(),
         pipeline,
         engine_renderer,
         camera.transparent_render_queue,
         0.0f);
      }
    });
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
