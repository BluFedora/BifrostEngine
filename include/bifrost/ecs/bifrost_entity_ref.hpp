/******************************************************************************/
/*!
 * @file   bifrost_entity_ref.hpp
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
#ifndef BIFROST_REF_HPP
#define BIFROST_REF_HPP

#include "bifrost/utility/bifrost_uuid.h" /* BifrostUUIDNumber */

namespace bifrost
{
  class IBaseObject;
  class Entity;

  class EntityRef final
  {
   private:
    BifrostUUIDNumber m_ID;
    Entity*           m_CachedEntity;

   public:
    explicit EntityRef(Entity* object = nullptr) noexcept :
      m_ID{},
      m_CachedEntity{object}
    {
    }

    EntityRef(const EntityRef& rhs) noexcept;
    EntityRef(EntityRef&& rhs) noexcept;

    EntityRef& operator=(const EntityRef& rhs) noexcept;
    EntityRef& operator=(EntityRef&& rhs) noexcept;

    [[nodiscard]] Entity* object() const noexcept
    {
      return m_CachedEntity;
    }

    void bind(Entity* obj) noexcept
    {
      m_CachedEntity = obj;
    }

    ~EntityRef() noexcept;

   private:
    void unref();
    void safeUnref();
    void ref(Entity* obj);
  };
}  // namespace bifrost

#endif /* BIFROST_REF_HPP */
