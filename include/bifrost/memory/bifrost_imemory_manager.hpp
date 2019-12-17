/******************************************************************************/
/*!
  @file   bifrost_imemory_manager.hpp
  @author Shareef Abdoul-Raheem
  @par    email: shareef.a\@digipen.edu
  @brief
      Outlines a basic interface for the various types of memory managers.
*/
/******************************************************************************/
#ifndef IMEMORY_MANAGER_HPP
#define IMEMORY_MANAGER_HPP

#include <cstddef>     /* size_t                       */
#include <cstring>     /* memcpy                       */
#include <type_traits> /* is_convertible, enable_if_t  */
#include <utility>     /* forward                      */

#ifndef DEBUG_MEMORY
#define DEBUG_MEMORY 1
#define DEBUG_MEMORY_SIGNATURE 0xCD
#define DEBUG_PATTERN_ALIGNMENT 0xFE
#endif /* DEBUG_MEMORY */

// [memory_returned][ptr_to_start_of_memory][aligned_memory][extra_memory]
// Allocation:
//static const size_t ptr_alloc = sizeof(void *);
//static const size_t align_size = 64;
//static const size_t request_size = sizeof(RingBuffer)+align_size;
//static const size_t needed = ptr_alloc+request_size;
//void * alloc = ::operator new(needed);
//void *ptr = std::align(align_size, sizeof(RingBuffer),
//                     alloc+ptr_alloc, request_size);
//((void **)ptr)[-1] = alloc; // save for delete calls to use
//return ptr;

// Deallocation:
//if (ptr)
//{
//       void * alloc = ((void **)ptr)[-1];
//       ::operator delete (alloc);
//}

namespace bifrost
{
  class IMemoryManager
  {
   protected:
    IMemoryManager(void) = default;
    ~IMemoryManager()    = default;

   public:
    // Disallow Copies and Moves.
    IMemoryManager(const IMemoryManager& rhs) = delete;
    IMemoryManager(IMemoryManager&& rhs)      = delete;
    IMemoryManager& operator=(const IMemoryManager& rhs) = delete;
    IMemoryManager& operator=(IMemoryManager&& rhs) = delete;

    virtual void* alloc(const std::size_t size, const std::size_t alignment) = 0;
    virtual void  dealloc(void* ptr)                                         = 0;

    template<typename T, typename... Args>
    T* alloc_t(Args&&... args)
    {
      void* mem = alloc(sizeof(T), alignof(T));

      if (mem)
      {
        return new (mem) T(std::forward<decltype(args)>(args)...);
      }

      return nullptr;
    }

    template<typename T, typename... Args, typename std::enable_if<std::is_trivially_copyable<T>::value, T>::type* = nullptr>
    T* alloc_array_t(const std::size_t num_elements, Args&&... args)
    {
      if (num_elements)
      {
        T* const array_data = allocate_array<T>(num_elements);

        if (array_data)
        {
          T*       elements     = array_data;
          T* const elements_end = elements + num_elements;

          while (elements != elements_end)
          {
            new (elements++) T(std::forward<decltype(args)>(args)...);
          }
        }

        return array_data;
      }

      return nullptr;
    }

    template<typename T, typename... Args, typename std::enable_if<!std::is_trivially_copyable<T>::value, T>::type* = nullptr>
    T* alloc_array_t(const std::size_t num_elements, Args&&... args)
    {
      T* const array_data = allocate_array<T>(num_elements);

      if (array_data)
      {
        T* elements = array_data;

        for (std::size_t i = 0; i < num_elements; ++i)
        {
          new (elements++) T(std::forward<decltype(args)>(args)...);
        }
      }

      return array_data;
    }
    /*
    template<typename T>
    T* alloc_array_t(const std::size_t num_elements)
    {
      if (num_elements)
      {
        T* array_data = allocate_array<T>(num_elements);
        T* elements   = array_data;

        for (std::size_t i = 0; i < num_elements; ++i)
        {
          new (elements++) T();
        }

        return array_data;
      }

      return nullptr;
    }
    */
    //-------------------------------------------------------------------------------------//
    // NOTE(Shareef): Counted Bytes API
    //-------------------------------------------------------------------------------------//

    void* alloc_counted_bytes(const std::size_t num_bytes, std::size_t alignment = 8)
    {
      return num_bytes ? allocate_array<unsigned char>(num_bytes, alignment) : nullptr;
    }

