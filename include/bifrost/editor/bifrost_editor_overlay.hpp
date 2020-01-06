#ifndef BIFROST_EDITOR_OVERLAY_HPP
#define BIFROST_EDITOR_OVERLAY_HPP

#include "bifrost/bifrost.hpp"

namespace bifrost::editor
{
  class MenuItem
  {
    
  };

  class EditorOverlay final : public IGameStateLayer
  {
   private:
   protected:
    void onCreate(BifrostEngine& engine) override;
    void onLoad(BifrostEngine& engine) override;
    void onEvent(BifrostEngine& engine, Event& event) override;
    void onUpdate(BifrostEngine& engine, float delta_time) override;
    void onUnload(BifrostEngine& engine) override;
    void onDestroy(BifrostEngine& engine) override;

   public:
    const char* name() override
    {
      return "Bifrost Editor";
    }

   private:
  };

}  // namespace bifrost::editor

#endif /* BIFROST_EDITOR_OVERLAY_HPP */