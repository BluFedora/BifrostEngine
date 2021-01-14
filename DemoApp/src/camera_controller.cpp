
#include "bf/Engine.hpp"
#include "bf/FreeListAllocator.hpp"
#include "bf/MemoryUtils.h"
#include "bf/asset_io/bf_iserializer.hpp"
#include "bf/asset_io/bifrost_scene.hpp"
#include "bf/debug/bifrost_dbg_logger.h"
#include "bf/ecs/bifrost_behavior.hpp"
#include "bf/ecs/bifrost_entity.hpp"

using namespace bf;

class CameraController final : public Behavior<CameraController>
{
 private:
  EntityRef m_Player = nullptr;

 public:
  void onEnable() override;
  void onUpdate(float dt) override;
  void onDisable() override;
  void reflect(ISerializer& serializer) override;
};

bfRegisterBehavior(CameraController);

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

void CameraController::reflect(ISerializer& serializer)
{
  serializer.serialize("m_Player", m_Player);
}

// TODO(SR): Make the gameplay heap part of the core engine.
static std::array<char, bfMegabytes(50)> s_GameplayHeapBacking;
static FreeListAllocator                 s_GameplayHeap{s_GameplayHeapBacking.data(), s_GameplayHeapBacking.size()};

// Adapted From: [https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion]
Vector3f RotateVectorByQuat(const Quaternionf& quat, const Vector3f& vector)
{
  const Vector3f vec_part    = {quat.x, quat.y, quat.z, 0.0f};
  const float    scalar_part = quat.w;

  return 2.0f * vec::dot(vec_part, vector) * vec_part +
         (scalar_part * scalar_part - vec::dot(vec_part, vec_part)) * vector +
         2.0f * scalar_part * vec::cross(vec_part, vector);
}

static const uint s_Colors[] = {BIFROST_COLOR_CORAL, BIFROST_COLOR_CORNFLOWERBLUE, BIFROST_COLOR_CORNSILK, BIFROST_COLOR_CYAN, BIFROST_COLOR_DEEPPINK};

static const float k_ChainLinkLen = 0.5f;

//
// [https://www.euclideanspace.com/physics/kinematics/joints/ik/index.htm]
// [http://what-when-how.com/advanced-methods-in-computer-graphics/kinematics-advanced-methods-in-computer-graphics-part-4/]
//

class IkDemo final : public Behavior<IkDemo>
{
  struct IKJoint
  {
    Quaternionf rotation;
    float       length;

    Vector3f points[2];  // Cached points.

    // Note: I Only need one of these but I wanted to test both methods.
    Mat4x4      cached_world;
    Quaternionf parent_rot;

    void EndPointFrom(Quaternionf& parent_rotation, Vector3f& start_pos)
    {
      const Quaternionf total_rotation = bfQuaternionf_multQ(&parent_rotation, &rotation);

      points[0] = start_pos;

      parent_rot = parent_rotation;
      bfQuaternionf_toMatrix(parent_rotation, &cached_world);

      start_pos       = start_pos + RotateVectorByQuat(total_rotation, {length, 0.0f, 0.0f, 0.0f});
      parent_rotation = total_rotation;

      points[1] = start_pos;
    }
  };

 private:
  EntityRef      m_TargetPoint = nullptr;
  Array<IKJoint> m_Joints{s_GameplayHeap};
  bool           m_IsOverlay{true};
  float          m_DistToTarget;

 public:
  void onEnable() override
  {
    for (int i = 0; i < 3; ++i)
    {
      m_Joints.push(IKJoint{bfQuaternionf_identity(), k_ChainLinkLen});
    }

    // m_Joints.push(IKJoint{bfQuaternionf_identity(), 1.0f});
    // m_Joints.push(IKJoint{bfQuaternionf_identity(), 2.0f});
    // m_Joints.push(IKJoint{bfQuaternionf_identity(), 3.0f});
    // m_Joints.push(IKJoint{bfQuaternionf_identity(), 2.0f});
    // m_Joints.push(IKJoint{bfQuaternionf_identity(), 1.0f});

    setEventFlags(ON_UPDATE);
  }

