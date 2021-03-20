#if 0
#include "bf/asset_io/bifrost_file.hpp"    /* File                  */
#include "bf/core/bifrost_base_object.hpp" /* BaseObject            */
#include "bf/core/bifrost_engine.hpp"      /* Engine                */
#include "bifrost/bifrost_vm.hpp"          /* VMView, bfValueHandle */
#include "bifrost/bifrost_vm.hpp"          /* VMView                */
#endif

namespace bf
{
#if 0
  Script::Script(bfValueHandle module_handle) :
    m_ModuleHandle{module_handle}
  {
  }

  bool AssetScriptInfo::load(Engine& engine)
  {
    Assets&      assets    = engine.assets();
    const String full_path = filePathAbs();
    File         file      = {full_path, file::FILE_MODE_READ};

    if (file)
    {
      auto&   tmp_allocator = engine.tempMemory();
      VMView& vm            = engine.scripting();

      LinearAllocatorScope scope{tmp_allocator};

      const TempBuffer     buffer = file.readAll(engine.tempMemoryNoFree());
      const BifrostVMError err    = vm.execInModule(
       nullptr,  // TODO(SR): Maybe I will want to set the module name at some point.
       buffer.buffer(),
       buffer.size());

      if (err == BIFROST_VM_ERROR_NONE)
      {
        m_Payload.set<Script>(vm.stackMakeHandle(0));

        return true;
      }
    }

    return false;
  }

  void AssetScriptInfo::onAssetUnload(Engine& engine) { engine.scripting().stackDestroyHandle(payloadT()->m_ModuleHandle); }
#endif
}  // namespace bf
