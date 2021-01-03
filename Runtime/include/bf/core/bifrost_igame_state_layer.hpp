/******************************************************************************/
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
/******************************************************************************/
#ifndef BIFROST_IGAME_STATE_LAYER_HPP
#define BIFROST_IGAME_STATE_LAYER_HPP

#include "bf/PlatformFwd.h"        /* bfEvent              */
#include "bf/bf_non_copy_move.hpp" /* bfNonCopyMoveable<T> */

namespace bf
{
  class Engine;
  using Event = ::bfEvent;

  class GameStateMachine;

  class IGameStateLayer : private NonCopyMoveable<IGameStateLayer>
  {
    friend class GameStateMachine;
    friend class Engine;

   private:
    IGameStateLayer* m_Prev;
    IGameStateLayer* m_Next;
    bool             m_IsOverlay;

   protected:
    IGameStateLayer();

   private:
    virtual void onCreate(Engine& engine);
    virtual void onLoad(Engine& engine);
    virtual void onEvent(Engine& engine, bfEvent& event);
    virtual void onFixedUpdate(Engine& engine, float delta_time);
    virtual void onDraw2D(Engine& engine);
    virtual void onUpdate(Engine& engine, float delta_time);
    virtual void onUnload(Engine& engine);
    virtual void onDestroy(Engine& engine);

   public:
    virtual const char* name() = 0;
    virtual ~IGameStateLayer() = default;

    [[nodiscard]] IGameStateLayer* prev() const { return m_Prev; }
    [[nodiscard]] IGameStateLayer* next() const { return m_Next; }
  };
}  // namespace bf

#endif /* BIFROST_IGAME_STATE_LAYER_HPP */
