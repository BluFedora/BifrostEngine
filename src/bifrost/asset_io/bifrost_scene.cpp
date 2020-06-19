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

namespace bifrost
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

  Scene::Scene(IMemoryManager& memory) :
    BaseObject<Scene>{},
    m_Memory{memory},
    m_RootEntities{memory},
    m_ActiveComponents{memory},
    m_InactiveComponents{memory},
    m_ActiveBehaviors{memory},
    m_BVHTree{memory},
    m_TransformSystem{memory}
  {
  }

  Entity* Scene::addEntity(const StringRange& name)
  {
    Entity* entity = createEntity(name);
    m_RootEntities.push(entity);

    return entity;
  }

  void Scene::removeEntity(Entity* entity)
  {
    m_RootEntities.removeAt(m_RootEntities.find(entity));
    destroyEntity(entity);
  }

  void Scene::update(LinearAllocator& temp, DebugRenderer& dbg_renderer)
  {
    for (Entity* entity : m_RootEntities)
    {
      m_BVHTree.markLeafDirty(entity->bvhID(), entity->transform());
    }

    m_BVHTree.endFrame(temp, true);

    m_BVHTree.traverse([&dbg_renderer](const BVHNode& node) {
      dbg_renderer.addAABB(
       (Vector3f(node.bounds.max[0], node.bounds.max[1], node.bounds.max[2]) + Vector3f(node.bounds.min[0], node.bounds.min[1], node.bounds.min[2])) * 0.5f,
       Vector3f(node.bounds.max[0], node.bounds.max[1], node.bounds.max[2]) - Vector3f(node.bounds.min[0], node.bounds.min[1], node.bounds.min[2]),
       bfColor4u_fromUint32(BIFROST_COLOR_CYAN));
    });
  }

  void Scene::serialize(ISerializer& serializer)
  {
    std::size_t num_entities;
    if (serializer.pushArray("m_Entities", num_entities))
    {
      if (serializer.mode() == SerializerMode::LOADING)
      {
        m_RootEntities.resize(num_entities);

        for (auto& entity : m_RootEntities)
        {
          if (entity)
          {
            destroyEntity(entity);
            entity = nullptr;
          }
        }
      }
      else
      {
        num_entities = m_RootEntities.size();
      }

      for (auto& entity : m_RootEntities)
      {
        if (!entity)
        {
          entity = createEntity(nullptr);
        }

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
    while (!m_RootEntities.isEmpty())
    {
      removeEntity(m_RootEntities.back());
    }
  }

  Entity* Scene::createEntity(const StringRange& name)
  {
    Entity* const entity = m_Memory.allocateT<Entity>(*this, name);

    entity->m_BHVNode = m_BVHTree.insert(entity, entity->transform());

    return entity;
  }

  void Scene::destroyEntity(Entity* entity) const
  {
    m_Memory.deallocateT(entity);
  }

  bool AssetSceneInfo::load(Engine& engine)
  {
    Assets&      assets    = engine.assets();
    const String full_path = assets.fullPath(*this);
    File         file_in   = {full_path, file::FILE_MODE_READ};

    if (file_in)
    {
      LinearAllocatorScope scope       = {engine.tempMemory()};
      const TempBuffer     json_buffer = file_in.readAll(engine.tempMemoryNoFree());
      json::Value          json_value  = json::fromString(json_buffer.buffer(), json_buffer.size());
      Scene&               scene       = m_Payload.set<Scene>(engine.mainMemory());

      if (json_value.isObject())
      {
        JsonSerializerReader json_writer{assets, engine.tempMemoryNoFree(), json_value};
        RefPatcherSerializer ref_patcher{json_writer};

        if (json_writer.beginDocument(false))
        {
          scene.serialize(json_writer);
          json_writer.endDocument();
        }

        if (ref_patcher.beginDocument(false))
        {
          scene.serialize(ref_patcher);
          ref_patcher.endDocument();
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
