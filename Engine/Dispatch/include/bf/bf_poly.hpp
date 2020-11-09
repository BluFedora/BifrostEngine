/******************************************************************************/
/*!
 * @file   bf_poly.hpp
 * @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
 * @brief
 *    A polymorphic view of an object for a certain interface without using 
 *    inheritance.
 *
 * @version 0.0.1
 * @date    August 26th, 2020
 *
 * @copyright Copyright (c) 2020
 */
/******************************************************************************/
#ifndef BF_POLY_HPP
#define BF_POLY_HPP

#include "../../../Runtime/include/bifrost/meta/bf_meta_function_traits.hpp"

#include <cstddef>     /* max_align_t */
#include <tuple>       /* tuple stuff */
#include <type_traits> /* aligned_storage */
#include <utility>     /* integer_sequence stuff */

namespace bf
{
  namespace poly
  {
    template<bool is_self = true>
    struct BaseErasedTag
    {};

    using ErasedTag = BaseErasedTag<true>;

    template<typename>
    struct FnDef; /* undefined */

    template<typename R, typename... Args>
    struct FnDef<R(Args...)>
    {
      template<typename T>
      struct Filter
      {
        using type = T;
      };

      // clang-format off
      template<> struct Filter<ErasedTag> { using type = void*; };
      template<> struct Filter<ErasedTag&> { using type = void*; };
      template<> struct Filter<const ErasedTag&> { using type = const void*; };
      template<> struct Filter<ErasedTag*> { using type = void*; };
      template<> struct Filter<const ErasedTag*> { using type = const void*; };

      template<> struct Filter<BaseErasedTag<false>> { using type = void*; };
      template<> struct Filter<BaseErasedTag<false>&> { using type = void*; };
      template<> struct Filter<const BaseErasedTag<false>&> { using type = const void*; };
      template<> struct Filter<BaseErasedTag<false>*> { using type = void*; };
      template<> struct Filter<const BaseErasedTag<false>*> { using type = const void*; };
      // clang-format on

      template<typename F>
      struct FnWrapper
      {
        constexpr FnWrapper(std::nullptr_t) :
          fnp{nullptr}
        {
          static_assert(false, "Null(ptr) function pointers are not allowed as a vtable entry.");
        }

        constexpr FnWrapper(F fn) :
          fnp{fn}
        {
        }

        F fnp;

        constexpr operator F() const
        {
          return fnp;
        }
      };

      template<template<typename> typename TFilter>
      using FnType = FnWrapper<R (*)(typename TFilter<Args>::type...)>;

      FnType<Filter> callback;

      FnDef(FnType<Filter> cb) :
        callback{cb}
      {
      }

      template<std::size_t LogicalArgIndex, std::size_t ActualArgIndex, typename... ActualArgs>
      decltype(auto) genArgs(void* ctx, const std::tuple<ActualArgs...>& args)
      {
        static constexpr auto k_NumArgs = sizeof...(Args);

        using RawArgsTuple = std::tuple<Args...>;

        if constexpr (k_NumArgs > 0)
        {
          // If at last argument.
          if constexpr (LogicalArgIndex + 1 >= k_NumArgs)
          {
            if constexpr (std::is_same_v<std::decay_t<std::tuple_element_t<LogicalArgIndex, RawArgsTuple>>, ErasedTag>)
            {
              return std::make_tuple(ctx);
            }
            else
            {
              return std::make_tuple(std::get<ActualArgIndex>(args));
            }
          }
          else  // if constexpr (LogicalArgIndex + 1 < k_NumArgs)
          {
            if constexpr (std::is_same_v<std::decay_t<std::tuple_element_t<LogicalArgIndex, RawArgsTuple>>, ErasedTag>)
            {
              return std::tuple_cat(std::make_tuple(ctx), genArgs<LogicalArgIndex + 1, ActualArgIndex>(ctx, args));
            }
            else
            {
              return std::tuple_cat(std::make_tuple(std::get<ActualArgIndex>(args)), genArgs<LogicalArgIndex + 1, ActualArgIndex + 1>(ctx, args));
            }
          }
        }
        else
        {
          return std::tuple<>();
        }
      }

