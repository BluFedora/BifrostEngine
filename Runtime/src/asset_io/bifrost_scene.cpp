/*!
* @file   bifrost_scene.cpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   This is where Entities live in the engine.
*   Also contains the storage for the components.
*
* @version 0.0.1
* @date    2019-12-22
*
* @copyright Copyright (c) 2019
*/
#include "bifrost/asset_io/bifrost_scene.hpp"

#include "bifrost/asset_io/bifrost_file.hpp"            /* File                 */
#include "bifrost/asset_io/bifrost_json_serializer.hpp" /* JsonSerializerReader */
#include "bifrost/core/bifrost_engine.hpp"              /* Engine               */
#include "bifrost/ecs/bifrost_collision_system.hpp"     /* DebugRenderer        */
#include "bifrost/ecs/bifrost_entity.hpp"               /* Entity               */
#include "bifrost/utility/bifrost_json.hpp"             /* json::Value          */

#include "bf/anim2D/bf_animation_system.hpp"

namespace bf
{
#define ID_TO_INDEX(id) ((id)-1)

  BifrostTransform* SceneTransformSystem::transformFromIDImpl(IBifrostTransformSystem_t* self, BifrostTransformID id)
  {
    SceneTransformSystem* const system = static_cast<SceneTransformSystem*>(self);

    return id == k_TransformInvalidID ? nullptr : &system->m_Transforms[ID_TO_INDEX(id)];
  }

  BifrostTransformID SceneTransformSystem::transformToIDImpl(IBifrostTransformSystem_t* self, BifrostTransform* transform)
  {
    SceneTransformSystem* const system = static_cast<SceneTransformSystem*>(self);

    if (transform == nullptr)
    {
      return k_TransformInvalidID;
    }

    return BifrostTransformID(system->m_Transforms.indexOf(static_cast<TransformNode*>(transform))) + 1;
  }

  void SceneTransformSystem::addToDirtyListImpl(IBifrostTransformSystem_t* self, BifrostTransform* transform)
  {
    SceneTransformSystem* const system = static_cast<SceneTransformSystem*>(self);

    if (system->dirty_list)
    {
      system->dirty_list->dirty_list_next = transform;
    }

    system->dirty_list = transform;
  }

  SceneTransformSystem::SceneTransformSystem(IMemoryManager& memory) :
    IBifrostTransformSystem{nullptr, &transformFromIDImpl, &transformToIDImpl, &addToDirtyListImpl},
    m_Transforms{memory},
    m_FreeList{k_TransformInvalidID}
  {
  }

  BifrostTransformID SceneTransformSystem::createTransform()
  {
    BifrostTransformID id;

    if (m_FreeList)
    {
      id         = m_FreeList;
      m_FreeList = m_Transforms[ID_TO_INDEX(id)].freelist_next;
    }
    else
    {
      m_Transforms.emplace();
      id                                          = BifrostTransformID(m_Transforms.size());
      m_Transforms[ID_TO_INDEX(id)].freelist_next = k_TransformInvalidID;
    }

    bfTransform_ctor(&m_Transforms[ID_TO_INDEX(id)], this);
    return id;
  }

  void SceneTransformSystem::destroyTransform(BifrostTransformID transform)
  {
    bfTransform_dtor(&m_Transforms[ID_TO_INDEX(transform)]);

    m_Transforms[ID_TO_INDEX(transform)].freelist_next = m_FreeList;
    m_FreeList                                         = transform;
  }

#undef ID_TO_INDEX

  Scene::Scene(Engine& engine) :
    BaseObject<Scene>{},
    m_Engine{engine},
    m_Memory{m_Engine.mainMemory()},
    m_RootEntities{m_Memory},
    m_Entities{&Entity::m_Hierarchy},
    m_ActiveComponents{m_Memory},
    m_InactiveComponents{m_Memory},
    m_ActiveBehaviors{m_Memory},
    m_BVHTree{m_Memory},
    m_TransformSystem{m_Memory},
    m_Camera{},
    m_AnimationScene{bfAnimation2D_createScene(engine.animationSys().anim2DCtx())}
  {
    Camera_init(&m_Camera, nullptr, nullptr, 0.0f, 0.0f);
  }

  EntityRef Scene::addEntity(const StringRange& name)
  {
    return m_Engine.createEntity(*this, name);
  }

