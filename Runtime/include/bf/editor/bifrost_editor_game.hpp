#ifndef BIFROST_EDITOR_GAME_HPP
#define BIFROST_EDITOR_GAME_HPP

#include "bf/core/bifrost_engine.hpp"
#include "bf/utility/bifrost_json.hpp"
#include "bifrost_editor_window.hpp"  // EditorWindow

namespace bf::editor
{
  class GameView final : public EditorWindow<GameView>
  {
   private:
    EditorOverlay* m_Editor;
    CameraRender*  m_Camera;
    json::Value    m_SerializedScene;

   public:
    explicit GameView();
    ~GameView();

   protected:
    const char* title() const override { return "Game View"; }
    void        onDrawGUI(EditorOverlay& editor) override;

   private:
    void toggleEngineState(Engine& engine, const AssetSceneHandle& scene);
    void startSimulation(Engine& engine, const AssetSceneHandle& scene);
    void stopSimulation(Engine& engine, const AssetSceneHandle& scene);
  };
}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_GAME_HPP */