      template<typename... ActualArgs>
      decltype(auto) invokeImpl(void* ctx, ActualArgs... args)
      {
        const auto original_args_tuple = std::forward_as_tuple(std::forward<ActualArgs>(args)...);  // make_tuple
        const auto arg_tuple           = genArgs<0, 0>(ctx, original_args_tuple);

        return std::apply(callback, arg_tuple);
      }

      template<typename... ActualArgs>
      decltype(auto) invoke(void* ctx, ActualArgs&&... args)
      {
        return invokeImpl(ctx, std::forward<ActualArgs>(args)...);
      }
    };

    template<typename TDecl, typename TRemapTo>
    using concept_map = typename TDecl::template concept_map_t<TRemapTo>;

    template<typename TDecl, typename TRemapTo>
    static constexpr concept_map<TDecl, TRemapTo> remap = {};  // If this fails to construct you have not specialized it to handle a type you tried to assign.

    template<typename TDecl, typename TRemapTo, typename... Args>
    static constexpr concept_map<TDecl, TRemapTo> makeConceptMap(Args&&... args)
    {
      return concept_map<TDecl, TRemapTo>{
       std::forward<Args>(args)...,
       [](TRemapTo* obj) -> void { obj->~TRemapTo(); },                                    // Delete
       [](TRemapTo* lhs, const TRemapTo* rhs) -> void { new (lhs) TRemapTo(*rhs); },       // Copy
       [](TRemapTo* lhs, TRemapTo* rhs) -> void { new (lhs) TRemapTo(std::move(*rhs)); },  // Move
       []() -> std::size_t { return sizeof(TRemapTo); },                                   // Size
      };
    }

    using DtorFnDef = FnDef<void(ErasedTag&)>;
    using CopyFnDef = FnDef<void(ErasedTag&, const BaseErasedTag<false>&)>;
    using MoveFnDef = FnDef<void(ErasedTag&, BaseErasedTag<false>&)>;
    using SizeFnDef = FnDef<std::size_t()>;

    namespace detail
    {
      template<typename ClassArg, typename... Args>
      struct defMember_ final
      {
        using NonConstClassPtr = std::remove_const_t<std::remove_pointer_t<ClassArg>>*;  //!< This poly class is not const correct, it will accept const member functions for concepts that claim to mutate.

        template<auto member_fn, typename Return>
        static Return impl(NonConstClassPtr self, Args... args)
        {
          return meta::invoke(member_fn, self, std::forward<Args>(args)...);
        }
      };
    }  // namespace detail

    template<auto member_fn>
    static constexpr decltype(auto) defMember()
    {
      using FnTraits         = meta::function_traits<decltype(member_fn)>;
      using Return           = typename FnTraits::return_type;
      using defMemberApplied = typename FnTraits::parameter_pack::template apply<detail::defMember_>;

      return &defMemberApplied::template impl<member_fn, Return>;
    }

    template<typename... FnDefs>
    struct decl
    {
      static constexpr std::size_t k_DtorIndex = sizeof...(FnDefs);
      static constexpr std::size_t k_CopyIndex = k_DtorIndex + 1;
      static constexpr std::size_t k_MoveIndex = k_CopyIndex + 1;
      static constexpr std::size_t k_SizeIndex = k_MoveIndex + 1;

      template<typename ObjT, typename TFnDef>
      struct Filter
      {
        template<typename T>
        struct ErasedToT
        {
          using type = T;
        };

        // clang-format off
        template<> struct ErasedToT<ErasedTag> { using type = ObjT*; };
        template<> struct ErasedToT<ErasedTag&> { using type = ObjT*; };
        template<> struct ErasedToT<const ErasedTag&> { using type = const ObjT*; };
        template<> struct ErasedToT<ErasedTag*> { using type = ObjT*; };
        template<> struct ErasedToT<const ErasedTag*> { using type = const ObjT*; };
                        
        template<> struct ErasedToT<BaseErasedTag<false>> { using type = ObjT*; };
        template<> struct ErasedToT<BaseErasedTag<false>&> { using type = ObjT*; };
        template<> struct ErasedToT<const BaseErasedTag<false>&> { using type = const ObjT*; };
        template<> struct ErasedToT<BaseErasedTag<false>*> { using type = ObjT*; };
        template<> struct ErasedToT<const BaseErasedTag<false>*> { using type = const ObjT*; };
        // clang-format on

