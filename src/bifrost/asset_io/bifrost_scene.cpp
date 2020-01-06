#include "bifrost/asset_io/bifrost_scene.hpp"

#include "bifrost/ecs/bifrost_entity.hpp" /* Entity */

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
    m_Memory.dealloc_t(entity);
    m_RootEntities.removeAt(m_RootEntities.find(entity));
  }

  Scene::~Scene()
  {
    for (auto* const comp_storage : m_ComponentStorage)
    {
      m_Memory.dealloc_t(comp_storage);
    }
  }

  Entity* Scene::createEntity(std::string_view name)
  {
    return m_Memory.alloc_t<Entity>(*this, name);
  }
}  // namespace bifrost
