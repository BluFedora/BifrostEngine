/******************************************************************************/
/*!
  @file   bifrost_array.hpp
  @author Shareef Abdoul-Raheem
  @brief
    Safe(-ish) C++ wrapper around the bifrost_array_t.h API.
*/
/******************************************************************************/
#ifndef BIFROST_ARRAY_HPP
#define BIFROST_ARRAY_HPP

#include "bifrost/memory/bifrost_imemory_manager.hpp" /* IMemoryManager       */
#include "bifrost_array_t.h"                          /* bfArray_*            */

#include <cassert> /* assert */

namespace bifrost
{
  template<typename T>
  class Array final
  {
   private:
    static void* allocate(void* user_data, void* ptr, std::size_t new_size)
    {
      auto* const allocator = static_cast<IMemoryManager*>(user_data);

      if (ptr)
      {
        allocator->deallocate(ptr);
        ptr = nullptr;
      }
      else
      {
        ptr = allocator->allocate(new_size);
      }

      return ptr;
    }

   private:
    mutable T* m_Data;

   public:
    explicit Array(IMemoryManager& memory, const std::size_t alignment = alignof(T)) :
      m_Data{static_cast<T*>(bfArray_new_(&allocate, sizeof(T), alignment, &memory))}
    {
    }

    explicit Array(IMemoryManager& memory, std::initializer_list<T>&& values, const std::size_t alignment = alignof(T)) :
      m_Data{static_cast<T*>(bfArray_new_(&allocate, sizeof(T), alignment, &memory))}
    {
      reserve(values.size());

      for (const T& value : values)
      {
        push(value);
      }
    }

    Array(const Array& rhs) noexcept :
      m_Data{static_cast<T*>(bfArray_new_(&allocate, sizeof(T), alignof(T), &rhs.memory()))}
    {
      copyFrom(rhs);
    }

    Array(Array&& rhs) noexcept :
      m_Data{rhs.m_Data}
    {
      rhs.m_Data = nullptr;
    }

    Array& operator=(const T& rhs) noexcept
    {
      if (this != &rhs)
      {
        clear();
        copyFrom(rhs);
      }

      return *this;
    }

    Array& operator=(T&& rhs) noexcept
    {
      if (this != &rhs)
      {
        m_Data = std::exchange(rhs.m_Data, nullptr);
      }

      return *this;
    }

    IMemoryManager& memory() const
    {
      return *static_cast<IMemoryManager*>(bfArray_userData(rawData()));
    }

    [[nodiscard]] T*          begin() { return static_cast<T*>(::bfArray_begin(rawData())); }
    [[nodiscard]] const T*    begin() const { return static_cast<const T*>(::bfArray_begin(rawData())); }
    [[nodiscard]] T*          end() { return static_cast<T*>(::bfArray_end(rawData())); }
    [[nodiscard]] const T*    end() const { return static_cast<const T*>(::bfArray_end(rawData())); }
    [[nodiscard]] T&          back() { return *static_cast<T*>(::bfArray_back(rawData())); }
    [[nodiscard]] const T&    back() const { return *static_cast<const T*>(::bfArray_back(rawData())); }
    [[nodiscard]] std::size_t size() const { return ::bfArray_size(rawData()); }
    [[nodiscard]] std::size_t capacity() const { return ::bfArray_capacity(rawData()); }
    [[nodiscard]] T*          data() { return begin(); }
    [[nodiscard]] const T*    data() const { return begin(); }
    [[nodiscard]] bool        isEmpty() const { return size() == 0u; }

    /*
    void copy(Array<T>& src, std::size_t num_elements)
    {
      // ::bfArray_copy(rawData(), &src.m_Data, num_elements);
      // TODO: This function sucks, what to make it better?
      resize(num_elements);
      std::copy(src.m_Data, src.m_Data + num_elements, m_Data);
    }
    */

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
      // TODO(SR): This only works if T is trivially locatable.
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

    template<typename... Args>
    void insert(std::size_t index, Args&&... args)
    {
      // TODO(SR): This only works if T is trivially locatable.
      new (bfArray_insertEmplace(rawData(), index)) T(std::forward<Args>(args)...);
    }

    T& at(std::size_t index)
    {
      return *static_cast<T*>(::bfArray_at(rawData(), index));
    }

    const T& at(std::size_t index) const
    {
      return *static_cast<const T*>(::bfArray_at(rawData(), index));
    }

    T& operator[](std::size_t index)
    {
      // TODO: Only use Array::at in debug mode.
      return at(index);  // return m_Data[index];
    }

    const T& operator[](std::size_t index) const
    {
      // TODO: Only use Array::at in debug mode.
      return at(index);  // return m_Data[index];
    }

    T* binarySearchRange(std::size_t bgn, std::size_t end, const T& key, bfArrayFindCompare compare = nullptr)
    {
      return static_cast<T*>(::bfArray_binarySearchRange(rawData(), bgn, end, &key, compare));
    }

    T* binarySearch(const T& key, bfArrayFindCompare compare = nullptr)
    {
      return static_cast<T*>(::bfArray_binarySearch(rawData(), &key, compare));
    }

    std::size_t findInRange(std::size_t bgn, std::size_t end, const T& key, bfArrayFindCompare compare = nullptr)
    {
      return ::bfArray_findInRange(rawData(), bgn, end, &key, compare);
    }

    std::size_t find(const T& key, bfArrayFindCompare compare = nullptr)
    {
      return ::bfArray_find(rawData(), &key, compare);
    }

    void removeAt(const std::size_t index)
    {
      // TODO(SR): This only works if T is trivially locatable.
      m_Data[index].~T();
      ::bfArray_removeAt(rawData(), index);
    }

    void swapAndPopAt(const std::size_t index)
    {
      // TODO(SR): This only works if T is trivially locatable.
      m_Data[index].~T();
      ::bfArray_swapAndPopAt(rawData(), index);
    }

    void pop()
    {
      // NOTE(Shareef):
      //   The C version returns the element since
      //   dtors does not exist for C but C++
      //   lifetime semantics do not allow that type of API.
      back().~T();
      ::bfArray_pop(rawData());
    }

    void sortRange(size_t bgn, size_t end, ArraySortCompare compare)
    {
      // TODO(SR): This only works if T is "trivially relocatable".
      ::bfArray_sortRange(rawData(), bgn, end, compare);
    }

    void sort(ArraySortCompare compare)
    {
      // TODO(SR): This only works if T is "trivially relocatable".
      ::bfArray_sort(rawData(), compare);
    }

    std::size_t indexOf(const T* element)
    {
      assert(begin() <= element && element < end() && "'element' must be within this Array");

      return element - begin();
    }

    ~Array()
    {
      if (m_Data)
      {
        clear();
        ::bfArray_delete(rawData());
      }
    }

   private:
    void copyFrom(const Array& rhs)
    {
      reserve(rhs.size());

      for (const T& value : rhs)
      {
        push(value);
      }
    }

    void** rawData()
    {
      return reinterpret_cast<void**>(&m_Data);
    }

    void** rawData() const
    {
      return reinterpret_cast<void**>(&m_Data);
    }
  };

  template<typename T>
  struct ArrayView
  {
    T*          data;
    std::size_t data_size;

    ArrayView(T* data, std::size_t size) :
      data{data},
      data_size{size}
    {
    }

    T* begin()
    {
      return data;
    }

    T* end()
    {
      return data + data_size;
    }
  };
}  // namespace bifrost

#endif /* BIFROST_ARRAY_HPP */
