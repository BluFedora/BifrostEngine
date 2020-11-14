#include "bf/editor/bifrost_editor_undo_redo.hpp"

#include "bf/ecs/bifrost_entity.hpp" // Entity

namespace bf::editor
{
  IUndoRedoCommandPtr cmd::deleteEntity(Entity& entity)
  {
    struct State final
    {
      EntityRef parent{nullptr};
      EntityRef entity{nullptr};
    };

    State s;

    s.entity  = EntityRef{entity};

    return makeLambdaCommand(
     std::move(s),
     [](State& state) {
       Entity& entity = *state.entity.editorCachedEntity();
       entity.editorLinkEntity(state.parent.editorCachedEntity());
     },
     [](State& state) {
       Entity& entity = *state.entity.editorCachedEntity();

       state.parent = EntityRef{entity.editorUnlinkEntity()};
     });
  }
}  // namespace bifrost::editor
