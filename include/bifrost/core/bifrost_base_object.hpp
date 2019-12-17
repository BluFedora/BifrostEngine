#ifndef BIFROST_BASE_OBJECT_HPP
#define BIFROST_BASE_OBJECT_HPP

#include "bifrost/meta/bifrost_meta_factory.hpp"     /* Factory<T>           */
#include "bifrost/utility/bifrost_non_copy_move.hpp" /* bfNonCopyMoveable<T> */

class BifrostEngine;

namespace bifrost
{
  // NOTE(Shareef): Use this class as the base type.

  // clang-format off
  class BaseObjectT : private bfNonCopyMoveable<BaseObjectT>, public meta::Factory<BaseObjectT>
  // clang-format on
  {
   protected:
    BaseObjectT(PrivateCtorTag) {}
  };

  // NOTE(Shareef): Inherit from this
  template<typename... T>
  using BaseObject = BaseObjectT::Base<T...>;
}  // namespace bifrost

#endif /* BIFROST_BASE_OBJECT_HPP */
