//
// C++ Math Utilities
//
// References:
//   "What Every Computer Scientist Should Know About Floating-Point Arithmetic"
//      [https://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html]
//
#ifndef BIFROST_MATH_HPP
#define BIFROST_MATH_HPP

#include "bifrost_math.h"
#include "math/bifrost_rect2.hpp"

#include <cmath> /* std::fma */

namespace bf
{
  static constexpr float    k_PI       = 3.14159265358979323846f;
  static constexpr float    k_TwoPI    = k_PI * 2.0f;
  static constexpr float    k_HalfPI   = k_PI * 0.5f;
  static constexpr float    k_RadToDeg = 180.0f / k_PI;
  static constexpr float    k_DegToRad = k_PI / 180.0f;
  static constexpr float    k_Epsilon  = 1.0e-4f;
  static constexpr Vector3f k_XAxis3f  = {1.0f, 0.0f, 0.0f, 0.0f};
  static constexpr Vector3f k_YAxis3f  = {0.0f, 1.0f, 0.0f, 0.0f};
  static constexpr Vector3f k_ZAxis3f  = {0.0f, 0.0f, 1.0f, 0.0f};
}  // namespace bifrost

namespace bf::math
{
  // NOTE(Shareef):
  //   For safe floating point comparisons.
  template<class T>
  typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type isAlmostEqual(T x, T y, int unit_of_least_precision = 3)
  {
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    // unless the result is subnormal
    const auto diff = std::abs(x - y);

    return diff <= std::numeric_limits<T>::epsilon() * std::abs(x + y) * unit_of_least_precision || diff < std::numeric_limits<T>::min();
  }

  // The classic lerp function.
  template<typename T, typename F>
  T lerp(const T& a, F t, const T& b)
  {
    return (a * (F(1.0f) - t)) + (b * t);
  }

  // This is a faster algorithm by algebraically simplifying but there
  // is precision loss when a and b significantly differ in magnitude.
  template<typename T, typename F>
  T lerp2(const T& a, F t, const T& b)
  {
    return a + t * (b - a);
  }

  // [https://devblogs.nvidia.com/lerp-faster-cuda/]
  // fma is typically implemented as a "fused multiply-add CPU instruction".
  template<typename T, typename F>
  T lerp3(const T& a, F t, const T& b)
  {
    return std::fma(t, b, std::fma(-t, a, a));
  }

  // NOTE(Shareef): Float Based Functions
  template<typename T>
  T mapToRange(T min, T value, T max, T new_min, T new_max)
  {
    return ((value - min) / (max - min)) * (new_max - new_min) + new_min;
  }

  // Optimized for going into the 0 - 1 range.
  template<typename T>
  T mapToRange01(T min, T value, T max)
  {
    return ((value - min) / (max - min));
  }

  template<typename T>
  T clamp(T min, T value, T max)
  {
    return std::min(std::max(min, value), max);
  }

  // Function Aliases from the C-API

  inline constexpr const auto& alignf   = &bfMathAlignf;
  inline constexpr const auto& invLerpf = &bfMathInvLerpf;
  inline constexpr const auto& remapf   = &bfMathRemapf;
}  // namespace bifrost::math

#endif /* BIFROST_MATH_HPP */
