#include "bifrost/editor/bifrost_editor_selection.hpp"

namespace bifrost::editor
{
  Selection::Selection(IMemoryManager& memory):
    m_Selectables{memory}
  {
  }
}
