#include "bifrost/core/bifrost_engine.hpp"

BifrostEngine::BifrostEngine(IBaseWindow& window, char* main_memory, std::size_t main_memory_size, int argc, const char* argv[]) :
  Camera{},
  m_CmdlineArgs{argc, argv},
  m_MainMemory{main_memory, main_memory_size},
  m_TempMemory{static_cast<char*>(m_MainMemory.allocate(main_memory_size / 4)), main_memory_size / 4},
  m_TempAdapter{m_TempMemory},
  m_StateMachine{*this, m_MainMemory},
  m_Scripting{},
  m_Renderer{m_MainMemory},
  m_DebugRenderer{m_MainMemory},
  m_SceneStack{m_MainMemory},
  m_Assets{*this, m_MainMemory},
  m_Systems{m_MainMemory},
  m_Window{window}
{
  // TEMP CODE BGN
  const Vec3f cam_pos = {0.0f, 0.0f, 4.0f, 1.0f};
  Camera_init(&Camera, &cam_pos, nullptr, 0.0f, 0.0f);
  // TEMP CODE END
}

AssetSceneHandle BifrostEngine::currentScene() const
{
  return m_SceneStack.isEmpty() ? AssetSceneHandle{} : m_SceneStack.back();
}

namespace bifrost
{
  void detail::CoreEngineGameStateLayer::onEvent(BifrostEngine& engine, Event& event)
  {
    if (event.type == EventType::ON_WINDOW_RESIZE)
    {
      const int window_width  = event.window.width;
      const int window_height = event.window.height;

      Camera_onResize(&engine.Camera, window_width, window_height);
      engine.renderer().resize(window_width, window_height);
    }

    // This is the bottom most layer so just accept the event.
    event.accept();
  }
}  // namespace bifrost