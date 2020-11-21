# Editor Implementation

## Undo Redo Commands

### Delete Entity

- Construction
  - Save which Entity is being deleted.
  - Save Some way to reference it's location in the scene graph.
- Redo
  - Removed from Scene Graph.
    - If has parent remove from parent child list.
    - Else removed from root scene list.
  - DO this for Self and All Children recursively
    - Mark as Deleted so the GC will handle it when the time comes.
    - Moved all components to the inactive list.
    - Move all Behaviors to the inactive list.
- Undo
  - Add back into the scene graph at the right location.
- Destruction
