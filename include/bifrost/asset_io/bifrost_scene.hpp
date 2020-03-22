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

#include "bifrost/bifrost_math.h"                    /* Vec3f, Mat4x4     */
#include "bifrost/core/bifrost_base_object.hpp"      /* BaseObject<T>     */
#include "bifrost/ecs/bifrost_component_storage.hpp" /* ComponentStorage  */
#include "bifrost_asset_handle.hpp"                  /* AssetInfo<T1, T2> */

namespace bifrost
{
  class Entity;

  /*!
   * @brief
   *  Hold entities along with any associated Component data.
   */
  class Scene final : public BaseObject<Scene>
  {
    BIFROST_META_FRIEND;
    friend class Entity;

   private:
    IMemoryManager&  m_Memory;
    Array<Entity*>   m_RootEntities;
    ComponentStorage m_ActiveComponents;
    ComponentStorage m_InactiveComponents;

   public:
    explicit Scene(IMemoryManager& memory);

    // Entity Management

    const Array<Entity*>& rootEntities() const { return m_RootEntities; }
    Entity*               addEntity(const StringRange& name);
    void                  removeEntity(Entity* entity);

    // Component

    template<typename T>
    DenseMap<T>& components()
    {
      return m_ActiveComponents.get<T>();
    }

    template<typename T>
    const DenseMap<T>& components() const
    {
      return m_ActiveComponents.get<T>();
    }

    // Extra

    void serialize(ISerializer& serializer);

   private:
    Entity* createEntity(const StringRange& name);
  };

  class AssetSceneInfo final : public AssetInfo<Scene, AssetSceneInfo>
  {
   private:
    using BaseT = AssetInfo<Scene, AssetSceneInfo>;

   public:
    using BaseT::BaseT;

    bool load(Engine& engine) override;
    bool save(Engine& engine, ISerializer& serializer) override;
  };

  using AssetSceneHandle = AssetHandle<Scene>;
}  // namespace bifrost

BIFROST_META_REGISTER(Quaternionf){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<Quaternionf>("Quaternionf"),  //
   ctor<>(),                                //
   field("x", &Quaternionf::x),             //
   field("y", &Quaternionf::y),             //
   field("z", &Quaternionf::z),             //
   field("w", &Quaternionf::w)              //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bifrost::Scene){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<Scene>("Scene"),                               //
   ctor<IMemoryManager&>(),                                  //
   field_readonly("m_RootEntities", &Scene::m_RootEntities)  //
   )
   BIFROST_META_END()}

BIFROST_META_REGISTER(bifrost::AssetSceneInfo){
 BIFROST_META_BEGIN()
  BIFROST_META_MEMBERS(
   class_info<AssetSceneInfo>("AssetSceneInfo"),  //
   ctor<StringRange, BifrostUUID>())
   BIFROST_META_END()}

BIFROST_META_REGISTER(Vec3f)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<Vec3f>("Vec3f"),  //
     ctor<>(),                    //
     field("x", &Vec3f::x),       //
     field("y", &Vec3f::y),       //
     field("z", &Vec3f::z),       //
     field("w", &Vec3f::w)        //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_SCENE_HPP */
