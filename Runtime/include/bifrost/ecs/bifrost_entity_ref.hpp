/******************************************************************************/
/*!
 * @file   bifrost_entity_ref.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *  This is for having safe refences to Entities even if they get _deleted_.
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

#include <cstddef> /* nullptr_t */

namespace bf
{
  class Entity;
  class IMemoryManager;

  class EntityRef final
  {
    friend class JsonSerializerWriter;
    friend class JsonSerializerReader;

   private:
    bfUUIDNumber m_ID;
    Entity*      m_CachedEntity;

   public:
    explicit EntityRef(Entity* object = nullptr) noexcept;
    explicit EntityRef(Entity& object) noexcept;
    EntityRef(std::nullptr_t) noexcept;

    EntityRef(const EntityRef& rhs) noexcept;
    EntityRef(EntityRef&& rhs) noexcept;

    EntityRef& operator=(const EntityRef& rhs) noexcept;
    EntityRef& operator=(EntityRef&& rhs) noexcept;

    [[nodiscard]] const bfUUIDNumber& uuid() const;
    [[nodiscard]] Entity*             object() noexcept;

    [[nodiscard]]         operator bool() { return object() != nullptr; }
    [[nodiscard]]         operator Entity*() { return object(); }
    [[nodiscard]] Entity* operator->() noexcept;
    [[nodiscard]] Entity& operator*() noexcept;

    ~EntityRef() noexcept;

   public: /* 'Private' Editor API */
    Entity* editorCachedEntity() const { return m_CachedEntity; }

   private:
    void unref(bool reset_id = true);
    void safeUnref(bool reset_id = true);
    void ref(Entity* obj);
    void safeRef(Entity* obj);
  };

  //
  // This is the API for the very basic Entity Garbage collection system
  //
  namespace gc
  {
    void    init(IMemoryManager& memory);
    bool    hasUUID(const bfUUIDNumber& id);
    void    registerEntity(Entity& object);
    Entity* findEntity(const bfUUIDNumber& id);
    void    removeEntity(Entity& object);
    void    reviveEntity(Entity& object);  // TODO(SR): Editor only API
    void    collect(IMemoryManager& entity_memory);
    void    quit();
  }  // namespace gc
}  // namespace bf

#endif /* BIFROST_REF_HPP */
