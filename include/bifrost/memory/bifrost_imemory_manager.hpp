/******************************************************************************/
/*!
* @file   bifrost_imemory_manager.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*  Outlines a basic interface for the various types of memory managers.
*
* @version 0.0.1
* @date    2019-12-26
*
* @copyright Copyright (c) 2019
*/
/******************************************************************************/
#ifndef BIFROST_IMEMORY_MANAGER_HPP
#define BIFROST_IMEMORY_MANAGER_HPP

#include <cstddef>     /* size_t                       */
#include <cstring>     /* memcpy                       */
#include <type_traits> /* is_convertible, enable_if_t  */
#include <utility>     /* forward, move                */

#ifndef BIFROST_MEMORY_DEBUG_WIPE_MEMORY
// NOTE(Shareef):
//   Set to 0 if you want faster allocations
//   at the cost of less safety.
#define BIFROST_MEMORY_DEBUG_WIPE_MEMORY 1
#define BIFROST_MEMORY_DEBUG_SIGNATURE 0xCD
#define BIFROST_MEMORY_DEBUG_ALIGNMENT_PAD 0xFE
#endif /* BIFROST_MEMORY_DEBUG_WIPE_MEMORY */

namespace bifrost
{
  /*!
   * @brief
   *   Interface for all memory managers.
   */
  class IMemoryManager
  {
   private:
   /*!
    * @brief
    *   Header for all array API allocated blocks.
    */
    struct ArrayHeader final
    {
      std::size_t size;
    };

   protected:
    //-------------------------------------------------------------------------------------//
    // Only Subclasses should be able to call these
    //-------------------------------------------------------------------------------------//

    IMemoryManager()  = default;
    ~IMemoryManager() = default;

   public:
    //-------------------------------------------------------------------------------------//
    // Disallow Copies and Moves
    //-------------------------------------------------------------------------------------//

    IMemoryManager(const IMemoryManager& rhs)     = delete;
    IMemoryManager(IMemoryManager&& rhs) noexcept = delete;
    IMemoryManager& operator=(const IMemoryManager& rhs) = delete;
    IMemoryManager& operator=(IMemoryManager&& rhs) noexcept = delete;

    //-------------------------------------------------------------------------------------//
    // Base Interface
    //-------------------------------------------------------------------------------------//

    /*!
     * @brief
     *   Allocates a block of bytes of \p size.
     *
     * @param[in] size
     *   The size of the block to allocate.
     *
     * @return void*
     *   The begginning to the allocated block.
     *   nullptr if the request could not be fulfilled.
     */
    virtual void* allocate(std::size_t size) = 0;

    /*!
     * @brief
     *   Frees the memory pointer to by \p ptr.
     *
     * @param ptr
     *   The pointer to free memory from.
     *   Must not be a nullptr.
     *   Must have been allocated with 'IMemoryManager::allocate'.
     */
    virtual void deallocate(void* ptr) = 0;

    //-------------------------------------------------------------------------------------//
    // Aligned API
    //-------------------------------------------------------------------------------------//

    /*!
     * @brief
     *   Allocates a block of bytes of \p size, the returned pointer will be on an address
     *   aligned to \p alignment.
     *
     * @param size
     *   The size of the block to allocate.
     *
     * @param alignment
     *   The alignment of the allocated block.
     *   Must be a power of two.
     *
     * @return void*
     *   The begginning to the allocated block.
     *   nullptr if the request could not be fulfilled.
     */
    void* allocateAligned(std::size_t size, std::size_t alignment);

    /*!
     * @brief
     *  Frees the memory pointer to by \p ptr.
     *
     * @param ptr
     *   The pointer to free memory from.
     *   Freeing a nullptr is safe.
     *   Must have been allocated with 'IMemoryManager::allocateAligned'.
     */
    void deallocateAligned(void* ptr);

    //-------------------------------------------------------------------------------------//
    // Templated API
    //-------------------------------------------------------------------------------------//

    /*!
     * @brief
     *   Allocates a new T object calling it's constructor.
     *
     * @tparam T
     *   The type of the object to allocate.
     *
     * @tparam Args
     *   Constructor argument type.
     *
     * @param args
     *   Constructor arguments.
     *
     * @return T*
     *   A pointer to the new object.
     *   nullptr if the request could not be fulfilled.
     */
    template<typename T, typename... Args>
    T* allocateT(Args&&... args)
    {
      void* const mem_block = allocate(sizeof(T));

      return mem_block ? new (mem_block) T(std::forward<decltype(args)>(args)...) : nullptr;
    }

    /*!
     * @brief
     *   Frees the memory pointer to by \p ptr.
     *
     * @tparam T
     *   The type of the object to deallocate.
     *
     * @param ptr
     *   The pointer to free memory from.
     *   Freeing a nullptr is safe.
     *   Must have been allocated with 'IMemoryManager::allocateT'.
     */
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

