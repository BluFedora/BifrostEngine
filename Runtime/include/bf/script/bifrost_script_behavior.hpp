/*!
 * @file   bifrost_script_behavior.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Implements the IBehavior interface for runtime scripts.
 *
 * @version 0.0.1
 * @date    2020-07-08
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_SCRIPT_BEHAVIOR_HPP
#define BIFROST_SCRIPT_BEHAVIOR_HPP

#include "bf/asset_io/bifrost_script.hpp" /* AssetScriptHandle */
#include "bf/ecs/bifrost_behavior.hpp"    /* Behavior<T>       */

namespace bf
{
  class ScriptBehavior final : public Behavior<ScriptBehavior>
  {
   private:
    AssetScriptHandle m_ScriptPath;
    bfValueHandle     m_ScriptOnEnable;
    bfValueHandle     m_ScriptOnUpdate;
    bfValueHandle     m_ScriptOnDisable;

   public:
    ScriptBehavior();

    void onEnable() override;
    void onUpdate(float dt) override;
    void onDisable() override;
    void serialize(ISerializer& serializer) override;

    bool setScriptPath(const String& path);

    ~ScriptBehavior();

   private:
    static bfValueHandle loadVmValueHandleFromModule(VMView& vm, const char* name);
    VMView&              vm() const;
    bool                 loadFunctionPointers();
  };
}  // namespace bf

bfRegisterBehavior(bf::ScriptBehavior);

#endif /* BIFROST_SCRIPT_BEHAVIOR_HPP */
