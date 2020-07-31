
#include "bifrost/asset_io/bifrost_scene.hpp"
#include "bifrost/debug/bifrost_dbg_logger.h"
#include "bifrost/ecs/bifrost_behavior.hpp"
#include "bifrost/ecs/bifrost_entity.hpp"

using namespace bf;

class CameraController final : public Behavior<CameraController>
{
 private:
  EntityRef m_Player = nullptr;

 public:
  void onEnable() override;
  void onUpdate(float dt) override;
  void onDisable() override;
  void serialize(ISerializer& serializer) override;
};

void CameraController::onEnable()
{
  // If the player was not assigned through the editor.
  if (!m_Player)
  {
    m_Player = scene().findEntity("Rhino");
  }

  if (m_Player)
  {
    setEventFlags(ON_UPDATE);
  }
  else
  {
    bfLogWarn("Failed to find the player.");
  }
}

void CameraController::onUpdate(float dt)
{
  auto&    camera     = scene().camera();
  Vector3f player_pos = m_Player->transform().world_position;

  player_pos -= Vector3f{0.0f, 0.0f, 3.0f};

  bfCamera_setPosition(&camera, &player_pos);
}

void CameraController::onDisable()
{
}

void CameraController::serialize(ISerializer& serializer)
{
  serializer.serialize("m_Player", m_Player);
}

bfRegisterBehavior(CameraController);
