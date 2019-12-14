#ifndef BIFROST_FUNCTION_VIEW_HPP
#define BIFROST_FUNCTION_VIEW_HPP

#include <optional> /* optional<T>  */
#include <utility>  /* pair<T1, T2> */

namespace bifrost
{
  template<typename>
  class FunctionView; /* undefined */

  template<typename R, typename... Args>
  class FunctionView<R(Args...)>
  {
   public:
    using InstancePtr = void*;
      using FunctionPtr = R (*)(Args...);

    template<typename C>
    using MemberFunctionPtr = R (C::*)(Args...);
    template<typename C>
    using ConstMemberFunctionPtr = R (C::*)(Args...) const;
    using ErasedFunctionPtr      = R (*)(InstancePtr, Args...);
    using FunctionPair           = std::pair<InstancePtr, ErasedFunctionPtr>;

   private:
    FunctionPair m_Callback;

   public:
    FunctionView() :
      m_Callback(nullptr, nullptr)
    {
    }

    decltype(auto) operator()(Args... args)
    {
      if constexpr (std::is_same_v<void, R>)
      {
        return this->call(std::forward<Args>(args)...);
      }
      else
      {
        call(std::forward<Args>(args)...);
      }
    }

    operator bool() const
    {
      return (m_Callback.second != nullptr);
    }


    template<typename F>
    void bind(F&& lambda)
    {
      this->m_Callback.first  = &lambda;
      this->m_Callback.second = &FunctionView::template lambda_function_wrapper<F>;
    }

    void bind(FunctionPtr lambda)
    {
      this->m_Callback.first  = reinterpret_cast<void*>(lambda);
      this->m_Callback.second = &FunctionView::c_ptr_function_wrapper;
    }

    template<FunctionPtr callback>
    void bind()
    {
      this->m_Callback.first  = nullptr;
      this->m_Callback.second = &FunctionView::template c_function_wrapper<callback>;
    }

    template<typename Clz, R (Clz::*callback)(Args...)>
    void bind(Clz* obj)
    {
      this->m_Callback.first  = obj;
      this->m_Callback.second = &FunctionView::template member_function_wrapper<Clz, callback>;
    }

    template<typename Clz, R (Clz::*callback)(Args...) const>
    void bind(Clz* obj)
    {
      this->m_Callback.first  = obj;
      this->m_Callback.second = &FunctionView::template const_member_function_wrapper<Clz, callback>;
    }

    void unBind()
    {
      this->m_Callback.first  = nullptr;
      this->m_Callback.second = nullptr;
    }

    // This function returns an empty optional if there is not a
    // valid callback stored in this delegate.
    decltype(auto) safeCall(Args... args) const
    {
      using OptionalReturn = std::optional<R>;

      if (m_Callback.second)
      {
        if constexpr (std::is_same_v<void, R>)
        {
          call(std::forward<Args>(args)...);
        }
        else
        {
          return OptionalReturn{call(std::forward<Args>(args)...)};
        }
      }

      if constexpr (!std::is_same_v<void, R>)
      {
        return OptionalReturn{};
      }
    }

    decltype(auto) call(Args... args) const
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (m_Callback.second)(this->m_Callback.first, std::forward<Args>(args)...);
      }
      else
      {
        return (m_Callback.second)(this->m_Callback.first, std::forward<Args>(args)...);
      }
    }

   private:
    static decltype(auto) c_ptr_function_wrapper(InstancePtr instance, Args... args)
    {
      if constexpr (std::is_same_v<void, R>)
      {
        reinterpret_cast<FunctionPtr>(instance)(std::forward<Args>(args)...);
      }
      else
      {
        return reinterpret_cast<FunctionPtr>(instance)(std::forward<Args>(args)...);
      }
    }

    template<typename F>
    static decltype(auto) lambda_function_wrapper(InstancePtr instance, Args... args)
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (*reinterpret_cast<F*>(instance))(std::forward<Args>(args)...);
      }
      else
      {
        return (*reinterpret_cast<F*>(instance))(std::forward<Args>(args)...);
      }
    }

    template<FunctionPtr callback>
    static decltype(auto) c_function_wrapper(InstancePtr instance, Args... args)
    {
      (void)instance;

      if constexpr (std::is_same_v<void, R>)
      {
        callback(std::forward<Args>(args)...);
      }
      else
      {
        return callback(std::forward<Args>(args)...);
      }
    }

    template<typename Clz, MemberFunctionPtr<Clz> callback>
    static decltype(auto) member_function_wrapper(InstancePtr instance, Args... args)
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (static_cast<Clz*>(instance)->*callback)(std::forward<Args>(args)...);
      }
      else
      {
        return (static_cast<Clz*>(instance)->*callback)(std::forward<Args>(args)...);
      }
    }

    template<typename Clz, ConstMemberFunctionPtr<Clz> callback>
    static decltype(auto) const_member_function_wrapper(InstancePtr instance, Args... args)
    {
      if constexpr (std::is_same_v<void, R>)
      {
        (static_cast<const Clz*>(instance)->*callback)(std::forward<Args>(args)...);
      }
      else
      {
        return (static_cast<const Clz*>(instance)->*callback)(std::forward<Args>(args)...);
      }
    }
  };
}  // namespace bifrost

#endif /* BIFROST_FUNCTION_VIEW_HPP */