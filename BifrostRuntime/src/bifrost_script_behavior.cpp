/*!
 * @file   bifrost_script_behavior.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Implements the IBehavior interface for runtime scripts.
 *
 * @version 0.0.1
 * @date    2020-07-08
 *
 * @copyright Copyright (c) 2020
 */
#include "bifrost/script/bifrost_script_behavior.hpp"

#include "bifrost/asset_io/bifrost_file.hpp" /* File   */
#include "bifrost/core/bifrost_engine.hpp"   /* Engine */

namespace bf
{
  static constexpr std::size_t k_CallIndex = 0;

  ScriptBehavior::ScriptBehavior() :
    m_ScriptPath{},
    m_ScriptOnEnable{nullptr},
    m_ScriptOnUpdate{nullptr},
    m_ScriptOnDisable{nullptr}
  {
  }

  void ScriptBehavior::onEnable()
  {
    if (m_ScriptOnEnable)
    {
      VMView& vm = this->vm();

      vm.stackLoadHandle(k_CallIndex, m_ScriptOnEnable);
      vm.call(k_CallIndex);
    }
  }

  void ScriptBehavior::onUpdate(float dt)
  {
    VMView& vm = this->vm();

    vm.stackLoadHandle(k_CallIndex, m_ScriptOnUpdate);
    vm.call(k_CallIndex, dt);
  }

  void ScriptBehavior::onDisable()
  {
    if (m_ScriptOnDisable)
    {
      VMView& vm = this->vm();

      vm.stackLoadHandle(k_CallIndex, m_ScriptOnDisable);
      vm.call(k_CallIndex);
    }
  }

  void ScriptBehavior::serialize(ISerializer& serializer)
  {
    serializer.serialize("m_ScriptPath", m_ScriptPath);

    if (serializer.mode() == SerializerMode::LOADING && m_ScriptPath)
    {
      loadFunctionPointers();
    }
  }

  bool ScriptBehavior::setScriptPath(const String& path)
  {
    const bool loaded_asset = engine().assets().tryLoadAsset<AssetScriptInfo>(m_ScriptPath, path);

    if (loaded_asset && m_ScriptPath)
    {
      return loadFunctionPointers();
    }

    return loaded_asset;
  }

  ScriptBehavior::~ScriptBehavior()
  {
    auto& vm = this->vm();

    vm.stackDestroyHandle(m_ScriptOnEnable);
    vm.stackDestroyHandle(m_ScriptOnUpdate);
    vm.stackDestroyHandle(m_ScriptOnDisable);
  }

  bfValueHandle ScriptBehavior::loadVmValueHandleFromModule(VMView& vm, const char* name)
  {
    // Preconditions:
    //   - The module exists in slot 0
    //   - Stack space has already been resized to 2.

    static constexpr std::size_t k_ModuleIndex   = 0;
    static constexpr std::size_t k_VariableIndex = 1;

    vm.stackLoadVariable(k_VariableIndex, k_ModuleIndex, name);

    if (vm.stackGetType(k_VariableIndex) == BIFROST_VM_FUNCTION)
    {
      return vm.stackMakeHandle(k_VariableIndex);
    }

    return nullptr;
  }

  VMView& ScriptBehavior::vm() const
  {
    return engine().scripting();
  }

  bool ScriptBehavior::loadFunctionPointers()
  {
    VMView&              vm  = this->vm();
    const BifrostVMError err = vm.stackResize(2);

    if (err == BIFROST_VM_ERROR_NONE)
    {
      vm.stackLoadHandle(0, m_ScriptPath->vmModuleHandle());

      const struct
      {
        bfValueHandle&     handle;
        const char*        name;
        BehaviorEventFlags evt_flags;

      } variables_to_load[] =
       {
#define VarToLoadEntry(name, flags) {m_ScriptOn##name, "on" #name, flags}

        VarToLoadEntry(Enable, ON_NOTHING),
        VarToLoadEntry(Update, ON_UPDATE),
        VarToLoadEntry(Disable, ON_NOTHING),

#undef VarToLoadEntry
       };

      for (const auto& var : variables_to_load)
      {
        vm.stackDestroyHandle(var.handle);

        var.handle = loadVmValueHandleFromModule(vm, var.name);

        if (var.evt_flags && var.handle)
        {
          setEventFlags(var.evt_flags);
        }
      }

      return true;
    }

    return false;
  }
}  // namespace bifrost
