#ifndef BIFROST_META_RUNTIME_HPP
#define BIFROST_META_RUNTIME_HPP

#include "bifrost/data_structures/bifrost_any.hpp"
#include "bifrost/data_structures/bifrost_array.hpp"
#include "bifrost/data_structures/bifrost_hash_table.hpp"
#include "bifrost/memory/bifrost_proxy_allocator.hpp"
#include <string_view> /* string_view */

namespace bifrost::meta
{
  class BaseClassMetaInfo;

  NoFreeAllocator&                                 gRttiMemory();
  HashTable<std::string_view, BaseClassMetaInfo*>& gRegistry();

  template<typename BaseT, typename... CtorArgs>
  class Factory;

  class BaseCtorMetaInfo
  {
    friend class BaseClassMetaInfo;

   protected:
    Array<BaseClassMetaInfo*> m_Parameters;

   public:
    const Array<BaseClassMetaInfo*>& paremeters() const { return m_Parameters; }

    virtual ~BaseCtorMetaInfo() = default;

   protected:
    BaseCtorMetaInfo();

    virtual Any  instantiateImpl(IMemoryManager& memory, const Any* arguments) = 0;
    virtual bool isCompatible(const Any* arguments)                            = 0;
  };

  class BaseMetaInfo
  {
    template<typename BaseT, typename... CtorArgs>
    friend class Factory;

   protected:
    std::string_view m_Name;

   public:
    explicit BaseMetaInfo(std::string_view name);

    [[nodiscard]] std::string_view name() const { return m_Name; }

    virtual ~BaseMetaInfo() = default;
  };

  class BasePropertyMetaInfo : public BaseMetaInfo
  {
    template<typename BaseT, typename... CtorArgs>
    friend class Factory;

   private:
    BaseClassMetaInfo* m_Type;

   protected:
    BasePropertyMetaInfo(std::string_view name, BaseClassMetaInfo* type);

   public:
    [[nodiscard]] BaseClassMetaInfo* type() const { return m_Type; }

    virtual Any  get(Any& instance)                   = 0;
    virtual void set(Any& instance, const Any& value) = 0;
  };

  class BaseMemberMetaInfo : public BasePropertyMetaInfo
  {
    template<typename BaseT, typename... CtorArgs>
    friend class Factory;

   protected:
    std::ptrdiff_t m_Offset;

   protected:
    BaseMemberMetaInfo(std::string_view name, BaseClassMetaInfo* type, std::ptrdiff_t offset);

   public:
    [[nodiscard]] std::ptrdiff_t offset() const { return m_Offset; }
  };

  class BaseMethodMetaInfo : public BaseMetaInfo
  {
    template<typename BaseT, typename... CtorArgs>
    friend class Factory;

   protected:
    Array<BaseClassMetaInfo*> m_Parameters;
    BaseClassMetaInfo*        m_ReturnType;

   public:
    [[nodiscard]] const Array<BaseClassMetaInfo*>& parameters() const { return m_Parameters; }
    [[nodiscard]] BaseClassMetaInfo*               returnType() const { return m_ReturnType; }

    template<typename... Args>
    Any invoke(Args&&... args)
    {
      if (sizeof...(Args) != m_Parameters.size())
      {
        throw "Arity Mismatch";
      }

      const Any any_arguments[] = {args...};
      return invokeImpl(any_arguments);
    }

   protected:
    BaseMethodMetaInfo(std::string_view name, std::size_t arity, BaseClassMetaInfo* return_type);

   private:
    virtual Any invokeImpl(const Any* arguments) = 0;
  };

  class BaseEnumElementMetaInfo : public BaseMetaInfo
  {
    template<typename BaseT, typename... CtorArgs>
    friend class Factory;

   protected:
    std::size_t m_Value;

   protected:
    BaseEnumElementMetaInfo(std::string_view name, std::size_t value) :
      BaseMetaInfo(name),
      m_Value{value}
    {
    }

