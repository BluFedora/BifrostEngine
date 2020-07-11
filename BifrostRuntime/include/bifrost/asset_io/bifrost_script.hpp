/*!
 * @file   bifrost_script.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   Implements the IBehavior interface for runtime scripts.
 *
 * @version 0.0.1
 * @date    2020-07-08
 *
 * @copyright Copyright (c) 2020
 */
#ifndef BIFROST_SCRIPT_HPP
#define BIFROST_SCRIPT_HPP

#include "bifrost/asset_io/bifrost_asset_handle.hpp" /* AssetHandle   */
#include "bifrost/asset_io/bifrost_asset_info.hpp"   /* AssetInfo     */
#include "bifrost/core/bifrost_base_object.hpp"      /* BaseObject    */
#include "bifrost/script/bifrost_vm.h"               /* bfValueHandle */
#include "bifrost/script/bifrost_vm.hpp"             /* VMView        */

namespace bifrost
{
  class Script final : public BaseObject<Script>
  {
    friend class AssetScriptInfo;

   private:
    VMView&       m_VM;
    bfValueHandle m_ModuleHandle;

   public:
    Script(VMView& vm);

    bfValueHandle vmModuleHandle() const { return m_ModuleHandle; }

    ~Script();
  };

  class AssetScriptInfo final : public AssetInfo<Script, AssetScriptInfo>
  {
   private:
    using BaseT = AssetInfo<Script, AssetScriptInfo>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
  };

  using AssetScriptHandle = AssetHandle<Script>;
}  // namespace bifrost

#endif /* BIFROST_SCRIPT_HPP */
