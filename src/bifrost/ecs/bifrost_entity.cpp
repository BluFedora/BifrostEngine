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
#include "bifrost/ecs/bifrost_entity.hpp"

#include "bifrost/asset_io/bifrost_asset_handle.hpp"
#include "bifrost/asset_io/bifrost_scene.hpp"
#include "bifrost/debug/bifrost_dbg_logger.h"
#include "bifrost/ecs/bifrost_behavior.hpp"

namespace bifrost
{
  static constexpr StringRange k_SerializeComponentActiveKey   = "__Active__";
  static constexpr StringRange k_SerializeBehaviorClassNameKey = "__BehaviorClass__";

  Entity::Entity(Scene& scene, const StringRange& name) :
    m_OwningScene{scene},
    m_Name{name},
    m_Transform{scene.m_TransformSystem.createTransform()},
    m_Parent{nullptr},
    m_Children{&Entity::m_Hierarchy},
    m_Hierarchy{},
    m_ComponentHandles{},
    m_BHVNode{k_BVHNodeInvalidOffset},
    m_Behaviors{sceneMemoryManager()}
  {
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

  Entity* Entity::addChild(const StringRange& name)
  {
    Entity* child   = m_OwningScene.createEntity(name);
    child->m_Parent = this;
    m_Children.pushBack(*child);
    return child;
  }

  void Entity::setParent(Entity* new_parent)
  {
    if (m_Parent != new_parent)
    {
      if (m_Parent)
      {
        m_Parent->removeChild(this);
      }

      m_Parent = new_parent;

      if (new_parent)
      {
        new_parent->m_Children.pushBack(*this);
      }
      else
      {
        m_OwningScene.m_RootEntities.push(this);
      }
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
    std::size_t       index     = BIFROST_ARRAY_INVALID_INDEX;

    for (std::size_t i = 0; i < list_size; ++i)
    {
      IBehavior* behavior_ = m_Behaviors[i];

      if (behavior_ == behavior)
      {
        index = i;
        break;
      }
    }

    const bool was_found = index != BIFROST_ARRAY_INVALID_INDEX;

    if (was_found)
    {
      deleteBehavior(m_Behaviors[index]);
      m_Behaviors.removeAt(index);
    }

    return was_found;
  }

  void Entity::serialize(ISerializer& serializer)
  {
    serializer.serializeT(this);

    if (serializer.mode() != SerializerMode::INSPECTING)
    {
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
            if (serializer.pushObject("_"))
            {
              String class_name_str;

              serializer.serialize(k_SerializeBehaviorClassNameKey, class_name_str);

              IBehavior* const behavior = addBehavior(class_name_str);

              if (behavior)
              {
                behavior->serialize(serializer);
              }

              serializer.popObject();
            }
          }
        }
        else
        {
          for (IBehavior* behavior : m_Behaviors)
          {
            if (serializer.pushObject("_"))
            {
              const std::string_view class_name_sv  = behavior->type()->name();
              const StringRange      class_name_sr  = {class_name_sv.data(), class_name_sv.length()};
              String                 class_name_str = class_name_sr;

              serializer.serialize(k_SerializeBehaviorClassNameKey, class_name_str);
              behavior->serialize(serializer);

              serializer.popObject();
            }
          }
        }

        serializer.popArray();
      }
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
    for (IBehavior* const behavior : m_Behaviors)
    {
      deleteBehavior(behavior);
    }
    m_Behaviors.clear();

    // Children
    for (Entity& child : m_Children)
    {
      m_OwningScene.destroyEntity(&child);
    }

    // Transform
    scene().m_TransformSystem.destroyTransform(m_Transform);
    scene().m_BVHTree.remove(m_BHVNode);

    if (m_Parent)
    {
      m_Parent->removeChild(this);
    }
  }

  void Entity::removeChild(Entity* child)
  {
    m_Children.erase(m_Children.makeIterator(*child));
    child->m_Parent = nullptr;
  }

  BaseBehavior* Entity::addBehavior(meta::BaseClassMetaInfoPtr type)
  {
    BaseBehavior* const behavior = static_cast<BaseBehavior*>(meta::variantToCompatibleT<IBehavior*>(type->instantiate(sceneMemoryManager())));

    if (behavior)
    {
      m_OwningScene.m_ActiveBehaviors.push(behavior);
      m_Behaviors.push(behavior);
      behavior->setOwner(*this);
    }

    return behavior;
  }

  IBehavior* Entity::findBehaviorByType(meta::BaseClassMetaInfoPtr type) const
  {
    const std::size_t index = findBehaviorIdxByType(type);

    return index != BIFROST_ARRAY_INVALID_INDEX ? m_Behaviors[index] : nullptr;
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

    return BIFROST_ARRAY_INVALID_INDEX;
  }

  bool Entity::removeBehaviorFromList(meta::BaseClassMetaInfoPtr type)
  {
    const std::size_t index = findBehaviorIdxByType(type);

    if (index != BIFROST_ARRAY_INVALID_INDEX)
    {
      deleteBehavior(m_Behaviors[index]);
      m_Behaviors.removeAt(index);

      return true;
    }

    return false;
  }

  void Entity::deleteBehavior(IBehavior* behavior) const
  {
    const auto idx = m_OwningScene.m_ActiveBehaviors.find(behavior);

    if (idx != BIFROST_ARRAY_INVALID_INDEX)
    {
      m_OwningScene.m_ActiveBehaviors.swapAndPopAt(idx);
    }

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
}  // namespace bifrost
