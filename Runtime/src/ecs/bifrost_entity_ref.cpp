/******************************************************************************/
/*!
 * @file   bifrost_entity_ref.cpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  This is for having safe refences to Entities even if they get deleted.
 *
 * @version 0.0.1
 * @date    2020-06-09
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#include "bf/ecs/bifrost_entity_ref.hpp"

#include "bf/ecs/bifrost_entity.hpp"
#include "bf/utility/bifrost_uuid.hpp"

namespace bf
{
  EntityRef::EntityRef(Entity* object) noexcept :
    m_ID{bfUUID_makeEmpty().as_number},
    m_CachedEntity{nullptr}
  {
    safeRef(object);
  }

  EntityRef::EntityRef(Entity& object) noexcept :
    EntityRef{&object}
  {
  }

  EntityRef::EntityRef(std::nullptr_t) noexcept :
    m_ID{bfUUID_makeEmpty().as_number},
    m_CachedEntity{nullptr}
  {
  }

  EntityRef::EntityRef(const EntityRef& rhs) noexcept :
    m_ID{rhs.m_ID},
    m_CachedEntity{nullptr}
  {
    safeRef(rhs.m_CachedEntity);
  }

  EntityRef::EntityRef(EntityRef&& rhs) noexcept :
    m_ID{std::exchange(rhs.m_ID, bfUUID_makeEmpty().as_number)},
    m_CachedEntity{std::exchange(rhs.m_CachedEntity, nullptr)}
  {
  }

  EntityRef& EntityRef::operator=(const EntityRef& rhs) noexcept
  {
    if (this != &rhs)
    {
      safeUnref();
      safeRef(rhs.m_CachedEntity);
    }

    return *this;
  }

  EntityRef& EntityRef::operator=(EntityRef&& rhs) noexcept
  {
    assert(this != &rhs && "Moving a value into itself is undefined behavior (and probably indicative of a bug).");

    safeUnref();

    m_ID           = std::exchange(rhs.m_ID, bfUUID_makeEmpty().as_number);
    m_CachedEntity = std::exchange(rhs.m_CachedEntity, nullptr);

    return *this;
  }

  const bfUUIDNumber& EntityRef::uuid() const
  {
    return m_ID;
  }

  Entity* EntityRef::object() noexcept
  {
    if (m_CachedEntity && m_CachedEntity->isFlagSet(Entity::IS_PENDING_DELETED))
    {
      unref(false);
    }
    else if (!m_CachedEntity && !bfUUID_numberIsEmpty(&m_ID))
    {
      safeRef(gc::findEntity(m_ID));
    }

    return m_CachedEntity;
  }

  Entity* EntityRef::operator->() noexcept
  {
    return object();
  }

  Entity& EntityRef::operator*() noexcept
  {
    return *object();
  }

  EntityRef::~EntityRef() noexcept
  {
    safeUnref();
  }

  void EntityRef::unref(bool reset_id)
  {
    assert(m_CachedEntity && "This method should only be called after the caller has checked for null.");

    m_CachedEntity->release();
    m_CachedEntity = nullptr;

    if (reset_id)
    {
      m_ID = bfUUID_makeEmpty().as_number;
    }
  }

  void EntityRef::safeUnref(bool reset_id)
  {
    if (m_CachedEntity)
    {
      unref(reset_id);
    }
  }

  void EntityRef::ref(Entity* obj)
  {
    assert(obj && "This function should only be called when you actually have an object to reference.");
    assert(!m_CachedEntity && "This should only be called when nothing is currently referenced.");

    m_ID           = obj->uuid();
    m_CachedEntity = obj;
    m_CachedEntity->acquire();
  }

  void EntityRef::safeRef(Entity* obj)
  {
    if (obj)
    {
      ref(obj);
    }
  }

  namespace gc
  {
    static constexpr int k_InitialMapSize = 256;

    using UUIDToObject = HashTable<bfUUIDNumber, Entity*, k_InitialMapSize, UUIDHasher, UUIDEqual>;

    struct GCContext final
    {
      struct Deleter final
      {
        void operator()(GCContext* self) const
        {
          self->memory->deallocateT(self);
        }
      };

      IMemoryManager*             memory;
      UUIDToObject                id_to_object;
      intrusive::ListView<Entity> gc_list;

      explicit GCContext(IMemoryManager* mem) :
        memory{mem},
        id_to_object{},
        gc_list{&Entity::m_GCList}
      {
      }

      static bfUUIDNumber& id(Entity& object)
      {
        return object.m_UUID;
      }
    };

    using GCContextPtr = std::unique_ptr<GCContext, GCContext::Deleter>;

    static GCContextPtr g_GCCtx;

    void init(IMemoryManager& memory)
    {
      g_GCCtx = GCContextPtr(memory.allocateT<GCContext>(&memory));
    }

    bool hasUUID(const bfUUIDNumber& id)
    {
      return findEntity(id) != nullptr;
    }

    void registerEntity(Entity& object)
    {
      assert(!hasUUID(GCContext::id(object)) && "The Entity must not already be in the system.");

      reviveEntity(object);
    }

    Entity* findEntity(const bfUUIDNumber& id)
    {
      Entity** const entry  = g_GCCtx->id_to_object.at(id);
      Entity* const  entity = entry ? *entry : nullptr;

      return entity && !entity->isFlagSet(Entity::IS_PENDING_DELETED) ? entity : nullptr;
    }

    void removeEntity(Entity& object)
    {
      g_GCCtx->gc_list.pushBack(object);
    }

    void reviveEntity(Entity& object)
    {
      assert(object.hasUUID() && "The Entity must have a UUID to be registered to the GC system.");

      g_GCCtx->id_to_object[GCContext::id(object)] = &object;
    }

    void collect(IMemoryManager& entity_memory)
    {
      // TODO(Shareef): Don't do this every frame, maybe every few frames.

      auto& gc_list = g_GCCtx->gc_list;

      intrusive::ListView<Entity> free_list{&Entity::m_GCList};

      for (auto it = gc_list.begin(); it != gc_list.end(); /* NO-OP */)
      {
        Entity& entity = *it;

        if (entity.refCount() == 0)
        {
          it = gc_list.erase(it);
          free_list.pushBack(entity);
        }
        else
        {
          ++it;
        }
      }

      while (!free_list.isEmpty())
      {
        Entity& entity = free_list.back();
        free_list.popBack();

        if (entity.hasUUID())
        {
          g_GCCtx->id_to_object[GCContext::id(entity)] = nullptr;
        }

        entity_memory.deallocateT(&entity);
      }
    }

    void quit()
    {
      g_GCCtx.reset();
    }
  }  // namespace gc
}  // namespace bifrost
