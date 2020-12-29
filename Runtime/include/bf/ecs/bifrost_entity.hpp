/******************************************************************************/
/*!
 * @file   bifrost_entity.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   This engine's concept of a GameObject / Actor.
 *   An entity is a bag of components with a 'BifrostTransform' and a name.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BIFROST_ENTITY_HPP
#define BIFROST_ENTITY_HPP

#include "bf/core/bifrost_base_object.hpp"                // BaseObject
#include "bf/data_structures/bifrost_intrusive_list.hpp"  // ListView
#include "bf/math/bifrost_transform.h"                    // BifrostTransform
#include "bifrost_collision_system.hpp"                   // BVHNodeOffset
#include "bifrost_component_handle_storage.hpp"           // ComponentHandleStorage, ComponentActiveStorage
#include "bifrost_component_storage.hpp"                  // ComponentStorage

#include <atomic>  // std::atomic_uint32_t

namespace bf::meta
{
  template<>
  inline const auto& Meta::registerMembers<BifrostTransform>()
  {
    static auto member_ptrs = members(
     class_info<BifrostTransform>("Transform"),
     ctor<>(),
     field("Origin", &BifrostTransform::origin),            //
     field("Position", &BifrostTransform::local_position),  //
     field("Rotation", &BifrostTransform::local_rotation),  //
     field("Scale", &BifrostTransform::local_scale)         //
    );

    return member_ptrs;
  }
}  // namespace bf::meta

namespace bf
{
  class BaseBehavior;
  class ISerializer;
  class Entity;
  class Scene;
  class bfPureInterface(IBehavior);

  namespace gc
  {
    struct GCContext;
  }

  using EntityList   = ListView<Entity>;
  using BehaviorList = Array<BaseBehavior*>;

  // clang-format off
  class Entity final : public BaseObject<Entity>, private NonCopyMoveable<Entity>
  // clang-format on
  {
    BIFROST_META_FRIEND;
    friend class Scene;
    friend struct gc::GCContext;

   public:
    static constexpr std::uint8_t IS_PENDING_DELETED     = bfBit(0);  //!<
    static constexpr std::uint8_t IS_HIDDEN_IN_HIERARCHY = bfBit(1);  //!<
    static constexpr std::uint8_t IS_PREFAB_INSTANCE     = bfBit(2);  //!<
    static constexpr std::uint8_t IS_ACTIVE              = bfBit(3);  //!<
    static constexpr std::uint8_t IS_SERIALIZABLE        = bfBit(4);  //!<

   private:
    Scene&                 m_OwningScene;              //!<
    String                 m_Name;                     //!<
    bfTransformID          m_Transform;                //!<
    Entity*                m_Parent;                   //!<
    EntityList             m_Children;                 //!<
    Node<Entity>           m_Hierarchy;                //!<
    ComponentHandleStorage m_ComponentHandles;         //!<
    ComponentActiveStorage m_ComponentActiveStates;    //!<
    ComponentActiveStorage m_ComponentInActiveStates;  //!<
    BVHNodeOffset          m_BHVNode;                  //!<
    BehaviorList           m_Behaviors;                //!<
    std::atomic_uint32_t   m_RefCount;                 //!<
    Node<Entity>           m_GCList;                   //!<
    std::uint8_t           m_Flags;                    //!<
    bfUUIDNumber           m_UUID;                     //!< This uuid will remain unset until the first use through "Entity::uuid".

   public:
    Entity(Scene& scene, const StringRange& name);

    // Accessors

    [[nodiscard]] Engine&             engine() const;
    [[nodiscard]] Scene&              scene() const { return m_OwningScene; }
    [[nodiscard]] const String&       name() const { return m_Name; }
    [[nodiscard]] BifrostTransform&   transform() const;
    [[nodiscard]] BVHNode&            bvhNode() const;
    [[nodiscard]] EntityList&         children() { return m_Children; }
    [[nodiscard]] BVHNodeOffset       bvhID() const { return m_BHVNode; }
    [[nodiscard]] const BehaviorList& behaviors() const { return m_Behaviors; }
    [[nodiscard]] bool                hasUUID() const;
    [[nodiscard]] const bfUUIDNumber& uuid();

    // General API

    [[nodiscard]] bool isActive() const { return isActiveParent() && isActiveSelf(); }
    [[nodiscard]] bool isActiveParent() const { return (!m_Parent || m_Parent->isActive()); }
    [[nodiscard]] bool isActiveSelf() const { return isFlagSet(IS_ACTIVE); }
    void               setActive(bool is_active);

    // Child API

    // This API needs to make it nearly impossible to leak children.
    // This is why there is no "removeChild" function publicly available.
    // To remove a child you must destroy the object itself which
    // will enforce that it does not dangle outside of the hierarchy.

    EntityRef addChild(const StringRange& name);
    void      setParent(Entity* new_parent);

    // Component API

    template<typename T>
    T& add()
    {
      if (!has<T>())
      {
        const bool is_active = isActive();
        auto&      handle    = componentHandle<T>();

        handle.handle = getComponentList<T>(is_active).add(*this);
        setComponentActiveState<T>(is_active);

        if (is_active)
        {
          get<T>()->privateOnEnable(engine());
        }
      }

      return *get<T>();
    }

    template<typename T>
    T* get() const
    {
      const auto& handle = componentHandle<T>();

      if (handle.handle.isValid())
      {
        return &getComponentList<T>(getComponentActiveState<T>()).find(handle.handle);
      }

      return nullptr;
    }

    template<typename T>
    bool has() const
    {
      return get<T>() != nullptr;
    }

    template<typename T>
    bool isComponentActive() const
    {
      return getComponentActiveState<T>() && componentHandle<T>().handle.isValid();
    }

    //
    // Returns true if the operation succeeds to do anything.
    //
    template<typename T>
    bool setComponentActive(bool value)
    {
      if (isActive())
      {
        if (has<T>() && value != isComponentActive<T>())
        {
          auto& handle     = componentHandle<T>();
          T&    old_data   = *get<T>();
          auto& new_list   = getComponentList<T>(value);
          auto  new_handle = new_list.add(*this);
          T&    new_data   = new_list.find(new_handle);

          new_data = std::move(old_data);

          if (value)
          {
            new_data.privateOnEnable(engine());
          }
          else
          {
            new_data.privateOnDisable(engine());
          }

          // remove<T>();
          getComponentList<T>(!value).remove(handle.handle);

          handle.handle = new_handle;

          setComponentActiveState<T>(value);

          return true;
        }
      }

      return false;
    }

    //
    // These functions returns the active state of components before calling this function.
    //

    ComponentActiveStorage activateComponents();
    ComponentActiveStorage applyComponentActiveState(const ComponentActiveStorage& state);
    ComponentActiveStorage deactivateComponents();

    template<typename T>
    bool remove()
    {
      auto& handle = m_ComponentHandles.get<T>();

      if (handle.handle.isValid())
      {
        get<T>()->privateOnDisable(engine());
        getComponentList<T>(getComponentActiveState<T>()).remove(handle.handle);
        handle.handle = {};
        setComponentActiveState<T>(false);

        return true;
      }

      return false;
    }

    // Behavior API

    template<typename T>
    T* addBehavior()
    {
      static_assert(std::is_base_of_v<IBehavior, T>, "T must derive from the Behavior<T> class.");

      return (T*)addBehavior(meta::typeInfoGet<T>());
    }

    IBehavior*    addBehavior(const StringRange& name);
    BaseBehavior* addBehavior(meta::BaseClassMetaInfoPtr type);

    template<typename T>
    T* findBehavior()
    {
      return (T*)findBehaviorByType(meta::typeInfoGet<T>());
    }

    IBehavior* findBehavior(const StringRange& name) const;

    void activateBehaviors() const;
    void deactivateBehaviors() const;

    template<typename T>
    bool removeBehavior()
    {
      auto type_info = meta::typeInfoGet<T>();

      return type_info ? removeBehaviorFromList(type_info) : false;
    }

    bool removeBehavior(const StringRange& name);
    bool removeBehavior(IBehavior* behavior);

    // GC / Ref Count API

    [[nodiscard]] std::uint32_t refCount() const;
    void                        acquire();
    void                        release();

    void destroy();

    // Flags

    [[nodiscard]] bool isFlagSet(std::uint8_t flag) const { return m_Flags & flag; }
    void               setFlags(std::uint8_t flags) { m_Flags = flags; }
    void               addFlags(std::uint8_t flags) { m_Flags |= flags; }
    void               clearFlags(std::uint8_t flags) { m_Flags &= ~flags; }

    // Meta

    void serialize(ISerializer& serializer);

    ~Entity() override;

   private:
    void setActiveImpl(bool old_state, bool new_state);

    const BifrostTransform& metaGetTransform() const { return transform(); }
    // ReSharper disable once CppMemberFunctionMayBeConst
    void metaSetTransform(const BifrostTransform& value)
    {
      bfTransform_copyFrom(&transform(), &value);
    }

    template<typename T>
    DenseMap<T>& getComponentList(bool is_active) const
    {
      return sceneComponentStorage(is_active).get<T>();
    }

    template<typename T>
    ComponentHandle<T>& componentHandle()
    {
      return m_ComponentHandles.get<T>();
    }

    template<typename T>
    const ComponentHandle<T>& componentHandle() const
    {
      return m_ComponentHandles.get<T>();
    }

    template<typename T>
    bool getComponentActiveState() const
    {
      return m_ComponentActiveStates.get<T>().is_active;
    }

    template<typename T>
    void setComponentActiveState(bool value)
    {
      m_ComponentActiveStates.get<T>().is_active = value;
    }

    void        detachFromParent();
    void        attachToParent(Entity* new_parent);
    void        removeChild(Entity* child);
    IBehavior*  findBehaviorByType(meta::BaseClassMetaInfoPtr type) const;     // not found = nullptr
    std::size_t findBehaviorIdxByType(meta::BaseClassMetaInfoPtr type) const;  // not found = BIFROST_ARRAY_INVALID_INDEX
    bool        removeBehaviorFromList(meta::BaseClassMetaInfoPtr type);       // false if could not find behavior to be removed
    void        deleteBehavior(BaseBehavior* behavior) const;

    ComponentStorage& sceneComponentStorage(bool is_active) const;
    IMemoryManager&   sceneMemoryManager() const;

    void toggleFlags(std::uint8_t flags);

   private:
    friend void gc::collect(IMemoryManager& entity_memory);

   public: /* 'Private' Editor API */
    void    editorLinkEntity(Entity* old_parent);
    Entity* editorUnlinkEntity();
  };
}  // namespace bf

BIFROST_META_REGISTER(bf::Entity)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<Entity>("Entity"),  //
     // ctor<Scene&, const StringRange&>(),                                            // TODO(Shareef): Tries to Register Scene before it is fully defined :(
     field("m_Name", &Entity::m_Name),                                              //
     field("m_Flags", &Entity::m_Flags),                                            //
     field("m_UUID", &Entity::m_UUID),                                              //
     property("m_Transform", &Entity::metaGetTransform, &Entity::metaSetTransform)  //
    )
  BIFROST_META_END()
}

#endif /* BIFROST_ENTITY_HPP */
