/******************************************************************************/
/*!
 * @file   bifrost_scene.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   This is where Entities live in the engine.
 *   Also contains the storage for the components.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_SCENE_HPP
#define BF_SCENE_HPP

#include "bf/bifrost_math.h"                             /* Vec3f, Mat4x4     */
#include "bf/core/bifrost_base_object.hpp"               /* BaseObject<T>     */
#include "bf/data_structures/bifrost_intrusive_list.hpp" /* ListView<T>       */
#include "bf/ecs/bf_component_storage.hpp"               /* ComponentStorage  */
#include "bf/ecs/bifrost_collision_system.hpp"           /* BVH               */

namespace bf
{
  class BaseBehavior;
  class DebugRenderer;
  class Engine;
  class Entity;
  class bfPureInterface(IBehavior);

  using Camera = BifrostCamera;

  /*!
   * @brief
   *  Hold entities along with any associated Component data.
   */
  class Scene final : public BaseAsset<Scene>
  {
    friend class BaseBehavior;
    friend class Engine;
    friend class Entity;
    BF_META_FRIEND;

   public:
    bool m_DoDebugDraw = true;  // TODO(SR): Remove this.

   private:
    Engine&              m_Engine;  // TODO(SR): Remove this.
    IMemoryManager&      m_Memory;
    ListView<Entity>     m_RootEntities;
    ComponentStorage     m_ActiveComponents;
    ComponentStorage     m_InactiveComponents;
    Array<BaseBehavior*> m_ActiveBehaviors;
    BVH                  m_BVHTree;
    Camera               m_Camera;
    bfTransform*         m_DirtyList;

   public:
    explicit Scene(Engine& engine);

    // Meta

    ClassID::Type classID() const override { return ClassID::SCENE_ASSET; }
    void          reflect(ISerializer& serializer) override;

    // Accessors

    Engine&       engine() const { return m_Engine; }
    const Camera& camera() const { return m_Camera; }
    Camera&       camera() { return m_Camera; }

    // Entity Management

    const ListView<Entity>& rootEntities() const { return m_RootEntities; }
    EntityRef               addEntity(const StringRange& name = "Untitled");
    EntityRef               findEntity(const StringRange& name) const;
    void                    removeEntity(Entity* entity);
    void                    removeAllEntities();
    BVH&                    bvh() { return m_BVHTree; }

    void update(LinearAllocator& temp, DebugRenderer& dbg_renderer);

    // Component

    template<typename T>
    DenseMap<ComponentHandleImpl<T>>& components()
    {
      return m_ActiveComponents.get<ComponentHandleImpl<T>>();
    }

    template<typename T>
    const DenseMap<ComponentHandleImpl<T>>& components() const
    {
      return m_ActiveComponents.get<ComponentHandleImpl<T>>();
    }

    // Behavior

    const Array<BaseBehavior*>& behaviors() const { return m_ActiveBehaviors; }

    // Runtime

    void startup();
    void shutdown();

    ~Scene() override;

   private:
    void updateDirtyListTransforms();
  };

  BIFROST_META_REGISTER(bf::Scene)
  {
    BIFROST_META_BEGIN()
      BIFROST_META_MEMBERS(
       class_info<Scene>("Scene")  //
      )
    BIFROST_META_END()
  }

  using SceneAsset = Scene;

  class SceneDocument : public IDocument
  {
   public:
    SceneAsset* m_SceneAsset = nullptr;

   private:
    AssetStatus onLoad() override;
    void        onUnload() override;
    void        onSaveAsset() override;
  };

  void assetImportScene(AssetImportCtx& ctx);
}  // namespace bf

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

#endif /* BF_SCENE_HPP */
