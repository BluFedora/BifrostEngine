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
#include "bifrost/ecs/bifrost_entity_ref.hpp"

#include "bifrost/ecs/bifrost_entity.hpp"

namespace bifrost
{
  EntityRef::EntityRef(const EntityRef& rhs) noexcept :
    m_ID{},
    m_CachedEntity{rhs.m_CachedEntity}
  {
    if (m_CachedEntity)
    {
      m_CachedEntity->acquire();
    }
  }

  EntityRef::EntityRef(EntityRef&& rhs) noexcept :
    m_ID{},
    m_CachedEntity{rhs.m_CachedEntity}
  {
    if (m_CachedEntity)
    {
      m_CachedEntity->acquire();
    }
  }

  EntityRef& EntityRef::operator=(const EntityRef& rhs) noexcept
  {
    if (this != &rhs)
    {
      safeUnref();

      if (rhs.m_CachedEntity)
      {
        ref(rhs.m_CachedEntity);
      }
    }

    return *this;
  }

  EntityRef& EntityRef::operator=(EntityRef&& rhs) noexcept
  {
    assert(this != &rhs && "Moving a value into itself is undefined behavior (and probably indicative of a bug).");

    safeUnref();

    m_CachedEntity = std::exchange(rhs.m_CachedEntity, nullptr);

    return *this;
  }

  EntityRef::~EntityRef() noexcept
  {
    if (m_CachedEntity)
    {
      unref();
    }
  }

  void EntityRef::unref()
  {
    assert(m_CachedEntity && "This method should only be called after the caller has checked for null.");
    m_CachedEntity->release();
    m_CachedEntity = nullptr;
  }

  void EntityRef::safeUnref()
  {
    if (m_CachedEntity)
    {
      unref();
    }
  }

  void EntityRef::ref(Entity* obj)
  {
    assert(!m_CachedEntity && "This should only be called when nothing is currently referenced.");

    m_CachedEntity = obj;
    m_CachedEntity->acquire();
  }
}  // namespace bifrost