  void onUpdate(float dt) override
  {
    // TODO(SR): All uses of this function goes through all bones. Not good, they need to use the 'start_index' parameter.
    const auto recalculate_joint_positions = [this](int start_index = 0) {
      const int   num_joints    = int(m_Joints.length());
      auto&       transform     = owner().transform();
      Vector3f    base_position = transform.world_position;
      Quaternionf base_rotation = transform.world_rotation;

      for (int i = start_index; i < num_joints; ++i)
      {
        m_Joints[i].EndPointFrom(base_rotation, base_position);
      }
    };

    recalculate_joint_positions();

    // Draw Them
    auto& dbg_drawer = engine().debugDraw();

    for (int i = 0; i < int(m_Joints.length()); ++i)
    {
      dbg_drawer.addLine(
       m_Joints[i].points[0],
       m_Joints[i].points[1],
       bfColor4u_fromUint32(s_Colors[i % bfCArraySize(s_Colors)]),
       0.0f,
       m_IsOverlay);
    }

    if (m_TargetPoint)
    {
      const Vector3f target_pos = m_TargetPoint->transform().world_position;
      Vector3f       end_point  = m_Joints.back().points[1];

      m_DistToTarget = vec::length(Vector3f{owner().transform().world_position} - target_pos);

      for (IKJoint& j : ReverseLoop(m_Joints))
      {
        if (math::isAlmostEqual(vec::length(target_pos - end_point), 0.0f) || vec::length(target_pos - end_point) <= 0.1f)
        {
          break;
        }

        // Calculate Rotation Axis

        const Vector3f       pos_to_end    = end_point - j.points[0];
        const Vector3f       pos_to_target = target_pos - j.points[0];
        /* const */ Vector3f rot_axis      = vec::normalized(vec::cross(pos_to_end, pos_to_target));

        if (math::isAlmostEqual(vec::length(rot_axis), 0.0f))
        {
          continue;
        }

        // Convert Rotation Axis From World Space To Bone Local Space.

#if 0 /* Matrix Method */
        Mat4x4 inv_world;

        if (Mat4x4_inverse(&j.cached_world, &inv_world))
        {
          Mat4x4_multVec(&inv_world, &rot_axis, &rot_axis);
        }
#else /* Quaternion Method */
        rot_axis = RotateVectorByQuat(bfQuaternionf_conjugate(&j.parent_rot), rot_axis);
#endif

        // Calculate The Angle

        const float             dot_prod          = vec::dot(pos_to_end, pos_to_target);
        const float             pos_to_end_len    = vec::length(pos_to_end);
        const float             pos_to_target_len = vec::length(pos_to_target);
        const float             cos_value         = dot_prod / (pos_to_end_len * pos_to_target_len);
        const float             rot_angle         = std::acos(math::clamp(-1.0f, cos_value, 1.0f));
        /* const */ Quaternionf rot               = bfQuaternionf_fromAxisAngleRad(&rot_axis, rot_angle);

        bfQuaternionf_normalize(&rot);

        j.rotation = bfQuaternionf_multQ(&rot, &j.rotation);
        bfQuaternionf_normalize(&j.rotation);

        recalculate_joint_positions();
        end_point = m_Joints.back().points[1];
      }

      dbg_drawer.addAABB(end_point, {0.3f}, bfColor4u_fromUint32(BIFROST_COLOR_ALICEBLUE), 0.0f, true);
    }
  }

  void onDisable() override
  {
    m_Joints.clear();
  }

 private:
  void reflect(ISerializer& serializer) override
  {
    if (serializer.mode() == SerializerMode::INSPECTING)
    {
      std::size_t joints_size = m_Joints.size();

      float ARM_SIZE = float(joints_size) * k_ChainLinkLen;

      serializer.serialize("m_DistToTarget", m_DistToTarget);
      serializer.serialize("ARM_SIZE", ARM_SIZE);
      serializer.serialize("m_TargetPoint", m_TargetPoint);
      serializer.serialize("m_IsOverlay", m_IsOverlay);

      if (serializer.pushArray("Joints", joints_size))
      {
        for (IKJoint& j : m_Joints)
        {
          if (serializer.pushObject(nullptr))
          {
            serializer.serialize("Rotation", j.rotation);
            serializer.serialize("Length", j.length);

            serializer.popObject();
          }
        }

        serializer.popArray();
      }
    }
  }
};

bfRegisterBehavior(IkDemo);
