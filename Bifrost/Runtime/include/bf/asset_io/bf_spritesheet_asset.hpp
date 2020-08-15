#ifndef BF_SPRITESHEET_ASSET_HPP
#define BF_SPRITESHEET_ASSET_HPP

#include "bf/Animation2D.h"   // bfSpritesheet
#include "bf/BaseObject.hpp"  // BaseObject

#include "bifrost/asset_io/bifrost_asset_handle.hpp" /* AssetHandle<T> */
#include "bifrost/asset_io/bifrost_asset_info.hpp"   /* AssetInfo<T1, T2> */

namespace bf
{
  class Spritesheet final : public BaseObject<Spritesheet>
  {
    friend class AssetSpritesheetInfo;

   private:
    bfSpritesheet*  m_Anim2DSpritesheet;

   public:
    Spritesheet() :
      Base(),
      m_Anim2DSpritesheet{nullptr}
    {
    }
  };

  class AssetSpritesheetInfo final : public AssetInfo<Spritesheet, AssetSpritesheetInfo>
  {
   private:
    using BaseT = AssetInfo<Spritesheet, AssetSpritesheetInfo>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
  };

  using AssetSpritesheetHandle = AssetHandle<Spritesheet>;
}  // namespace bf

BIFROST_META_REGISTER(bf::AssetSpritesheetInfo)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<AssetSpritesheetInfo>("AssetSpritesheetInfo"),  //
     ctor<String, std::size_t, BifrostUUID>())
  BIFROST_META_END()
}

#endif /* BF_SPRITESHEET_ASSET_HPP */