        using type = typename TFnDef::template FnType<ErasedToT>;
      };

      template<typename TRemapTo>
      using concept_map_t = std::tuple<
       typename Filter<TRemapTo, FnDefs>::type...,
       typename Filter<TRemapTo, DtorFnDef>::type,
       typename Filter<TRemapTo, CopyFnDef>::type,
       typename Filter<TRemapTo, MoveFnDef>::type,
       typename Filter<TRemapTo, SizeFnDef>::type>;

      using vtable = std::tuple<FnDefs..., DtorFnDef, CopyFnDef, MoveFnDef, SizeFnDef>;

      template<typename T, std::size_t... I>
      static vtable* gen_vtable(std::index_sequence<I...>)
      {
        static vtable s_Table = std::make_tuple(std::tuple_element_t<I, vtable>{((decltype(std::get<I>(std::declval<vtable>())))std::get<I>(remap<decl, T>))}...);
        return &s_Table;
      }

      template<typename T>
      static vtable* get_table_for()
      {
        return gen_vtable<T>(std::make_index_sequence<std::tuple_size_v<vtable>>{});
      }
    };

    //
    // StoragePolicy 'Concept'
    //
    // static constexpr bool needs_dtor_called;
    //
    // Copyable.
    //
    //   // Do any static type checking here.
    // void  staticCheck<T>();
    //   // Return a pointer to the memory allocated.
    // void* self() const;
    //   // Allocate memeory of 'size' bytes, the 'original_object' should not be used unless you are the 'RefStorage' object.
    //   // (this will only be called when there is no prior allocation.)
    // void  alloc(const void* original_object, std::size_t size);
    //   // Free any memory allocated by 'alloc' (will only be called after a call to 'alloc')
    // void free();
    //

    struct RefStorage
    {
      void* ptr;

      static constexpr bool needs_dtor_called = false;

      template<typename T>
      void staticCheck()
      {} /* NO-OP */

      void* self() const { return ptr; }
      void  alloc(const void* original_object, std::size_t size) { ptr = const_cast<void*>(original_object); }
      void  free() {} /* NO-OP */
    };

    struct HeapStorage
    {
      void* ptr;

      static constexpr bool needs_dtor_called = true;

      template<typename T>
      void staticCheck()
      {} /* NO-OP */

      void* self() const { return ptr; }
      void  alloc(const void*, std::size_t size) { ptr = ::malloc(size); }
      void  free() { ::free(ptr); }
    };

    template<std::size_t k_SmallSize = sizeof(void*) * 4, std::size_t k_Alignment = alignof(std::max_align_t)>
    struct SBOStorage
    {
      using SBOBuffer = std::aligned_storage_t<k_SmallSize, k_Alignment>;

      void*     ptr;
      SBOBuffer sbo;

      static constexpr bool needs_dtor_called = true;

      template<typename T>
      void staticCheck()
      {} /* NO-OP */

      void* self() const { return ptr; }
      void  alloc(const void*, std::size_t size) { ptr = size <= k_SmallSize ? &sbo : ::malloc(size); }

      void free()
      {
        if (ptr != &sbo)
          ::free(ptr);
      }
    };

    template<std::size_t k_SmallSize = sizeof(void*) * 4, std::size_t k_Alignment = alignof(std::max_align_t)>
    struct SmallStorage
    {
      using SBOBuffer = std::aligned_storage_t<k_SmallSize, k_Alignment>;

      mutable SBOBuffer sbo;

      static constexpr bool needs_dtor_called = true;

      template<typename T>
      void staticCheck()
      {
        static_assert(sizeof(T) <= k_SmallSize, "This object is too large to fit within this storage policy.");
      }

      void* self() const { return &sbo; }
      void  alloc(const void*, std::size_t size) {} /* NO-OP */
      void  free() {}                               /* NO-OP */
    };

  }  // namespace poly

  template<typename TInterfaceDecl, typename StoragePolicy = poly::HeapStorage>
  class Poly
  {
    using VTablePtr = typename TInterfaceDecl::vtable*;

