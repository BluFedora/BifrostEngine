/******************************************************************************/
/*!
 * @file   bf_function_view.hpp
 * @author Shareef Abdoul-Raheem (https://blufedora.github.io/)
 * @brief
 *   Non-owning callable wrapper with the most basic of type erasure.
 *
 *   Limitations:
 *     - Cannot store stateful functor objects such as (stateful) lambdas.
 *     - Member function pointers must be known at compile time (as template argument).
 *
 *   In return for the limitations you get faster execution speed.
 *
 * @version 1.0.3
 * @date    2019-12-28
 *
 * @copyright Copyright (c) 2019-2021
 */
/******************************************************************************/
#ifndef BF_FUNCTION_VIEW_HPP
#define BF_FUNCTION_VIEW_HPP

#include <type_traits> /* aligned_storage_t */

namespace bf
{
  //
  // Thin alternative to std::optional that does not
  // cleanup itself automatically so you must call
  // destroy to end the lifetime of the stored object.
  //
  // You must only call destroy if `FunctionView::safeCall` returns `true`.
  //
  template<typename T>
  struct OptionalResult
  {
    std::aligned_storage_t<sizeof(T), alignof(T)> storage;

    T*   as() { return reinterpret_cast<T*>(&storage); }
    void destroy() { as()->~T(); }
  };

  template<>
  struct OptionalResult<void>
  {
  };

  template<typename>
  class FunctionView; /* undefined */

  template<typename R, typename... Args>
  class FunctionView<R(Args...)>
  {
   private:
    using InstancePtr = void*;
    using FunctionPtr = R (*)(Args...);

    template<typename C>
    using MemberFunctionPtr = R (C::*)(Args...);

    template<typename C>
    using ConstMemberFunctionPtr = R (C::*)(Args...) const;

    using ErasedFunctionPtr = R (*)(InstancePtr, Args...);

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

    R operator()(Args... args) { return call(std::forward<Args>(args)...); }
    operator bool() const { return m_Callback.second != nullptr; }

    void bind(FunctionPtr fn_ptr)
    {
      m_Callback.first  = reinterpret_cast<void*>(fn_ptr);
      m_Callback.second = &FunctionView::wrapperFn;
    }

    template<FunctionPtr callback>
    void bind()
    {
      m_Callback.first  = nullptr;
      m_Callback.second = &FunctionView::template wrapperFnPtr<callback>;
    }

    template<typename Clz, R (Clz::*callback)(Args...)>
    void bind(Clz* obj)
    {
      m_Callback.first  = obj;
      m_Callback.second = &FunctionView::template wrapperMemberFn<Clz, callback>;
    }

    template<typename Clz, R (Clz::*callback)(Args...) const>
    void bind(Clz* obj)
    {
      m_Callback.first  = obj;
      m_Callback.second = &FunctionView::template wrapperConstMemberFn<Clz, callback>;
    }

    void unBind()
    {
      m_Callback.first  = nullptr;
      m_Callback.second = nullptr;
    }

    // This function returns false if there is not a valid
    // function stored in this view and leaves `result` as
    // uninitialized.
    bool safeCall(OptionalResult<R>& result, Args... args)
    {
      const bool is_valid = m_Callback.second;

      if (is_valid)
      {
        if constexpr (std::is_same_v<void, R>)
        {
          call(std::forward<Args>(args)...);
        }
        else
        {
          new (&result.storage) R(call(std::forward<Args>(args)...));
        }
      }

      return is_valid;
    }

    R call(Args... args) const
    {
      return (m_Callback.second)(m_Callback.first, std::forward<Args>(args)...);
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
    static decltype(auto) wrapperFnPtr(InstancePtr instance, Args... args)
    {
      return static_cast<FunctionPtr>(instance)(std::forward<Args>(args)...);
    }

    template<FunctionPtr callback>
    static decltype(auto) wrapperFn(InstancePtr instance, Args... args)
    {
      (void)instance;
      return callback(std::forward<Args>(args)...);
    }

    template<typename Clz, MemberFunctionPtr<Clz> callback>
    static decltype(auto) wrapperMemberFn(InstancePtr instance, Args... args)
    {
      return (static_cast<Clz*>(instance)->*callback)(std::forward<Args>(args)...);
    }

    template<typename Clz, ConstMemberFunctionPtr<Clz> callback>
    static decltype(auto) wrapperConstMemberFn(InstancePtr instance, Args... args)
    {
      return (static_cast<const Clz*>(instance)->*callback)(std::forward<Args>(args)...);
    }
  };
}  // namespace bf

#endif /* BF_FUNCTION_VIEW_HPP */

/******************************************************************************/
/*
  MIT License

  Copyright (c) 2019-2021 Shareef Abdoul-Raheem

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
/******************************************************************************/
