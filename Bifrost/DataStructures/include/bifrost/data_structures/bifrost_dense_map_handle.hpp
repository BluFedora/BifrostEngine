/******************************************************************************/
/*!
  @file         bifrost_dense_map_handle.hpp
  @author       Shareef Abdoul-Raheem
  @par 
  @brief
    Inspired By:
      [http://bitsquid.blogspot.com/2011/09/managing-decoupling-part-4-id-lookup.html]
*/
/******************************************************************************/
#ifndef BIFROST_DENSE_MAP_HANDLE_HPP
#define BIFROST_DENSE_MAP_HANDLE_HPP

#include <cstdint> /* uint32_t, uint16_t */
#include <limits>  /* numeric_limits<T>  */

namespace bf
{
  namespace dense_map
  {
    using ID_t         = std::uint32_t;  //!< The type used for an ID in a DenseMap.
    using MaxObjects_t = std::uint16_t;  //!< The type used for indexing into a DenseMap.

    static constexpr ID_t INDEX_MASK = std::numeric_limits<MaxObjects_t>::max();
  }  // namespace dense_map

  template<typename TObject>
  class DenseMap;

  template<typename T>
  class DenseMapHandle final
  {
    friend class DenseMap<T>;

   private:
    dense_map::ID_t id;

   public:
    DenseMapHandle(const std::uint32_t id = dense_map::INDEX_MASK) :
      id(id)
    {
    }

    [[nodiscard]] dense_map::ID_t toID() const { return id; } 

    [[nodiscard]] bool operator==(const DenseMapHandle<T>& rhs) const
    {
      return id == rhs.id;
    }

    [[nodiscard]] bool operator!=(const DenseMapHandle<T>& rhs) const
    {
      return id != rhs.id;
    }

    [[nodiscard]] bool isValid() const
    {
      return id != dense_map::INDEX_MASK;
    }
  };
}  // namespace bifrost

#endif /* BIFROST_DENSE_MAP_HANDLE_HPP */
