/******************************************************************************/
/*!
  @file   bifrost_imemory_manager.hpp
  @author Shareef Abdoul-Raheem
  @par    
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

namespace bifrost
{
  class IMemoryManager
  {
   private:
    struct ArrayHeader final
    {
      std::size_t size;
    };

   protected:
    IMemoryManager(void) = default;
    ~IMemoryManager()    = default;

   public:
    // Disallow Copies and Moves.
    IMemoryManager(const IMemoryManager& rhs) = delete;
    IMemoryManager(IMemoryManager&& rhs)      = delete;
    IMemoryManager& operator=(const IMemoryManager& rhs) = delete;
    IMemoryManager& operator=(IMemoryManager&& rhs) = delete;

    //-------------------------------------------------------------------------------------//
    // Base Interface
    //-------------------------------------------------------------------------------------//

    virtual void* allocate(std::size_t size) = 0;
    virtual void  deallocate(void* ptr)      = 0;

    //-------------------------------------------------------------------------------------//
    // Aligned API
    //-------------------------------------------------------------------------------------//

    void* allocateAligned(std::size_t size, std::size_t alignment);
    void  deallocateAligned(void* ptr);

    //-------------------------------------------------------------------------------------//
    // Templated API
    //-------------------------------------------------------------------------------------//

    template<typename T, typename... Args>
    T* allocateT(Args&&... args)
    {
      void* const mem_block = allocate(sizeof(T));

      return mem_block ? new (mem_block) T(std::forward<decltype(args)>(args)...) : nullptr;
    }

    template<typename T>
    void deallocateT(T* ptr)
    {
      if (ptr)
      {
        ptr->~T();
        deallocate(ptr);
      }
    }

    //-------------------------------------------------------------------------------------//
    // Array API
    //-------------------------------------------------------------------------------------//

    template<typename T>
    T* allocateArray(std::size_t num_elements, std::size_t element_alignment = alignof(T))
    {
      if (num_elements)
      {
        T* const array_data = allocateArrayTrivial<T>(num_elements, element_alignment);

        if (array_data)
        {
          T*       elements     = array_data;
          T* const elements_end = elements + num_elements;

          while (elements != elements_end)
          {
            new (elements++) T();
          }
        }

        return array_data;
      }

      return nullptr;
    }

    template<typename T, typename std::enable_if<std::is_trivially_copyable<T>::value, T>::type* = nullptr>
    T* allocateArrayTrivial(std::size_t num_elements, std::size_t element_alignment = alignof(T))
    {
      if (num_elements)
      {
        T* const array_data = allocateAligned(sizeof(ArrayHeader), sizeof(T) * num_elements, element_alignment);

        if (array_data)
        {
          ArrayHeader* header = grabHeader(sizeof(ArrayHeader), array_data);
          header->size        = num_elements;
        }

        return array_data;
      }

      return nullptr;
    }

    template<typename T>
    T* arrayResize(T* const old_ptr, std::size_t num_elements, std::size_t element_alignment = alignof(T))
    {
      if (!old_ptr)  // NOTE(Shareef): if old_ptr is null act as malloc
      {
        return allocateArray<T>(num_elements, element_alignment);
      }

      if (num_elements == 0)  // NOTE(Shareef): if new_size is zero act as 'free'
      {
        deallocateArray(old_ptr);
      }
      else  // NOTE(Shareef): attempt to resize the allocation
      {
        ArrayHeader*      header   = grabHeader(sizeof(ArrayHeader), old_ptr);
        const std::size_t old_size = header->size;

        if (num_elements > old_size)
        {
          T* const new_ptr = allocateArrayTrivial<T>(num_elements, element_alignment);

          for (std::size_t i = 0; i < old_size; ++i)
          {
            new (new_ptr + i) T(std::move(old_ptr[i]));
          }

          deallocateArray(old_ptr);

          for (std::size_t i = old_size; i < num_elements; ++i)
          {
            new (new_ptr + i) T();
          }

          return new_ptr;
        }

        // NOTE(Shareef): The allocation did not need to be resized.
        return old_ptr;
      }

      return nullptr;
    }

    template<typename T>
    void deallocateArray(T* ptr)
    {
      ArrayHeader* header       = grabHeader(sizeof(ArrayHeader), ptr);
      T*           elements     = ptr;
      T* const     elements_end = elements + header->size;

      while (elements != elements_end)
      {
        elements->~T();
        ++elements;
      }

      deallocateAligned(ptr);
    }

   private:
    void*        allocateAligned(std::size_t header_size, std::size_t size, std::size_t alignment);
    static void* grabHeader(std::size_t header_size, void* ptr);
    void         deallocateAligned(std::size_t header_size, void* ptr);
  };

  class MemoryManager : public IMemoryManager
  {
   public:
    template<typename T,
             typename = std::enable_if_t<std::is_convertible<T*, IMemoryManager*>::value>>
    static constexpr std::size_t header_size()
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

    char*       begin() const { return m_MemoryBlockBegin; }
    char*       end() const { return m_MemoryBlockEnd; }
    std::size_t size() const { return end() - begin(); }
  };
}  // namespace bifrost

#endif /* IMEMORY_MANAGER_HPP */
