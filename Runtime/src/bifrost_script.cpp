/*!
 * @file   bifrost_script.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *
 * @version 0.0.1
 * @date    2020-07-08
 *
 * @copyright Copyright (c) 2020
 */
#include "bf/asset_io/bifrost_script.hpp"

#include "bf/asset_io/bifrost_file.hpp" /* File   */
#include "bf/core/bifrost_engine.hpp"   /* Engine */
#include "bifrost/bifrost_vm.hpp"       /* VMView */

namespace bf
{
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

  void AssetScriptInfo::onAssetUnload(Engine& engine)
  {
    engine.scripting().stackDestroyHandle(payloadT()->m_ModuleHandle);
  }
}  // namespace bf
