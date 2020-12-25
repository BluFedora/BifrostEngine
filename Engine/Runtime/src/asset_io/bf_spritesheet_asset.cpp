#include "bf/asset_io/bf_spritesheet_asset.hpp"

#include "bf/anim2D/bf_animation_system.hpp"  // AnimationSystem
#include "bf/asset_io/bifrost_file.hpp"       // File
#include "bf/core/bifrost_engine.hpp"         // Engine

namespace bf
{
  // TODO(SR): We dont want this.
  LinearAllocator& ENGINE_TEMP_MEM(Engine& engine);
  IMemoryManager&  ENGINE_TEMP_MEM_NO_FREE(Engine& engine);

  SpritesheetAsset::SpritesheetAsset() :
    Base(),
    m_Anim2DSpritesheet{nullptr}
  {
  }

  void SpritesheetAsset::onLoad()
  {
    Assets&       assets    = this->assets();
    Engine&       engine    = assets.engine();
    const String& full_path = fullPath();
    File          file_in   = {full_path, file::FILE_MODE_READ};

    if (file_in)
    {
      LinearAllocatorScope scope       = {ENGINE_TEMP_MEM(engine)};
      const TempBuffer     json_buffer = file_in.readAll(engine.tempMemoryNoFree());
      const auto           file_name   = nameWithExt();

      m_Anim2DSpritesheet = bfAnimation2D_loadSpritesheet(
       engine.animationSys().anim2DCtx(),
       bfStringSpan{file_name.begin(), file_name.length()},
       (const uint8_t*)json_buffer.buffer(),
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
    Engine& engine = assets().engine();

    bfAnimation2D_destroySpritesheet(
     engine.animationSys().anim2DCtx(),
     m_Anim2DSpritesheet);
  }
}  // namespace bf
