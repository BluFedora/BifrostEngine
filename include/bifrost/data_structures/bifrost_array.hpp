/******************************************************************************/
/*!
  @file   bifrost_array.hpp
  @author Shareef Abdoul-Raheem
  @par   
  @brief  
*/
/******************************************************************************/
#ifndef BIFROST_ARRAY_HPP
#define BIFROST_ARRAY_HPP

#include "bifrost/memory/bifrost_imemory_manager.hpp" /* IMemoryManager       */
#include "bifrost/utility/bifrost_non_copy_move.hpp"  /* bfNonCopyMoveable<T> */
#include "bifrost_array_t.h"                          /* bfArray_*            */

namespace bifrost
{
  template<typename T>
  class Array : bfNonCopyMoveable<T>
  {
   private:
    static void* allocate(void* user_data, void* ptr, std::size_t new_size)
    {
      Array<T>* array = static_cast<Array<T>*>(user_data);

      if (ptr)
      {
        array->m_Memory.dealloc(ptr);
        ptr = nullptr;
      }
      else
      {
        ptr = array->m_Memory.alloc(new_size, alignof(T));
      }

      return ptr;
    }

   private:
    IMemoryManager& m_Memory;
    mutable T*      m_Data;

   public:
    explicit Array(IMemoryManager& memory) :
      m_Memory{memory},
      m_Data{bfArray_new(T, &allocate, this)}
    {
    }

    T*                        begin() { return static_cast<T*>(::bfArray_begin(rawData())); }
    const T*                  begin() const { return static_cast<const T*>(::bfArray_begin(rawData())); }
    T*                        end() { return static_cast<T*>(::bfArray_end(rawData())); }
    const T*                  end() const { return static_cast<const T*>(::bfArray_end(rawData())); }
    T&                        back() { return *static_cast<T*>(::bfArray_back(rawData())); }
    const T&                  back() const { return *static_cast<const T*>(::bfArray_back(rawData())); }
    [[nodiscard]] std::size_t size() const { return ::bfArray_size(rawData()); }
    [[nodiscard]] std::size_t capacity() const { return ::bfArray_capacity(rawData()); }

    void copy(Array<T>& src, std::size_t num_elements)
    {
      ::bfArray_copy(rawData(), &src.m_Data, num_elements);
    }

    void clear()
    {
      for (T& element : *this)
      {
        element.~T();
      }

      ::bfArray_clear(rawData());
    }

    void reserve(std::size_t num_elements)
    {
      ::bfArray_reserve(rawData(), num_elements);
    }

    void resize(std::size_t num_elements)
    {
      const std::size_t old_size = size();

      if (num_elements != old_size)
      {
        ::bfArray_resize(rawData(), num_elements);

        if (old_size > num_elements)
        {
          // Shrink
          for (std::size_t i = num_elements; i < old_size; ++i)
          {
            m_Data[i].~T();
          }
        }
        else
        {
          // Grow
          for (std::size_t i = old_size; i < num_elements; ++i)
          {
            new (m_Data + i) T();
          }
        }
      }
    }

    void push(const T& element)
    {
      new (::bfArray_emplace(rawData())) T(element);
    }

    template<typename... Args>
    T& emplace(Args&&... args)
    {
      return *(new (::bfArray_emplace(rawData())) T(std::forward<Args>(args)...));
    }

    template<typename... Args>
    T* emplaceN(std::size_t num_elements, Args&&... args)
    {
      T* elements = ::bfArray_emplaceN(rawData(), num_elements);

      for (std::size_t i = 0; i < num_elements; ++i)
      {
        new (elements) T(std::forward<Args>(args)...);
      }

      return elements;
    }

    T& at(std::size_t index)
    {
      return ::bfArray_at(rawData(), index);
    }

    const T& at(std::size_t index) const
    {
      return ::bfArray_at(rawData(), index);
    }

    T& operator[](std::size_t index)
    {
      return m_Data[index];
    }

    const T& operator[](std::size_t index) const
    {
      return m_Data[index];
    }

    T* binarySearchRange(std::size_t bgn, std::size_t end, const T& key, bfArrayFindCompare compare)
    {
      return static_cast<T*>(::bfArray_binarySearchRange(rawData(), bgn, end, &key, compare));
    }

    T* binarySearch(const T& key, bfArrayFindCompare compare)
    {
      return static_cast<T*>(::bfArray_binarySearch(rawData(), &key, compare));
    }

    std::size_t findInRange(std::size_t bgn, std::size_t end, const T& key, bfArrayFindCompare compare)
    {
      return ::bfArray_findInRange(rawData(), bgn, end, &key, compare);
    }

    std::size_t find(const T& key, bfArrayFindCompare compare)
    {
      return ::bfArray_find(rawData(), &key, compare);
    }

    T& pop()
    {
      return *static_cast<T*>(::bfArray_pop(rawData()));
    }

    void sortRange(size_t bgn, size_t end, ArraySortCompare compare)
    {
      ::bfArray_sortRange(rawData(), bgn, end, compare);
    }

    void sort(ArraySortCompare compare)
    {
      ::bfArray_sort(rawData(), compare);
    }

    ~Array()
    {
      clear();
      ::bfArray_delete(rawData());
    }

   private:
    void** rawData()
    {
      return reinterpret_cast<void**>(&m_Data);
    }

    void** rawData() const
    {
      return reinterpret_cast<void**>(&m_Data);
    }
  };
}  // namespace bifrost

#endif /* BIFROST_ARRAY_HPP */
