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

#include "bifrost/core/bifrost_engine.hpp"  /* Engine      */
#include "bifrost/ecs/bifrost_entity.hpp"   /* Entity      */
#include "bifrost/utility/bifrost_json.hpp" /* json::Value */

namespace bifrost
{
  Entity* Scene::addEntity(const std::string_view name)
  {
    Entity* entity = createEntity(name);
    m_RootEntities.push(entity);
    return entity;
  }

  void Scene::removeEntity(Entity* entity)
  {
    m_Memory.deallocateT(entity);
    m_RootEntities.removeAt(m_RootEntities.find(entity));
  }

  Scene::~Scene()
  {
    for (auto* const comp_storage : m_ComponentStorage)
    {
      m_Memory.deallocateT(comp_storage);
    }
  }

  Entity* Scene::createEntity(std::string_view name)
  {
    return m_Memory.allocateT<Entity>(*this, name);
  }

  bool AssetSceneInfo::load(Engine& engine)
  {
    const String full_path = engine.assets().fullPath(*this);
    File         file_in   = {full_path, file::FILE_MODE_READ};

    if (file_in)
    {
      LinearAllocatorScope scope       = {engine.tempMemory()};
      const TempBuffer     json_buffer = file_in.readAll(engine.tempMemoryNoFree());
      json::Value          json_value  = json::fromString(json_buffer.buffer(), json_buffer.size());
      Scene&               scene       = m_Payload.set<Scene>(engine.mainMemory());

      if (json_value.isObject())
      {
      }

      return true;
    }

    return false;
  }
}  // namespace bifrost
