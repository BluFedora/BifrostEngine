
#ifndef BIFROST_FUNCTION_VIEW_HPP
#define BIFROST_FUNCTION_VIEW_HPP

#include <optional> /* optional<T>  */

namespace bifrost
{
  template<typename>
  class FunctionView; /* undefined */

  template<typename R, typename... Args>
  class FunctionView<R(Args...)> final
  {
   private:
    using InstancePtr = void*;
    using FunctionPtr = R (*)(Args...);

    template<typename C>
    using MemberFunctionPtr = R (C::*)(Args...);
    template<typename C>
    using ConstMemberFunctionPtr = R (C::*)(Args...) const;
    using ErasedFunctionPtr      = R (*)(InstancePtr, Args...);

    struct FunctionPair final
    {
      InstancePtr       first;
      ErasedFunctionPtr second;
    };

   private:
    FunctionPair m_Callback;

   public:
    static FunctionView make(FunctionPtr fn_ptr)
    {
      FunctionView self;
      self.bind(fn_ptr);
      return self;
    }

    template<FunctionPtr callback>
    static FunctionView make()
    {
      FunctionView self;
      self.bind<callback>();
      return self;
    }

    template<typename Clz, R (Clz::*callback)(Args...)>
    static FunctionView make(Clz* obj)
    {
      FunctionView self;
      self.template bind<Clz, callback>(obj);
      return self;
    }

    template<typename Clz, R (Clz::*callback)(Args...) const>
    static FunctionView make(Clz* obj)
    {
      FunctionView self;
      self.template bind<Clz, callback>(obj);
      return self;
    }

    FunctionView() :
      m_Callback{nullptr, nullptr}
    {
    }

    R operator()(Args... args)
    {
      return call(std::forward<Args>(args)...);
    }

    operator bool() const
    {
      return (m_Callback.second != nullptr);
    }

    void bind(FunctionPtr fn_ptr)
    {
      m_Callback.first  = reinterpret_cast<void*>(fn_ptr);
      m_Callback.second = &FunctionView::c_ptr_function_wrapper;
    }

    template<FunctionPtr callback>
    void bind()
    {
      m_Callback.first  = nullptr;
      m_Callback.second = &FunctionView::template c_function_wrapper<callback>;
    }

    template<typename Clz, R (Clz::*callback)(Args...)>
    void bind(Clz* obj)
    {
      m_Callback.first  = obj;
      m_Callback.second = &FunctionView::template member_function_wrapper<Clz, callback>;
    }

    template<typename Clz, R (Clz::*callback)(Args...) const>
    void bind(Clz* obj)
    {
      m_Callback.first  = obj;
      m_Callback.second = &FunctionView::template const_member_function_wrapper<Clz, callback>;
    }

    void unBind()
    {
      m_Callback.first  = nullptr;
      m_Callback.second = nullptr;
    }

    // This function returns an empty optional if there is not a
    // valid callback stored in this delegate.
    decltype(auto) safeCall(Args... args) const
    {
      using OptionalReturn = std::optional<R>;

      if constexpr (std::is_same_v<void, R>)
      {
        if (m_Callback.second)
        {
          call(std::forward<Args>(args)...);
        }
      }
      else
      {
        return m_Callback.second ? OptionalReturn{call(std::forward<Args>(args)...)} : OptionalReturn{};
      }
    }

    R call(Args&&... args) const
    {
      return (m_Callback.second)(this->m_Callback.first, std::forward<Args>(args)...);
    }

    bool operator==(const FunctionView& rhs) const
    {
      return m_Callback.first == rhs.m_Callback.first && m_Callback.second == rhs.m_Callback.second;
    }

    bool operator!=(const FunctionView& rhs) const
    {
      return m_Callback.first != rhs.m_Callback.first || m_Callback.second != rhs.m_Callback.second;
    }

   private:
    static decltype(auto) c_ptr_function_wrapper(InstancePtr instance, Args... args)
    {
      return reinterpret_cast<FunctionPtr>(instance)(std::forward<Args>(args)...);
    }

    template<typename F>
    static decltype(auto) lambda_function_wrapper(InstancePtr instance, Args... args)
    {
      return (*reinterpret_cast<F*>(instance))(std::forward<Args>(args)...);
    }

    template<FunctionPtr callback>
    static decltype(auto) c_function_wrapper(InstancePtr instance, Args... args)
    {
      (void)instance;
      return callback(std::forward<Args>(args)...);
    }

    template<typename Clz, MemberFunctionPtr<Clz> callback>
    static decltype(auto) member_function_wrapper(InstancePtr instance, Args... args)
    {
      return (static_cast<Clz*>(instance)->*callback)(std::forward<Args>(args)...);
    }

    template<typename Clz, ConstMemberFunctionPtr<Clz> callback>
    static decltype(auto) const_member_function_wrapper(InstancePtr instance, Args... args)
    {
      return (static_cast<const Clz*>(instance)->*callback)(std::forward<Args>(args)...);
    }
  };
}  // namespace bifrost

#endif /* BIFROST_FUNCTION_VIEW_HPP */
