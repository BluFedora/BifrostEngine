/*!
 * @file   bifrost_igame_state_layer.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   A single gamestate, used with the 'GameStateMachine' to
 *   handle stackable game states.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_IGAME_STATE_LAYER_HPP
#define BIFROST_IGAME_STATE_LAYER_HPP

#include "bifrost/utility/bifrost_non_copy_move.hpp" /* bfNonCopyMoveable<T> */

class BifrostEngine;

namespace bifrost
{
  struct Event;

  class IGameStateLayer : private bfNonCopyMoveable<IGameStateLayer>
  {
    friend class BifrostEngine;
    friend class GameStateMachine;

   private:
    IGameStateLayer* m_Prev;
    IGameStateLayer* m_Next;
    bool             m_IsOverlay;

   protected:
    IGameStateLayer();

    virtual void onCreate(BifrostEngine& engine);
    virtual void onLoad(BifrostEngine& engine);
    virtual void onEvent(BifrostEngine& engine, Event& event);
    virtual void onUpdate(BifrostEngine& engine, float delta_time);
    virtual void onUnload(BifrostEngine& engine);
    virtual void onDestroy(BifrostEngine& engine);

   public:
    virtual const char* name() = 0;
    virtual ~IGameStateLayer() = default;

    [[nodiscard]] IGameStateLayer* prev() const { return m_Prev; }
    [[nodiscard]] IGameStateLayer* next() const { return m_Next; }
  };
}  // namespace bifrost

#endif /* BIFROST_IGAME_STATE_LAYER_HPP */
