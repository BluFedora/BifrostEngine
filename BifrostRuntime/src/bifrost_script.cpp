/*!
 * @file   bifrost_script.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Implements the IBehavior interface for runtime scripts.
 *
 * @version 0.0.1
 * @date    2020-07-08
 *
 * @copyright Copyright (c) 2020
 */
#include "bifrost/asset_io/bifrost_script.hpp"

#include "bifrost/asset_io/bifrost_file.hpp" /* File   */
#include "bifrost/core/bifrost_engine.hpp"   /* Engine */
#include "bifrost/script/bifrost_vm.hpp"     /* VMView */

namespace bf
{
  Script::Script(VMView& vm) :
    m_VM{vm},
    m_ModuleHandle{nullptr}
  {
  }

  Script::~Script()
  {
    m_VM.stackDestroyHandle(m_ModuleHandle);
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
        auto& script = m_Payload.set<Script>(vm);

        script.m_ModuleHandle = vm.stackMakeHandle(0);

        return true;
      }
    }

    return false;
  }
}  // namespace bifrost