   private:
    VTablePtr     m_VTable;
    StoragePolicy m_Storage;

   public:
    Poly(const StoragePolicy& storage = StoragePolicy()) :
      m_VTable{nullptr},
      m_Storage{storage}
    {
    }

    template<typename T>
    Poly(const T& rhs, const StoragePolicy& storage = StoragePolicy()) :
      m_VTable{nullptr},
      m_Storage{storage}
    {
      createCopy(rhs);
    }

    template<typename T>
    Poly(T&& rhs, const StoragePolicy& storage = StoragePolicy()) :
      m_VTable{nullptr},
      m_Storage{storage}
    {
      createMove(std::forward<T>(rhs));
    }

    Poly(const Poly& rhs) :
      m_VTable{rhs.m_VTable},
      m_Storage{}
    {
      if (m_VTable)
      {
        cloneRhs<TInterfaceDecl::k_CopyIndex>(rhs);
      }
    }

    Poly(Poly&& rhs) noexcept :
      m_VTable{rhs.m_VTable},
      m_Storage{}
    {
      if (m_VTable)
      {
        cloneRhs<TInterfaceDecl::k_MoveIndex>(std::forward<Poly>(rhs));
      }
    }

    Poly& operator=(const Poly& rhs)
    {
      if (this != &rhs)
      {
        destroy();
        cloneRhs<TInterfaceDecl::k_CopyIndex>(rhs);
      }

      return *this;
    }

    Poly& operator=(Poly&& rhs) noexcept
    {
      destroy();

      if (rhs.m_VTable)
      {
        cloneRhs<TInterfaceDecl::k_MoveIndex>(std::forward<Poly>(rhs));
      }

      return *this;
    }

    template<typename T>
    Poly& operator=(const T& rhs)
    {
      destroy();
      createCopy(rhs);

      return *this;
    }

    template<typename T>
    Poly& operator=(T&& rhs)
    {
      destroy();
      createMove(std::forward<T>(rhs));

      return *this;
    }

    void* self() const
    {
      return m_Storage.self();
    }

    template<int f, typename... Args>
    decltype(auto) invoke(Args&&... args) const
    {
      return std::get<f>(*m_VTable).invoke(m_Storage.self(), std::forward<Args>(args)...);
    }

    ~Poly()
    {
      destroy();
    }

   private:
    template<std::size_t vtable_index, typename RhsPoly>
    void cloneRhs(RhsPoly&& rhs)
    {
      m_VTable = rhs.m_VTable;

      const std::size_t alloc_size = std::get<TInterfaceDecl::k_SizeIndex>(*m_VTable).invoke(rhs.self());

      m_Storage.alloc(rhs.self(), alloc_size);
      std::get<vtable_index>(*m_VTable).invoke(self(), rhs.self());
    }

    template<typename T>
    void create_(const void* obj)
    {
      m_Storage.staticCheck<T>();
      m_VTable = TInterfaceDecl::template get_table_for<T>();
      m_Storage.alloc(obj, sizeof(T));
    }

    template<typename T>
    void createCopy(const T& obj)
    {
      create_<T>(&obj);
      std::get<TInterfaceDecl::k_CopyIndex>(*m_VTable).invoke(self(), &obj);
    }

    template<typename T>
    void createMove(T&& obj)
    {
      create_<T>(&obj);
      std::get<TInterfaceDecl::k_MoveIndex>(*m_VTable).invoke(self(), &obj);
    }

    void dtor() const
    {
      dtor(m_VTable);
    }

    void dtor(VTablePtr table) const
    {
      if constexpr (StoragePolicy::needs_dtor_called)
      {
        std::get<TInterfaceDecl::k_DtorIndex>(*table).invoke(m_Storage.self());
      }
    }

    void destroy()
    {
      if (m_VTable)
      {
        dtor();
        m_Storage.free();
        m_VTable = nullptr;
      }
    }
  };

  template<typename TInterfaceDecl>
  using PolyView = Poly<TInterfaceDecl, poly::RefStorage>;
}  // namespace bf

#endif /* BF_POLY_HPP */
