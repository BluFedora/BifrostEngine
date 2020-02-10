/*!
 * @file   bifrost_scene.hpp
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
#ifndef BIFROST_SCENE_HPP
#define BIFROST_SCENE_HPP

#include "bifrost/bifrost_math.h"               /* Vec3f, Mat4x4                             */
#include "bifrost/core/bifrost_base_object.hpp" /* BaseObject<T>                             */
#include "bifrost/ecs/bifrost_component.hpp"    /* BaseComponentStorage, ComponentStorage<T> */

namespace bifrost
{
  class Entity;

  BIFROST_META_REGISTER(Vec3f){
   BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<Vec3f>("Vec3f"),  //
     ctor<>(),                    //
     field("x", &Vec3f::x),       //
     field("y", &Vec3f::y),       //
     field("z", &Vec3f::z),       //
     field("w", &Vec3f::w)        //
     )
     BIFROST_META_END()}

  BIFROST_META_REGISTER(Quaternionf)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<Quaternionf>("Quaternionf"),  //
       ctor<>(),                                //
       field("x", &Quaternionf::x),             //
       field("y", &Quaternionf::y),             //
       field("z", &Quaternionf::z),             //
       field("w", &Quaternionf::w)              //
      )
    BIFROST_META_END()
  }

  /*!
   * @brief
   *  Hold entities along with any associated Component data.
   */
  class Scene final : public BaseObject<Scene>
  {
    BIFROST_META_FRIEND;
    friend class Entity;

   private:
    IMemoryManager&              m_Memory;
    Array<Entity*>               m_RootEntities;
    Array<BaseComponentStorage*> m_ComponentStorage;

   public:
    explicit Scene(IMemoryManager& memory) :
      m_Memory{memory},
      m_RootEntities{memory},
      m_ComponentStorage{memory}
    {
    }

    // Entity Management
    const Array<Entity*>& rootEntities() const { return m_RootEntities; }
    Entity*               addEntity(std::string_view name);
    void                  removeEntity(Entity* entity);

    // Component Management

    template<typename T, typename F>
    void forEachComponent(F&& fn)
    {
      const std::uint32_t cid = T::s_ComponentID;

      if (cid < m_ComponentStorage.size())
      {
        BaseComponentStorage* storage = m_ComponentStorage[cid];

        if (storage)
        {
          const std::size_t num_components = storage->numComponents();

          for (std::size_t i = 0; i < num_components; ++i)
          {
            fn(static_cast<T*>(storage->componentAt(i)));
          }
        }
      }
    }

    ~Scene() override;

   private:
    template<typename T>
    dense_map::ID_t addComponent(Entity& owner)
    {
      const std::uint32_t cid = T::s_ComponentID;

      if (m_ComponentStorage.size() <= cid)
      {
        m_ComponentStorage.resize(cid + 1);

        m_ComponentStorage[cid] = m_Memory.alloc_t<ComponentStorage<T>>(m_Memory);
      }

      return m_ComponentStorage[cid]->createComponent(owner);
    }

    Entity* createEntity(std::string_view name);
  };
}  // namespace bifrost

BIFROST_META_REGISTER(bifrost::Scene)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<Scene>("Scene"),                                                                //
     ctor<IMemoryManager&>(),                                                                   //
     field_readonly("m_RootEntities", &Scene::m_RootEntities, offsetof(Scene, m_RootEntities))  //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_SCENE_HPP */