  EntityRef Scene::findEntity(const StringRange& name) const
  {
    for (Entity* const root_entity : m_RootEntities)
    {
      if (root_entity->name() == name)
      {
        return EntityRef{root_entity};
      }
    }

    return EntityRef{};
  }

  void Scene::removeEntity(Entity* entity)
  {
    m_RootEntities.removeAt(m_RootEntities.find(entity));
  }

  void Scene::removeAllEntities()
  {
    while (!m_RootEntities.isEmpty())
    {
      m_RootEntities.back()->destroy();

      // Entity::destroy detaches from parent.
      // m_RootEntities.pop();
    }
  }

  void Scene::update(LinearAllocator& temp, DebugRenderer& dbg_renderer)
  {
    for (Entity* entity : m_RootEntities)
    {
      markEntityTransformDirty(entity);
    }

    m_BVHTree.endFrame(temp, true);
    #if 0
    m_BVHTree.traverse([&dbg_renderer](const BVHNode& node) {
      // Don't draw inactive entities.
      if (bvh_node::isLeaf(node))
      {
        Entity* const entity = (Entity*)node.user_data;

        if (!entity->isActive())
        {
          return;
        }
      }

      dbg_renderer.addAABB(
       (Vector3f(node.bounds.max[0], node.bounds.max[1], node.bounds.max[2]) + Vector3f(node.bounds.min[0], node.bounds.min[1], node.bounds.min[2])) * 0.5f,
       Vector3f(node.bounds.max[0], node.bounds.max[1], node.bounds.max[2]) - Vector3f(node.bounds.min[0], node.bounds.min[1], node.bounds.min[2]),
       bfColor4u_fromUint32(BIFROST_COLOR_CYAN));
    });
    #else
    (void)dbg_renderer;
    #endif
  }

  void Scene::markEntityTransformDirty(Entity* entity)
  {
    for (Entity& child : entity->children())
    {
      markEntityTransformDirty(&child);
    }

    m_BVHTree.markLeafDirty(entity->bvhID(), entity->transform());
  }

  void Scene::serialize(ISerializer& serializer)
  {
    if (serializer.mode() == SerializerMode::LOADING)
    {
      // Reset Camera
      Camera_init(&m_Camera, nullptr, nullptr, 0.0f, 0.0f);
    }

    // Load Entities

    std::size_t num_entities;

    if (serializer.pushArray("m_Entities", num_entities))
    {
      if (serializer.mode() == SerializerMode::LOADING)
      {
        while (!m_RootEntities.isEmpty())
        {
          m_RootEntities.back()->destroy();
        }

        m_RootEntities.clear();
        m_RootEntities.reserve(num_entities);

        for (std::size_t i = 0; i < num_entities; ++i)
        {
          addEntity(nullptr);
        }
      }
      // else
      // {
      //   num_entities = m_RootEntities.size();
      // }

      for (Entity* const entity : m_RootEntities)
      {
        if (serializer.pushObject(entity->name()))
        {
          entity->serialize(serializer);
          serializer.popObject();
        }
      }

      serializer.popArray();
    }
  }

  Scene::~Scene()
  {
    bfAnimation2D_destroyScene(m_Engine.animationSys().anim2DCtx(), m_AnimationScene);

    removeAllEntities();
  }

  bool AssetSceneInfo::load(Engine& engine)
  {
    Assets&      assets    = engine.assets();
    const String full_path = filePathAbs();
    File         file_in   = {full_path, file::FILE_MODE_READ};

    if (file_in)
    {
      LinearAllocatorScope scope       = {engine.tempMemory()};
      const TempBuffer     json_buffer = file_in.readAll(engine.tempMemoryNoFree());
      json::Value          json_value  = json::fromString(json_buffer.buffer(), json_buffer.size());
      Scene&               scene       = m_Payload.set<Scene>(engine);

      if (json_value.isObject())
      {
        JsonSerializerReader json_writer{assets, engine.tempMemoryNoFree(), json_value};

        if (json_writer.beginDocument(false))
        {
          scene.serialize(json_writer);
          json_writer.endDocument();
        }
      }

      return true;
    }

    return false;
  }

  bool AssetSceneInfo::save(Engine& engine, ISerializer& serializer)
  {
    (void)engine;

    Scene& scene = m_Payload.as<Scene>();

    scene.serialize(serializer);

    return true;
  }
}  // namespace bifrost