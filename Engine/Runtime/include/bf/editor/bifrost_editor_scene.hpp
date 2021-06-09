#ifndef BIFROST_EDITOR_SCENE_HPP
#define BIFROST_EDITOR_SCENE_HPP

#include "bifrost_editor_window.hpp"  // EditorWindow

#include <ImGuizmo/ImGuizmo.h> // ImGuizmo::OPERATION

namespace bf::editor
{
  class SceneView final : public EditorWindow<SceneView>
  {
   private:
    Rect2i              m_SceneViewViewport;  // Global Window Coordinates
    bool                m_IsSceneViewHovered;
    RenderView*         m_Camera;
    Vector2f            m_OldMousePos;
    Vector2f            m_MousePos;
    bool                m_IsDraggingMouse;
    float               m_MouseLookSpeed;
    EditorOverlay*      m_Editor;
    ImVec2              m_OldWindowPadding;
    ImGuizmo::OPERATION m_GizmoOp;

   public:
    explicit SceneView();
    ~SceneView() override;

    bool isPointOverSceneView(const Vector2i& point) const
    {
      return m_IsSceneViewHovered && m_SceneViewViewport.intersects(point);
    }

   protected:
    const char* title() const override { return "Scene View"; }
    void        onPreDrawGUI(EditorOverlay& editor) override;
    void        onDrawGUI(EditorOverlay& editor) override;
    void        onPostDrawGUI(EditorOverlay& editor) override;
    void        onEvent(EditorOverlay& editor, Event& event) override;
    void        onUpdate(EditorOverlay& editor, float dt) override;
    void        onDraw(EditorOverlay& editor, Engine& engine, RenderView& camera, float alpha) override;

   private:
    bool isGizmoOver(const Selection& selection) const;
    void updateCameraMovement(EditorOverlay& editor, float dt) const;
  };
}  // namespace bf::editor

#endif /* BIFROST_EDITOR_SCENE_HPP */