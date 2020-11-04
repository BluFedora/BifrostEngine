#ifndef BIFROST_NON_COPY_MOVE_HPP
#define BIFROST_NON_COPY_MOVE_HPP

namespace bf
{
  template<typename T>
  class bfNonCopyable  // NOLINT(hicpp-special-member-functions)
  {
   public:
    bfNonCopyable(const bfNonCopyable&) = delete;
    explicit bfNonCopyable(const T&)    = delete;
    bfNonCopyable& operator=(const T&) = delete;

   //protected:
    bfNonCopyable()  = default;
    ~bfNonCopyable() = default;
  };

  template<typename T>
  class bfNonMoveable  // NOLINT(hicpp-special-member-functions)
  {
   public:
    bfNonMoveable(bfNonMoveable&&) = delete;
    explicit bfNonMoveable(T&&)    = delete;
    bfNonMoveable& operator=(T&&) = delete;

   //protected:
    bfNonMoveable()  = default;
    ~bfNonMoveable() = default;
  };

  // clang-format off
  template<typename T>
  class bfNonCopyMoveable : public bfNonCopyable<T>, public bfNonMoveable<T>
  // clang-format on
  {
  };
}  // namespace bifrost

#endif /* BIFROST_NON_COPY_MOVE_HPP */