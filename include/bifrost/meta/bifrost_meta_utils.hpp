#ifndef BIFROST_META_UTILS_HPP
#define BIFROST_META_UTILS_HPP

#include <tuple>       /* tuple_size, get                                         */
#include <type_traits> /* index_sequence, make_index_sequence, remove_reference_t */
#include <utility>     /* forward                                                 */

namespace bifrost::meta
{
  template<class... Ts>
  struct overloaded : public Ts...
  {
    using Ts::operator()...;
  };
  template<class... Ts>
  overloaded(Ts...)->overloaded<Ts...>;

  template<typename Tuple, typename F, std::size_t... Indices>
  constexpr void for_each_impl(Tuple&& tuple, F&& f, std::index_sequence<Indices...>)
  {
    //using swallow = int[];
    //(void)swallow{1,
    //              (f(std::get<Indices>(std::forward<Tuple>(tuple))), void(), int{})...};

    (f(std::get<Indices>(tuple)), ...);
  }

  template<typename Tuple, typename F>
  constexpr void for_each(Tuple&& tuple, F&& f)
  {
    constexpr std::size_t N = std::tuple_size<std::remove_reference_t<Tuple>>::value;
    for_each_impl(
     std::forward<Tuple>(tuple),
     std::forward<F>(f),
     std::make_index_sequence<N>{});
  }
}  // namespace bifrost::meta

#endif /* BIFROST_META_UTILS_HPP */