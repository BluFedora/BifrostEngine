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
    RenderView*    m_Camera;
    json::Value    m_SerializedScene;

   public:
    explicit GameView();
    ~GameView() override;

   protected:
    const char* title() const override { return "Game View"; }

    void onPreDrawGUI(EditorOverlay& editor) override;
    void onDrawGUI(EditorOverlay& editor) override;

   private:
    void toggleEngineState(Engine& engine, const ARC<SceneAsset>& scene);
    void startSimulation(Engine& engine, const ARC<SceneAsset>& scene);
    void stopSimulation(Engine& engine, const ARC<SceneAsset>& scene);
  };
}  // namespace bf::editor

#endif /* BIFROST_EDITOR_GAME_HPP */