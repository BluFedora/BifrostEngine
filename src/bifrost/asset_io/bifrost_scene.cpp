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
#include "bifrost/ecs/bifrost_entity.hpp"               /* Entity               */
#include "bifrost/utility/bifrost_json.hpp"             /* json::Value          */

namespace bifrost
{
  Scene::Scene(IMemoryManager& memory) :
    BaseObject<Scene>{},
    m_Memory{memory},
    m_RootEntities{memory},
    m_ActiveComponents{memory},
    m_InactiveComponents{memory}
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
    m_Memory.deallocateT(entity);
  }

  void Scene::serialize(ISerializer& serializer)
  {
    std::size_t num_entities;
    if (serializer.pushArray("m_Entities", num_entities))
    {
      if (serializer.mode() == SerializerMode::LOADING)
      {
        m_RootEntities.resize(num_entities);
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

  Entity* Scene::createEntity(const StringRange& name)
  {
    return m_Memory.allocateT<Entity>(*this, name);
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
