#include "bf/asset_io/bf_spritesheet_asset.hpp"

#include "bf/anim2D/bf_animation_system.hpp"  // AnimationSystem
#include "bifrost/asset_io/bifrost_file.hpp"  // File
#include "bifrost/core/bifrost_engine.hpp"    // Engine

namespace bf
{
  bool AssetSpritesheetInfo::load(Engine& engine)
  {
    Assets&      assets    = engine.assets();
    const String full_path = filePathAbs();
    File         file_in   = {full_path, file::FILE_MODE_READ};

    if (file_in)
    {
      LinearAllocatorScope scope       = {engine.tempMemory()};
      const TempBuffer     json_buffer = file_in.readAll(engine.tempMemoryNoFree());
      Spritesheet&         sheet       = m_Payload.set<Spritesheet>();

      sheet.m_Anim2DSpritesheet = bfAnimation2D_loadSpritesheet(
       engine.animationSys().anim2DCtx(),
       (const uint8_t*)json_buffer.buffer(),
       json_buffer.size());

      return sheet.m_Anim2DSpritesheet != nullptr;
    }

    return false;
  }
}  // namespace bf
