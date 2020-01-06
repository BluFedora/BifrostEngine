/*!
 * @file   bifrost_component.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 * @version 0.0.1
 * @date    2019-12-22
 *
 * @copyright Copyright (c) 2019
 */
#ifndef BIFROST_COMPONENT_HPP
#define BIFROST_COMPONENT_HPP

#include "bifrost/core/bifrost_base_object.hpp"           // BaseObject
#include "bifrost/data_structures/bifrost_dense_map.hpp"  // denseMap<T>

namespace bifrost
{
  class Entity;

  class BaseComponentStorage : bfNonCopyMoveable<BaseComponentStorage>
  {
   public:
    static std::uint32_t s_IDAssigner;

   public:
    BaseComponentStorage() = default;

    [[nodiscard]] virtual std::size_t  numComponents() const                = 0;
    [[nodiscard]] virtual BaseObjectT* componentAt(std::size_t idx)         = 0;
    [[nodiscard]] virtual BaseObjectT* getComponent(dense_map::ID_t id)     = 0;
    virtual dense_map::ID_t            createComponent(Entity& owner)       = 0;
    virtual void                       destroyComponent(dense_map::ID_t id) = 0;

    virtual ~BaseComponentStorage() = default;
  };

  template<typename TDerived>
  class Component : public BaseObject<Component<TDerived>, TDerived>
  {
   public:
    static std::uint32_t s_ComponentID;

   protected:
    Entity* m_Owner;

   public:
    explicit Component(Entity& owner) :
      m_Owner{&owner}
    {
    }

    meta::BaseClassMetaInfo* type() override
    {
      return meta::TypeInfo<TDerived>::get();
    }
  };

  template<typename TDerived>
  std::uint32_t Component<TDerived>::s_ComponentID = BaseComponentStorage::s_IDAssigner++;

  template<typename TComponent>
  class ComponentStorage : public BaseComponentStorage
  {
   private:
    DenseMap<TComponent> m_Storage;

   public:
    explicit ComponentStorage(IMemoryManager& memory) :
      BaseComponentStorage(),
      m_Storage{memory}
    {
    }

    [[nodiscard]] std::size_t numComponents() const override
    {
      return m_Storage.size();
    }

    [[nodiscard]] BaseObjectT* componentAt(std::size_t idx) override
    {
      return &m_Storage[idx];
    }

    [[nodiscard]] BaseObjectT* getComponent(dense_map::ID_t id) override
    {
      return &m_Storage.find(id);
    }

    dense_map::ID_t createComponent(Entity& owner) override
    {
      return m_Storage.add(owner).toID();
    }

    void destroyComponent(dense_map::ID_t id) override
    {
      m_Storage.remove(id);
    }

    ~ComponentStorage() override = default;
  };
}  // namespace bifrost

#endif /* BIFROST_COMPONENT_HPP */