    void* reallocate(void* const old_ptr, std::size_t new_size, std::size_t new_alignment = 8)
    {
      if (!old_ptr)  // NOTE(Shareef): if old_ptr is null act as malloc
      {
        return alloc_counted_bytes(new_size, new_alignment);
      }

      if (new_size == 0)  // NOTE(Shareef): if new_size is zero act as 'free'
      {
        dealloc(old_ptr);
      }
      else  // NOTE(Shareef): attempt to resize the allocation
      {
        const std::size_t num_bytes_old = array_size(old_ptr);

        if (new_size > num_bytes_old)
        {
          void* const new_mem = alloc_counted_bytes(new_size, new_alignment);
          std::memcpy(new_mem, old_ptr, num_bytes_old);
          return new_mem;
        }
        // NOTE(Shareef): The allocation did not need to be resized.
        return old_ptr;
      }

      return nullptr;
    }

    void deallocate(void* ptr)
    {
      if (ptr)
      {
        dealloc(reinterpret_cast<char*>(ptr) - sizeof(std::size_t));
      }
    }

    //-------------------------------------------------------------------------------------//

    template<typename T, typename... Args>
    T* realloc_array_t(T* oldPtr, std::size_t num_elements)
    {
      T* returned_value = (num_elements != 0) ? oldPtr : nullptr;

      // NOTE(Shareef): reallocate if the old size is not big enough.
      if (returned_value)
      {
        const std::size_t num_elements_old = array_size(oldPtr);

        if (num_elements > num_elements_old)
        {
          returned_value = allocate_array<T>(num_elements);

          std::memcpy(reinterpret_cast<char*>(returned_value),
                      reinterpret_cast<char*>(oldPtr),
                      num_elements_old * sizeof(T));

          dealloc_array_t<T>(oldPtr);
        }
      }
      // NOTE(Shareef): If oldPtr is nullptr then act as 'malloc'.
      else if (!oldPtr)
      {
        returned_value = allocate_array<T>(num_elements);
      }
      // NOTE(Shareef): if num_elements is zero act as 'free'
      else
      {
        dealloc_array_t<T>(oldPtr);
      }

      return returned_value;
    }

    template<typename T>
    static inline std::size_t array_size(const T* ptr)
    {
      if (ptr)
      {
        return *(reinterpret_cast<const std::size_t*>(ptr) - 1);
      }

      return 0;
    }

    template<typename T>
    void dealloc_array_t(T* ptr)
    {
      if (ptr)
      {
        const std::size_t num_elements = array_size(ptr);

        T* elements = ptr;

        for (std::size_t i = 0; i < num_elements; ++i)
        {
          elements->~T();
          ++elements;
        }

        deallocate(ptr);
      }
    }

    template<typename T>
    void dealloc_t(T* ptr)
    {
      if (ptr)
      {
        ptr->~T();
        this->dealloc(ptr);
      }
    }

   private:
    template<typename T>
    T* allocate_array(const std::size_t num_elements, const std::size_t alignment = alignof(T))
    {
      T* returned_value = reinterpret_cast<T*>(alloc((sizeof(T) * num_elements) + sizeof(std::size_t), alignment));

      if (returned_value)
      {
        (*reinterpret_cast<std::size_t*>(returned_value)) = num_elements;

        return reinterpret_cast<T*>(reinterpret_cast<char*>(returned_value) + sizeof(std::size_t));
      }

      return nullptr;
    }
    /*
    template<typename T>
    T* allocate_array(const std::size_t num_elements)
    {
      return allocate_array<T>(num_elements, alignof(T));
    }
    */
  };

  class MemoryManager : public IMemoryManager
  {
   public:
    template<typename T,
             typename = std::enable_if_t<std::is_convertible<T*, IMemoryManager*>::value>>
    static constexpr std::size_t header_size(void)
    {
      return T::header_size;
    }

   private:
    char* const m_MemoryBlockBegin;
    char* const m_MemoryBlockEnd;

   protected:
    MemoryManager(char* memory_block, std::size_t memory_block_size);
    ~MemoryManager() = default;

   public:
    MemoryManager(const MemoryManager& rhs) = delete;
    MemoryManager(MemoryManager&& rhs)      = delete;
    MemoryManager& operator=(const MemoryManager& rhs) = delete;
    MemoryManager& operator=(MemoryManager&& rhs) = delete;

    inline char*       begin() const { return this->m_MemoryBlockBegin; }
    inline char*       end() const { return this->m_MemoryBlockEnd; }
    inline std::size_t size() const { return end() - begin(); }
  };
}  // namespace tide

#endif /* IMEMORY_MANAGER_HPP */
