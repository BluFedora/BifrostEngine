/******************************************************************************/
/*!
 * @file   bifrost_entity.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *   This engine's concept of a GameObject / Actor.
 *   An entity is a bag of components with a Transform and a name.
 *
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019-2020
 */
/******************************************************************************/
#ifndef BF_ENTITY_HPP
#define BF_ENTITY_HPP

#include "bf/ListView.hpp"                       // ListView
#include "bf/core/bifrost_base_object.hpp"       // BaseObject
#include "bf/math/bifrost_transform.h"           // bfTransform
#include "bifrost_collision_system.hpp"          // BVHNodeOffset
#include "bifrost_component_handle_storage.hpp"  // ComponentHandleStorage, ComponentActiveStorage
#include "bifrost_component_storage.hpp"         // ComponentStorage

#include <atomic>  // std::atomic_uint32_t

namespace bf::meta
{
  template<>
  inline const auto& Meta::registerMembers<bfTransform>()
  {
    static auto member_ptrs = members(
     class_info<bfTransform>("Transform"),
     ctor<>(),
     field("Origin", &bfTransform::origin),            //
     field("Position", &bfTransform::local_position),  //
     field("Rotation", &bfTransform::local_rotation),  //
     field("Scale", &bfTransform::local_scale)         //
    );

    return member_ptrs;
  }
}  // namespace bf::meta

namespace bf
{
  class BaseBehavior;
  class ISerializer;
  class Entity;
  class EntityRef;
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
    friend class EntityRef;

   public:
    static constexpr std::uint8_t IS_PENDING_DELETED      = bfBit(0);  //!<
    static constexpr std::uint8_t IS_HIDDEN_IN_HIERARCHY  = bfBit(1);  //!<
    static constexpr std::uint8_t IS_PREFAB_INSTANCE      = bfBit(2);  //!<
    static constexpr std::uint8_t IS_ACTIVE               = bfBit(3);  //!<
    static constexpr std::uint8_t IS_SERIALIZABLE         = bfBit(4);  //!<
    static constexpr std::uint8_t IS_ADOPTS_PARENT_ACTIVE = bfBit(5);  //!<

    //
    // If you have a pointer to a transform that is on an entity you can turn that back into an Entity.
    //
    static Entity* fromTransform(bfTransform* transform)
    {
      return reinterpret_cast<Entity*>(reinterpret_cast<char*>(transform) - offsetof(Entity, m_Transform));
    }

   private:
    Scene&                 m_OwningScene;            //!<
    String                 m_Name;                   //!<
    Entity*                m_Parent;                 //!<
    EntityList             m_Children;               //!<
    ListNode<Entity>       m_Hierarchy;              //!<
    ListNode<Entity>       m_GCList;                 //!<
    BehaviorList           m_Behaviors;              //!<
    ComponentHandleStorage m_ComponentHandles;       //!<
    bfTransform            m_Transform;              //!<
    std::atomic_uint32_t   m_RefCount;               //!<
    ComponentActiveStorage m_ComponentActiveStates;  //!<
    std::uint8_t           m_Flags;                  //!<
    bfUUIDNumber           m_UUID;                   //!< This uuid will remain unset until the first use through "Entity::uuid".

   public:
    Entity(Scene& scene, StringRange name);

    // Accessors

    [[nodiscard]] Engine&             engine() const;
    [[nodiscard]] Scene&              scene() const { return m_OwningScene; }
    [[nodiscard]] const String&       name() const { return m_Name; }
    void                              setName(StringRange value);
    [[nodiscard]] bfTransform&        transform() { return m_Transform; }
    [[nodiscard]] const bfTransform&  transform() const { return m_Transform; }
    [[nodiscard]] Entity*             parent() const { return m_Parent; }
    [[nodiscard]] EntityList&         children() { return m_Children; }
    [[nodiscard]] const BehaviorList& behaviors() const { return m_Behaviors; }
    [[nodiscard]] bool                hasUUID() const;
    [[nodiscard]] const bfUUIDNumber& uuid();

    // General API

    [[nodiscard]] bool isActive() const { return isActiveParent() && isActiveSelf(); }
    [[nodiscard]] bool isActiveParent() const;
    [[nodiscard]] bool isActiveSelf() const { return isFlagSet(IS_ACTIVE); }
    void               setActiveSelf(bool is_active_value);

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
        Engine&  engine    = this->engine();
        T* const component = get<T>();

        ComponentTraits::onCreate(*component, engine);