   public:
    [[nodiscard]] std::size_t value() const { return m_Value; }
  };

  class BaseEnumMetaInfo : public BaseMetaInfo
  {
    template<typename BaseT, typename... CtorArgs>
    friend class Factory;

   protected:
    Array<BaseEnumElementMetaInfo*> m_Elements;

   protected:
    BaseEnumMetaInfo(std::string_view name, std::size_t num_values);

   public:
    [[nodiscard]] const Array<BaseEnumElementMetaInfo*>& elements() const { return m_Elements; }
  };

  struct InvalidCtorCall
  {};

  class BaseClassMetaInfo : public BaseMetaInfo
  {
    template<typename BaseT, typename... CtorArgs>
    friend class Factory;

   protected:
    Array<BaseClassMetaInfo*>    m_BaseClasses;
    Array<BaseCtorMetaInfo*>     m_Ctors;
    Array<BaseMemberMetaInfo*>   m_Members;
    Array<BasePropertyMetaInfo*> m_Properties;
    Array<BaseMethodMetaInfo*>   m_Methods;
    std::size_t                  m_Size;
    std::size_t                  m_Alignment;

   protected:
    BaseClassMetaInfo(std::string_view name, std::size_t size, std::size_t alignment);

   public:
    [[nodiscard]] std::size_t size() const { return m_Size; }
    [[nodiscard]] std::size_t alignment() const { return m_Alignment; }

    const Array<BaseClassMetaInfo*>&    baseClasses() const { return m_BaseClasses; }
    const Array<BaseCtorMetaInfo*>&     ctors() const { return m_Ctors; }
    const Array<BaseMemberMetaInfo*>&   members() const { return m_Members; }
    const Array<BasePropertyMetaInfo*>& properties() const { return m_Properties; }
    const Array<BaseMethodMetaInfo*>&   methods() const { return m_Methods; }

    Any instantiate(IMemoryManager& memory)
    {
      for (auto& ctor : m_Ctors)
      {
        if (ctor->paremeters().size() == 0)
        {
          return ctor->instantiateImpl(memory, nullptr);
        }
      }

      return InvalidCtorCall{};
    }

    template<typename... Args>
    Any instantiate(IMemoryManager& memory, Args&&... args)
    {
      const Any any_arguments[] = {args...};

      for (auto& ctor : m_Ctors)
      {
        if (sizeof...(Args) == ctor->paremeters().size())
        {
          if (ctor->isCompatible(any_arguments))
          {
            return ctor->instantiateImpl(memory, any_arguments);
          }
        }
      }

      return InvalidCtorCall{};
    }

    BaseMemberMetaInfo*   findMember(std::string_view name) const;
    BasePropertyMetaInfo* findProperty(std::string_view name) const;
    BaseMethodMetaInfo*   findMethod(std::string_view name) const;
  };

  using BaseUnionMetaInfo  = BaseClassMetaInfo;
  using BaseStructMetaInfo = BaseClassMetaInfo;

  template<typename T>
  BaseClassMetaInfo* g_TypeInfo = nullptr;

  template<typename T>
  BaseClassMetaInfo* g_TypeInfo<T*> = g_TypeInfo<T>;

  // clang-format off
  /*
  template<> extern BaseClassMetaInfo* g_TypeInfo<std::byte>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<bool>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<char>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<int8_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<uint8_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<int16_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<uint16_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<int32_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<uint32_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<int64_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<uint64_t>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<float>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<double>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<long double>;
  template<> extern BaseClassMetaInfo* g_TypeInfo<void*>;
  */
  // clang-format on

  template<typename T>
  BaseClassMetaInfo* TypeInfo()
  {
    return g_TypeInfo<T>;
  }

  BaseClassMetaInfo* TypeInfoFromName(std::string_view name);

}  // namespace bifrost::meta

#endif /* BIFROST_META_RUNTIME_HPP */
