/******************************************************************************/
/*!
  @file         bifrost_dense_map.hpp
  @author       Shareef Abdoul-Raheem
  @par 
  @brief
    Inspired By:
      [http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html]
*/
/******************************************************************************/
#ifndef BIFROST_DENSE_MAP_HPP
#define BIFROST_DENSE_MAP_HPP

#include "bifrost_array.hpp"            /* Array<T>          */
#include "bifrost_dense_map_handle.hpp" /* Handle<T>         */
#include <utility>                      /* move              */

namespace bifrost
{
  /*!
   * @brief
   *    The DenseMap is used for fast addition and removal of
   *    elements while keeping a cache coherent array of objects.
   *
   *    Made for faster (frequent) Insert(s) and Remove(s) relative to Vector
   *    While keeping a cache coherent dense array.
   */
  template<typename TObject>
  class DenseMap final
  {
   private:
    /*!
     * @brief
     *    The Index used to manage the Indices in the SparseArray.
     */
    struct Index
    {
     public:
      dense_map::ID_t         id;     //!< Used to check if the passed in ID is correct.
      dense_map::MaxObjects_t index;  //!< The actual index of the object.
      dense_map::MaxObjects_t next;   //!< The next free index in the SparseArray.

     public:
      Index(dense_map::ID_t id, dense_map::MaxObjects_t i, dense_map::MaxObjects_t n) :
        id(id),
        index(i),
        next(n)
      {
      }
    };

   public:
    struct Proxy
    {
      TObject         data;
      dense_map::ID_t id;

      template<typename... Args>
      Proxy(dense_map::ID_t id, Args&&... args) :
        data(std::forward<decltype(args)>(args)...),
        id(id)
      {
      }

      TObject*       operator->() { return &data; }
      const TObject* operator->() const { return &data; }
      TObject&       operator*() { return data; }
      const TObject& operator*() const { return data; }

      operator TObject&() { return data; }
      operator const TObject&() const { return data; }
    };

    class iterator
    {
     public:
      typedef iterator    self_type;
      typedef TObject     value_type;
      typedef value_type& reference;
      typedef value_type* pointer;

     protected:
      Proxy* m_Position;
      Proxy* m_End;

     public:
      explicit iterator(Proxy* pos, Proxy* end) :
        m_Position{pos},
        m_End{end}
      {
      }

      [[nodiscard]] self_type next() const
      {
        self_type it = (*this);
        ++it;
        return it;
      }

      self_type operator++(int)  // Post-fix
      {
        self_type it = (*this);
        ++(*this);
        return it;
      }

      self_type& operator++()  // Pre-fix
      {
        ++m_Position;
        return *this;
      }

      bool operator==(const iterator& rhs) const
      {
        return m_Position == rhs.m_Position;
      }

      bool operator==(pointer rhs) const
      {
        return m_Position == rhs;
      }

      bool operator!=(const iterator& rhs) const
      {
        return m_Position != rhs.m_Position;
      }

      bool operator!=(pointer rhs) const
      {
        return m_Position != rhs;
      }

      pointer operator->()
      {
        return &*m_Position;
      }

      reference operator*()
      {
        return *m_Position;
      }

      const value_type* operator->() const
      {
        return &*m_Position;
      }

      const value_type& operator*() const
      {
        return *m_Position;
      }

      [[nodiscard]] pointer value() const
      {
        return m_Position;
      }
    };

    using ObjectArray    = Array<Proxy>;
    using IndexArray     = Array<Index>;
    using const_iterator = iterator;

   private:
    ObjectArray m_DenseArray;     //!< The actual dense array of objects
    IndexArray  m_SparseIndices;  //!< Used to manage the indices of the next free index.
    std::size_t m_NextSparse;     //!< Keeps track of the next free index and whether or not to grow.
    std::size_t m_NextRemove;     //!< Keeps track of the remove index.

   public:
    /*!
     * @brief
     *    Constructs a new empty DenseMap.
     */
    explicit DenseMap(IMemoryManager& memory) :
      m_DenseArray(memory),
      m_SparseIndices(memory),
      m_NextSparse(0),
      m_NextRemove(0)
    {
    }

