#include "bf/asset_io/bf_spritesheet_asset.hpp"

#include "bf/anim2D/bf_animation_system.hpp"  // AnimationSystem
#include "bf/asset_io/bf_document.hpp"
#include "bf/asset_io/bifrost_file.hpp"  // File
#include "bf/core/bifrost_engine.hpp"    // Engine

namespace bf
{
  SpritesheetAsset::SpritesheetAsset() :
    Base(),
    m_Anim2DSpritesheet{nullptr}
  {
  }
  /*
  void SpritesheetAsset::onLoad()
  {
    Assets&       assets    = this->assets();
    Engine&       engine    = assets.engine();
    const String& full_path = document().fullPath();
    File          file_in   = {full_path, file::FILE_MODE_READ};

    if (file_in)
    {
      LinearAllocatorScope scope       = {engine.tempMemory()};
      const ScopedBuffer     json_buffer = file_in.readAll(engine.tempMemory());
      const auto           file_name   = document().nameWithExt();

      m_Anim2DSpritesheet = bfAnim2D_loadSpritesheet(
       engine.animationSys().anim2DCtx(),
       bfStringSpan{file_name.begin(), file_name.length()},
       reinterpret_cast<const uint8_t*>(json_buffer.buffer()),
       json_buffer.size());

      m_Anim2DSpritesheet->user_data = this;

      markIsLoaded();
    }
    else
    {
      markFailedToLoad();
    }
  }

  void SpritesheetAsset::onUnload()
  {
    bfAnim2D_destroySpritesheet(
     assets().engine().animationSys().anim2DCtx(),
     m_Anim2DSpritesheet);
  }
  */
  void assetImportSpritesheet(AssetImportCtx& ctx)
  {

  }
}  // namespace bf
