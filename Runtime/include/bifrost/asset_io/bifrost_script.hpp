/*!
 * @file   bifrost_script.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
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

namespace bf
{
  class Script final : public BaseObject<Script>
  {
    friend class AssetScriptInfo;

   private:
    bfValueHandle m_ModuleHandle;

   public:
    Script(bfValueHandle module_handle);

    bfValueHandle vmModuleHandle() const { return m_ModuleHandle; }
  };

  class AssetScriptInfo final : public AssetInfo<Script, AssetScriptInfo>
  {
   private:
    using BaseT = AssetInfo<Script, AssetScriptInfo>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
    void onAssetUnload(Engine& engine) override;
  };

  using AssetScriptHandle = AssetHandle<Script>;
}  // namespace bf

#endif /* BIFROST_SCRIPT_HPP */