    template<typename... Args>
    DenseMapHandle<TObject> add(Args&&... args)
    {
      Index& in = getNextIndex();

      in.id += 0x10000;
      in.index     = dense_map::MaxObjects_t(m_DenseArray.size());
      m_NextSparse = in.next;

      m_DenseArray.emplace(in.id, std::forward<decltype(args)>(args)...);

      return in.id;
    }

    /*!
     * @brief
     *    Check if the passed in ID is valid in this SparseArray.
     *
     * @param id
     *    The id to be checking against.
     *
     * @return
     *  true  - if the element can be found in this SparseArray
     *  false - if the ID is invalid and should not be used to get / remove an object.
     */
    bool has(DenseMapHandle<TObject> id) const
    {
      const auto index = id.id & dense_map::INDEX_MASK;

      if (index < m_SparseIndices.size())
      {
        const Index& in = m_SparseIndices[index];
        return in.id == id.id && in.index != dense_map::INDEX_MASK;
      }

      return false;
    }

    /*!
     * @brief
     *    Finds the object from the associated ID.
     *    This function does no bounds checking so be sure to call
     *    SparseArray<T>::has to guarantee safety.
     *
     *    This is the limitation of a decision I made to return a reference.
     *    Otherwise this function would return a pointer with nullptr indicating failure.
     *
     * @param id
     *    The id of the object to be found.
     *
     * @return
     *  The Object associated with that particular ID.
     */
    TObject& find(const DenseMapHandle<TObject> id)
    {
      const Index& in = m_SparseIndices[id.id & dense_map::INDEX_MASK];
      return m_DenseArray[in.index].data;
    }

    /*!
     * @brief
     *  Removes the object of the specified ID from this SparseArray.
     *  This function will cause a move of the last elements.
     *  Complexity: O(1)
     *
     * @param id
     *  The ID of the object to be removed.
     */
    void remove(DenseMapHandle<TObject> id)
    {
      if (has(id))
      {
        id.id      = id.id & dense_map::INDEX_MASK;
        Index& in  = m_SparseIndices[id.id];
        auto&  obj = m_DenseArray[in.index];
        obj        = std::move(m_DenseArray.back());

        m_SparseIndices[obj.id & dense_map::INDEX_MASK].index = in.index;
        in.index                                              = dense_map::INDEX_MASK;
        m_SparseIndices[m_NextRemove].next                    = id.id;
        m_NextRemove                                          = id.id;
        m_DenseArray.pop();
      }
      else
      {
        // __debugbreak();
      }
    }

    void removeAll()
    {
      // Invalidate all pointers into this Array.
      for (auto& index : this->m_SparseIndices)
      {
        index.index = dense_map::INDEX_MASK;
      }
      this->m_DenseArray.clear();
      this->m_SparseIndices.clear();
      this->m_NextSparse = 0;
      this->m_NextRemove = 0;
    }

    // NOTE(Shareef): STL Standard Container Functions

    iterator                  begin() { return iterator(m_DenseArray.data(), m_DenseArray.data() + size()); }
    const_iterator            begin() const { return iterator(m_DenseArray.data(), m_DenseArray.data() + size()); }
    TObject&                  at(const std::size_t index) { return m_DenseArray.at(index); }
    const TObject&            at(const std::size_t index) const { return m_DenseArray.at(index); }
    [[nodiscard]] std::size_t size() const { return m_DenseArray.size(); }
    TObject&                  operator[](const std::size_t index) { return m_DenseArray[index]; }
    const TObject&            operator[](const std::size_t index) const { return m_DenseArray[index]; }
    iterator                  end() { return iterator(m_DenseArray.data() + size(), m_DenseArray.data() + size()); }
    const_iterator            end() const { return iterator(m_DenseArray.data() + size(), m_DenseArray.data() + size()); }
    void                      reserve(std::size_t size) { m_DenseArray.reserve(size); }
    Proxy*                    data() { return m_DenseArray.data(); }
    const Proxy*              data() const { return m_DenseArray.data(); }

   private:
    Index& getNextIndex()
    {
      if (m_NextSparse == m_SparseIndices.size())
      {
        const dense_map::ID_t next_sparse = static_cast<dense_map::ID_t>(m_NextSparse);
        return m_SparseIndices.emplace(next_sparse, dense_map::INDEX_MASK, next_sparse + 1);
      }

      return m_SparseIndices[this->m_NextSparse];
    }
  };
}  // namespace bifrost

#endif /* BIFROST_DENSE_MAP_HPP */