    /*!
     * @brief
     *   Allocates an array of type T.
     *
     * @tparam T
     *   The type of the array to allocate.
     *   Must be default constructable.
     *
     * @param num_elements
     *   The number of the elements the array contains.
     *
     * @param array_alignment
     *   What to align each the array to.
     *   Must be a power of two.
     *
     * @return T*
     *   The allocated array with num_elements size.
     *   nullptr if the request could not be fulfilled.
     */
    template<typename T>
    T* allocateArray(std::size_t num_elements, std::size_t array_alignment = alignof(T))
    {
      if (num_elements)
      {
        T* const array_data = allocateArrayTrivial<T>(num_elements, array_alignment);

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

    /*!
     * @brief
     *   Allocates an array of type T without calling default constructors.
     *
     * @tparam T
     *   The type of the array to allocate.
     *
     * @tparam
     *   SFINAE checking to make sure T is trivial.
     *
     * @param num_elements
     *   The number of the elements the array contains.
     *
     * @param array_alignment
     *   What to align each the array to.
     *   Must be a power of two.
     *
     * @return T*
     *   The allocated array with num_elements size.
     *   nullptr if the request could not be fulfilled.
     */
    template<typename T, typename std::enable_if<std::is_trivially_copyable<T>::value, T>::type* = nullptr>
    T* allocateArrayTrivial(std::size_t num_elements, std::size_t array_alignment = alignof(T))
    {
      if (num_elements)
      {
        T* const array_data = allocateAligned(sizeof(ArrayHeader), sizeof(T) * num_elements, array_alignment);

        if (array_data)
        {
          ArrayHeader* header = grabHeader(sizeof(ArrayHeader), array_data);
          header->size        = num_elements;
        }

        return array_data;
      }

      return nullptr;
    }

    /*!
     * @brief
     *   Returns size of the array. (number of elements)
     *
     * @tparam T
     *   The type of the array.
     *
     * @param ptr
     *   The array to get the size of.
     *   Must have been allocated with
     *     'IMemoryManager::allocateArray'        or
     *     'IMemoryManager::allocateArrayTrivial' or
     *     'IMemoryManager::arrayResize'.
     *
     * @return std::size_t
     *   The number of elements the array has.
     */
    template<typename T>
    std::size_t arraySize(const T* const ptr)
    {
      return static_cast<ArrayHeader*>(grabHeader(sizeof(ArrayHeader), const_cast<T*>(ptr)))->size;
    }

    /*!
     * @brief
     *   Resizes an array potentially giving you a new allocation.
     *   Acts the same as the C stdlib's realloc.
     *   The new array has the old array's elements 'std::move'-d into them.
     *
     * @tparam T
     *   The type of the array.
     *
     * @param old_ptr
     *   The array that needs to be resized.
     *   If this is null then this acts as a malloc / IMemoryManager::allocateArray.
     *
     * @param num_elements
     *   The new size you want the array to be.
     *   If this size is 0 then this acts as free / IMemoryManager::deallocateArray.
     *   Otherwise a resize is attempted.
     *
     * @param array_alignment
     *   The alignment of the array.
     *
     * @return T*
     *   The allocated array with num_elements size.
     *   nullptr if the array was resized to 0 or resize could not happen.
     */
    template<typename T>
    T* arrayResize(T* const old_ptr, std::size_t num_elements, std::size_t array_alignment = alignof(T))
    {
      if (!old_ptr)  // NOTE(Shareef): if old_ptr is null act as malloc
      {
        return allocateArray<T>(num_elements, array_alignment);
      }

      if (num_elements == 0)  // NOTE(Shareef): if new_size is zero act as 'free'
      {
        deallocateArray(old_ptr);
      }
      else  // NOTE(Shareef): attempt to resize the allocation
      {
        ArrayHeader*      header   = static_cast<ArrayHeader*>(grabHeader(sizeof(ArrayHeader), old_ptr));
        const std::size_t old_size = header->size;

        if (num_elements > old_size)
        {
          T* const new_ptr = allocateArrayTrivial<T>(num_elements, array_alignment);

          if (new_ptr)
          {
            for (std::size_t i = 0; i < old_size; ++i)
            {
              new (new_ptr + i) T(std::move(old_ptr[i]));
            }

            deallocateArray(old_ptr);

            for (std::size_t i = old_size; i < num_elements; ++i)
            {
              new (new_ptr + i) T();
            }
          }

          return new_ptr;
        }

        // NOTE(Shareef): The allocation did not need to be resized.
        return old_ptr;
      }

      return nullptr;
    }

    /*!
     * @brief
     *   Frees the memory pointer to by \p ptr.
     *
     * @tparam T
     *   The type of the object to deallocate.
     *
     * @param ptr
     *   The array that needs to be freed.
     *   Freeing a nullptr is safe.
     *   Must have been allocated with
     *     'IMemoryManager::allocateArray'        or
     *     'IMemoryManager::allocateArrayTrivial' or
     *     'IMemoryManager::arrayResize'.
     */
    template<typename T>
    void deallocateArray(T* ptr)
    {
      if (ptr)
      {
        ArrayHeader* header       = static_cast<ArrayHeader*>(grabHeader(sizeof(ArrayHeader), ptr));
        T*           elements     = ptr;
        T* const     elements_end = elements + header->size;

        while (elements != elements_end)
        {
          elements->~T();
          ++elements;
        }

        deallocateAligned(ptr);
      }
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

#endif /* BIFROST_IMEMORY_MANAGER_HPP */
