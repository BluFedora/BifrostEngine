/******************************************************************************/
/*!
* @file   bifrost_entity.cpp
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
#include "bf/ecs/bifrost_entity.hpp"

#include "bf/Engine.hpp"
#include "bf/asset_io/bf_iserializer.hpp"
#include "bf/asset_io/bifrost_json_serializer.hpp"
#include "bf/asset_io/bifrost_scene.hpp"
#include "bf/debug/bifrost_dbg_logger.h"
#include "bf/ecs/bifrost_behavior.hpp"

namespace bf
{
  static constexpr StringRange k_SerializeComponentActiveKey   = "__Active__";
  static constexpr StringRange k_SerializeBehaviorClassNameKey = "__BehaviorClass__";
  static constexpr StringRange k_SerializeArrayIndexKey        = "__Idx__";

  Entity::Entity(Scene& scene, StringRange name) :
    m_OwningScene{scene},
    m_Name{name},
    m_Parent{nullptr},
    m_Children{&Entity::m_Hierarchy},
    m_Hierarchy{},
    m_GCList{},
    m_Behaviors{sceneMemoryManager()},
    m_ComponentHandles{},
    m_Transform{scene.m_TransformSystem.createTransform()},
    m_RefCount{ATOMIC_VAR_INIT(0)},
    m_BHVNode{scene.m_BVHTree.insert(this, transform())},
    m_ComponentActiveStates{},
    m_Flags{IS_ACTIVE | IS_SERIALIZABLE},
    m_UUID{bfUUID_makeEmpty().as_number}
  {
  }

  Engine& Entity::engine() const
  {
    return scene().engine();
  }

  BifrostTransform& Entity::transform() const
  {
    IBifrostTransformSystem& transform_system = scene().m_TransformSystem;

    return *transform_system.transformFromID(&transform_system, m_Transform);
  }

  BVHNode& Entity::bvhNode() const
  {
    return m_OwningScene.m_BVHTree.nodes[m_BHVNode];
  }

  bool Entity::hasUUID() const
  {
    return !bfUUID_numberIsEmpty(&m_UUID);
  }

  const bfUUIDNumber& Entity::uuid()
  {
    if (!hasUUID())
    {
      m_UUID = bfUUID_generate().as_number;

      // Make sure this ID is actually unique.
      while (gc::hasUUID(m_UUID))
      {
        m_UUID = bfUUID_generate().as_number;
      }

      gc::registerEntity(*this);
    }

    return m_UUID;
  }

  void Entity::setActiveSelf(bool is_active_value)
  {
    if (isActiveSelf() != is_active_value)
    {
      const bool was_active = isActive();
      toggleFlags(IS_ACTIVE);
      const bool is_active = isActive();

      reevaluateActiveState(was_active, is_active);
    }
  }

  EntityRef Entity::addChild(const StringRange& name)
  {
    EntityRef child = EntityRef{sceneMemoryManager().allocateT<Entity>(scene(), name)};

    child->attachToParent(this);

    return child;
  }

  void Entity::setParent(Entity* new_parent)
  {
    if (m_Parent != new_parent)
    {
      detachFromParent();
      attachToParent(new_parent);
    }
  }

  IBehavior* Entity::addBehavior(const StringRange& name)
  {
    const meta::BaseClassMetaInfoPtr info = meta::TypeInfoFromName(std::string_view{name.bgn, name.length()});

    if (info)
    {
      IBehavior* const behavior = addBehavior(info);

      if (behavior)
      {
        return behavior;
      }

      bfLogWarn("Failed to allocate memory for behavior (%.*s).", int(name.length()), name.begin());
    }
    else
    {
      bfLogWarn("Failed to create behavior from the name (%.*s).", int(name.length()), name.begin());
    }

    return nullptr;
  }

  IBehavior* Entity::findBehavior(const StringRange& name) const
  {
    const meta::BaseClassMetaInfoPtr info = meta::TypeInfoFromName(std::string_view{name.bgn, name.length()});

    if (info)
    {
      return findBehaviorByType(info);
    }

    return nullptr;
  }

  void Entity::activateBehaviors() const
  {
    for (BaseBehavior* const behavior : behaviors())
    {
      behavior->setActive(true);
    }
  }

  void Entity::deactivateBehaviors() const
  {
    for (BaseBehavior* const behavior : behaviors())
    {
      behavior->setActive(false);
    }
  }

  bool Entity::removeBehavior(const StringRange& name)
  {
    const meta::BaseClassMetaInfoPtr info = meta::TypeInfoFromName(std::string_view{name.bgn, name.length()});

    if (info)
    {
      return removeBehaviorFromList(info);
    }

    return false;
  }

  bool Entity::removeBehavior(IBehavior* behavior)
  {
    const std::size_t list_size = m_Behaviors.size();
    std::size_t       index     = k_bfArrayInvalidIndex;

    for (std::size_t i = 0; i < list_size; ++i)
    {
      IBehavior* behavior_ = m_Behaviors[i];

      if (behavior_ == behavior)
      {
        index = i;
        break;
      }
    }

    const bool was_found = index != k_bfArrayInvalidIndex;

    if (was_found)
    {
      deleteBehavior(m_Behaviors[index]);
      m_Behaviors.removeAt(index);
    }

    return was_found;
  }

  std::uint32_t Entity::refCount() const
  {
    return m_RefCount.load();
  }

  void Entity::acquire()
  {
    ++m_RefCount;
  }

  void Entity::release()
  {
    assert(m_RefCount > 0);

    --m_RefCount;
  }

  Entity* Entity::clone()
  {
    Engine& engine   = this->engine();
    Assets& assets   = engine.assets();
    auto&   temp_mem = engine.tempMemory();

    JsonSerializerWriter serializer{temp_mem};

    if (serializer.beginDocument(false))
    {
      reflect(serializer);
      serializer.endDocument();

      JsonSerializerReader deserializer{assets, temp_mem, serializer.document()};

      if (deserializer.beginDocument(false))
      {
        Entity* const clone = parent() ? parent()->addChild(name()) : scene().addEntity(name());

        clone->reflect(deserializer);
        deserializer.endDocument();

        // TODO(SR): This should not be needed for the runtime, this is purely an editor thing.
        assets.markDirty(&scene());

        return clone;
      }
    }

    return nullptr;
  }

  void Entity::reflect(ISerializer& serializer)
  {
    serializer.serializeT(this);

    if (serializer.mode() == SerializerMode::LOADING)
    {
      if (hasUUID() && !gc::hasUUID(m_UUID))
      {
        gc::registerEntity(*this);
      }
    }

    if (serializer.mode() != SerializerMode::INSPECTING)
    {
      std::size_t num_children;
      if (serializer.pushArray("m_Children", num_children))
      {
        if (serializer.mode() == SerializerMode::LOADING)
        {
          for (std::size_t i = 0; i < num_children; ++i)
          {
            if (serializer.pushObject(k_SerializeArrayIndexKey))
            {
              Entity* const child = addChild(nullptr);
              child->reflect(serializer);
              serializer.popObject();
            }
          }
        }
        else
        {
          for (Entity& child : m_Children)
          {
            if (serializer.pushObject(k_SerializeArrayIndexKey))
            {
              child.reflect(serializer);
              serializer.popObject();
            }
          }
        }

        serializer.popArray();
      }

      if (serializer.pushObject("m_Components"))
      {
        ComponentStorage::forEachType([&serializer, this](auto t) {
          using T = bfForEachTemplateT(t);

          const StringRange name = g_EngineComponentInfo[bfForEachTemplateIndex(t)].name;

          T* component;

          if (serializer.mode() == SerializerMode::LOADING && serializer.hasKey(name))
          {
            component = &add<T>();
          }
          else
          {
            component = get<T>();
          }

          if (component && serializer.pushObject(name))
          {
            bool is_active = isComponentActive<T>();
            serializer.serialize(k_SerializeComponentActiveKey, is_active);
            serializer.serializeT(component);
            setComponentActive<T>(is_active);
            serializer.popObject();
          }
        });

        serializer.popObject();
      }

      std::size_t num_behaviors;
      if (serializer.pushArray("m_Behaviors", num_behaviors))
      {
        if (serializer.mode() == SerializerMode::LOADING)
        {
          for (std::size_t i = 0; i < num_behaviors; ++i)
          {
            if (serializer.pushObject(k_SerializeArrayIndexKey))
            {
              String class_name_str;

              serializer.serialize(k_SerializeBehaviorClassNameKey, class_name_str);

              IBehavior* const behavior = addBehavior(class_name_str);

              if (behavior)
              {
                behavior->reflect(serializer);
              }

              serializer.popObject();
            }
          }
        }
        else
        {
          for (IBehavior* behavior : m_Behaviors)
          {
            if (serializer.pushObject(k_SerializeArrayIndexKey))
            {
              const std::string_view class_name_sv  = behavior->type()->name();
              const StringRange      class_name_sr  = {class_name_sv.data(), class_name_sv.length()};
              String                 class_name_str = class_name_sr;

              serializer.serialize(k_SerializeBehaviorClassNameKey, class_name_str);
              behavior->reflect(serializer);

              serializer.popObject();
            }
          }
        }

        serializer.popArray();
      }
    }
  }

  void Entity::startup()
  {
    for (BaseBehavior* behavior : m_Behaviors)
    {
      if (behavior->isActive())
      {
        behavior->onEnable();
      }
    }

    for (Entity& child : m_Children)
    {
      child.startup();
    }
  }

  void Entity::shutdown()
  {
    for (BaseBehavior* behavior : m_Behaviors)
    {
      behavior->setActive(false);
    }

    for (Entity& child : m_Children)
    {
      child.shutdown();
    }
  }

  void Entity::destroy()
  {
    if (!isFlagSet(IS_PENDING_DELETED))
    {
      addFlags(IS_PENDING_DELETED);

      while (!m_Children.isEmpty())
      {
        m_Children.back().destroy();
      }

      detachFromParent();

      gc::removeEntity(*this);
    }
  }

  Entity::~Entity()
  {
    // Components
    ComponentStorage::forEachType([this](auto t) {
      using T = bfForEachTemplateT(t);
      remove<T>();
    });

    // Behaviors
    for (BaseBehavior* const behavior : m_Behaviors)
    {
      deleteBehavior(behavior);
    }
    m_Behaviors.clear();

    // Children

    if (m_Parent)
    {
      m_Parent->removeChild(this);
    }

    while (!m_Children.isEmpty())
    {
      m_Children.back().destroy();
    }

    // Transform
    scene().m_TransformSystem.destroyTransform(m_Transform);
    scene().m_BVHTree.remove(m_BHVNode);
  }

  void Entity::reevaluateActiveState(bool was_active, bool is_active)
  {
    if (was_active != is_active)
    {
      ComponentStorage::forEachType([this, was_active, is_active](auto t) {
        using T = bfForEachTemplateT(t);
        // A component is active if both the Entity itself is active and it is active.
        setComponentActiveImpl<T>(was_active, is_active, isComponentActive<T>());
      });

      for (Entity& child : m_Children)
      {
        const bool is_child_active_self = child.isActiveSelf();
        const bool was_child_active     = was_active && is_child_active_self;
        const bool is_child_active      = is_active && is_child_active_self;

        child.reevaluateActiveState(was_child_active, is_child_active);
      }
    }
  }

  void Entity::detachFromParent()
  {
    if (m_Parent)
    {
      const bool was_active = isActive();
      m_Parent->removeChild(this);
      const bool is_active = isActive();

      reevaluateActiveState(was_active, is_active);
    }
    else
    {
      auto& root_entities = m_OwningScene.m_RootEntities;

      root_entities.removeAt(root_entities.find(this));
    }
  }

  void Entity::attachToParent(Entity* new_parent)
  {
    m_Parent = new_parent;

    if (new_parent)
    {
      bfTransform_setParent(&transform(), &new_parent->transform());
      new_parent->m_Children.pushBack(*this);
    }
    else
    {
      bfTransform_setParent(&transform(), nullptr);
      scene().m_RootEntities.push(this);
    }
  }

  void Entity::removeChild(Entity* child)
  {
    assert(child->m_Parent == this);

    m_Children.erase(m_Children.makeIterator(*child));
    child->m_Parent = nullptr;
  }

  BaseBehavior* Entity::addBehavior(meta::BaseClassMetaInfoPtr type)
  {
    BaseBehavior* const behavior = static_cast<BaseBehavior*>(meta::variantToCompatibleT<IBehavior*>(type->instantiate(sceneMemoryManager())));

    if (behavior)
    {
      m_Behaviors.push(behavior);
      behavior->setOwner(*this);

      if (isActive())
      {
        behavior->setActive(true);
      }
    }

    return behavior;
  }

  IBehavior* Entity::findBehaviorByType(meta::BaseClassMetaInfoPtr type) const
  {
    const std::size_t index = findBehaviorIdxByType(type);

    return index != k_bfArrayInvalidIndex ? m_Behaviors[index] : nullptr;
  }

  std::size_t Entity::findBehaviorIdxByType(meta::BaseClassMetaInfoPtr type) const
  {
    const std::size_t list_size = m_Behaviors.size();

    for (std::size_t i = 0; i < list_size; ++i)
    {
      IBehavior* const behavior = m_Behaviors[i];

      if (behavior->type() == type)
      {
        return i;
      }
    }

    return k_bfArrayInvalidIndex;
  }

  bool Entity::removeBehaviorFromList(meta::BaseClassMetaInfoPtr type)
  {
    const std::size_t index = findBehaviorIdxByType(type);

    if (index != k_bfArrayInvalidIndex)
    {
      deleteBehavior(m_Behaviors[index]);
      m_Behaviors.removeAt(index);

      return true;
    }

    return false;
  }

  void Entity::deleteBehavior(BaseBehavior* behavior) const
  {
    behavior->setActive(false);
    sceneMemoryManager().deallocateT(behavior);
  }

  ComponentStorage& Entity::sceneComponentStorage(bool is_active) const
  {
    return is_active ? m_OwningScene.m_ActiveComponents : m_OwningScene.m_InactiveComponents;
  }

  IMemoryManager& Entity::sceneMemoryManager() const
  {
    return m_OwningScene.m_Memory;
  }

  void Entity::toggleFlags(std::uint8_t flags)
  {
    m_Flags ^= flags;
  }

  void Entity::editorLinkEntity(Entity* old_parent)
  {
    if (old_parent)
    {
      bfTransform_setParent(&transform(), &old_parent->transform());
      old_parent->m_Children.pushBack(*this);
    }
    else
    {
      m_OwningScene.m_RootEntities.push(this);
    }

    m_Parent = old_parent;
  }

  Entity* Entity::editorUnlinkEntity()
  {
    Entity* old_parent = m_Parent;

    detachFromParent();

    return old_parent;
  }
}  // namespace bf
