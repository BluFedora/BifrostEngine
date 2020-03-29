//
// C++ Math Utilities
//
#ifndef BIFROST_MATH_HPP
#define BIFROST_MATH_HPP

#include "bifrost_math.h"
#include "math/bifrost_rect2.hpp"

#include <cmath> /* std::fma */

namespace bifrost::math
{
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
}  // namespace bifrost::math

#endif /* BIFROST_MATH_HPP */