        if (is_active)
        {
          ComponentTraits::onEnable(*component, engine);
        }
      }

      return *get<T>();
    }

    template<typename T>
    T* get() const
    {
      return getImpl<T>(isActive());
    }

    template<typename T>
    bool has() const
    {
      return hasImpl<T>(isActive());
    }

    template<typename T>
    bool isComponentActive() const
    {
      return getComponentActiveState<T>() && componentHandle<T>().handle.isValid();
    }

    template<typename T>
    void setComponentActive(bool value)
    {
      const bool is_active = isActive();

      setComponentActiveImpl<T>(is_active, is_active, value);
    }

    template<typename T>
    bool remove()
    {
      auto& handle = m_ComponentHandles.get<T>();

      if (handle.handle.isValid())
      {
        Engine&  engine    = this->engine();
        T* const component = get<T>();

        // TODO(SR): This should not be called if the component was already inactive.
        ComponentTraits::onDisable(*component, engine);
        ComponentTraits::onDestroy(*component, engine);

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

      return static_cast<T*>(addBehavior(meta::typeInfoGet<T>()));
    }

    IBehavior*    addBehavior(const StringRange& name);
    BaseBehavior* addBehavior(meta::BaseClassMetaInfoPtr type);

    template<typename T>
    T* findBehavior()
    {
      return static_cast<T*>(findBehaviorByType(meta::typeInfoGet<T>()));
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

    // Flags

    [[nodiscard]] bool isFlagSet(std::uint8_t flag) const { return m_Flags & flag; }

    // GC / Ref Count API

    [[nodiscard]] std::uint32_t refCount() const;
    void                        acquire();
    void                        release();

    // Meta

    void reflect(ISerializer& serializer) override;

    // Runtime

    Entity* clone();
    void    startup();
    void    shutdown();
    void    destroy();

    // Misc

    ~Entity() override;

   private:
    // Flags
    void setFlags(std::uint8_t flags) { m_Flags = flags; }
    void addFlags(std::uint8_t flags) { m_Flags |= flags; }
    void clearFlags(std::uint8_t flags) { m_Flags &= ~flags; }

    void reevaluateActiveState(bool was_active, bool is_active);

    const bfTransform& metaGetTransform() const { return transform(); }
    // ReSharper disable once CppMemberFunctionMayBeConst
    void metaSetTransform(const bfTransform& value)
    {
      bfTransform_copyFrom(&transform(), &value);
    }

    template<typename T>
    T* getImpl(bool was_active) const
    {
      const auto& handle = componentHandle<T>();

      if (handle.handle.isValid())
      {
        const bool active_list = was_active && getComponentActiveState<T>();

        return &getComponentList<T>(active_list).find(handle.handle);
      }

      return nullptr;
    }

    template<typename T>
    bool hasImpl(bool was_active) const
    {
      return getImpl<T>(was_active) != nullptr;
    }

    // Returns whether or not the component changed lists.
    template<typename T>
    bool setComponentActiveImpl(bool was_active, bool is_active, bool value)
    {
      if (hasImpl<T>(was_active))
      {
        const bool src_list     = was_active && isComponentActive<T>();
        const bool dst_list     = is_active && value;
        const bool needs_change = src_list != dst_list;

        if (needs_change)
        {
          auto& handle     = componentHandle<T>();
          T&    old_data   = *getImpl<T>(was_active);
          auto& new_list   = getComponentList<T>(dst_list);
          auto  new_handle = new_list.add(*this);
          T&    new_data   = new_list.find(new_handle);

          new_data = std::move(old_data);

          if (value)
          {
            ComponentTraits::onEnable(new_data, engine());
          }
          else
          {
            ComponentTraits::onDisable(new_data, engine());
          }

          getComponentList<T>(src_list).remove(handle.handle);

          handle.handle = new_handle;
        }

        setComponentActiveState<T>(value);

        return needs_change;
      }

      return false;
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
  };
}  // namespace bf

BIFROST_META_REGISTER(bf::Entity)
{
  BIFROST_META_BEGIN()
    BIFROST_META_MEMBERS(
     class_info<Entity>("Entity"),                                                  //
     field("m_Name", &Entity::m_Name),                                              //
     field("m_Flags", &Entity::m_Flags),                                            //
     field("m_UUID", &Entity::m_UUID),                                              //
     property("m_Transform", &Entity::metaGetTransform, &Entity::metaSetTransform)  //
    )
  BIFROST_META_END()
}

#endif /* BF_ENTITY_HPP */
