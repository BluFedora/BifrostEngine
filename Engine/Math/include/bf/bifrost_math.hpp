//
// C++ Math Utilities
//
// References:
//   "What Every Computer Scientist Should Know About Floating-Point Arithmetic"
//      [https://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html]
//
#ifndef BF_MATH_HPP
#define BF_MATH_HPP

#include "bifrost_math.h"
#include "math/bifrost_rect2.hpp"

#include <cmath> /* std::fma */

namespace bf
{
  static constexpr float    k_PI       = 3.14159265358979323846f;
  static constexpr float    k_TwoPI    = k_PI * 2.0f;
  static constexpr float    k_Tau      = k_TwoPI;
  static constexpr float    k_HalfPI   = k_PI * 0.5f;
  static constexpr float    k_RadToDeg = 180.0f / k_PI;
  static constexpr float    k_DegToRad = k_PI / 180.0f;
  static constexpr Vector3f k_XAxis3f  = {1.0f, 0.0f, 0.0f, 0.0f};
  static constexpr Vector3f k_YAxis3f  = {0.0f, 1.0f, 0.0f, 0.0f};
  static constexpr Vector3f k_ZAxis3f  = {0.0f, 0.0f, 1.0f, 0.0f};
}  // namespace bf

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

  // Optimized for going into the 0 - 1 range.
  template<typename T>
  T mapToRange01(T min, T value, T max)
  {
    return ((value - min) / (max - min));
  }

  template<typename T>
  T mapToRange(T min, T value, T max, T new_min, T new_max)
  {
    return mapToRange01(min, value, max) * (new_max - new_min) + new_min;
  }

  template<typename T>
  T clamp(T min, T value, T max)
  {
    return std::min(std::max(min, value), max);
  }

  // Adapted From: [https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion]
  inline static Vector3f rotateVectorByQuat(const Quaternionf& quat, const Vector3f& vector)
  {
    const Vector3f vec_part    = {quat.x, quat.y, quat.z, 0.0f};
    const float    scalar_part = quat.w;

    return 2.0f * vec::dot(vec_part, vector) * vec_part +
           (scalar_part * scalar_part - vec::dot(vec_part, vec_part)) * vector +
           2.0f * scalar_part * vec::cross(vec_part, vector);
  }

  inline static Vector3f sphericalToCartesian(float radius, float theta, float phi)
  {
    const float cos_theta = std::cos(theta);
    const float sin_theta = std::sin(theta);
    const float cos_phi   = std::cos(phi);
    const float sin_phi   = std::sin(phi);

    const float x = radius * cos_theta * sin_phi;
    const float y = radius * sin_theta * sin_phi;
    const float z = radius * cos_phi;

    return Vector3f(x, y, z);
  }

  // Function Aliases from the C-API

  inline constexpr const auto& alignf   = &bfMathAlignf;
  inline constexpr const auto& invLerpf = &bfMathInvLerpf;
  inline constexpr const auto& remapf   = &bfMathRemapf;
}  // namespace bf::math

#endif /* BF_MATH_HPP */
