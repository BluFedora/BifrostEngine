/******************************************************************************/
/*!
* @file   bifrost_rect2.hpp
* @author Shareef Abdoul-Raheem (http://blufedora.github.io/)
* @brief
*   Contains Utilities for 2D rectangle math.
*
* @version 0.0.1
* @date    2020-03-17
*
* @copyright Copyright (c) 2020
*/
/******************************************************************************/
#ifndef BIFROST_RECT2_HPP
#define BIFROST_RECT2_HPP

#include "bifrost_vec2.h" /* Vec2f, Vec2i */
#include "bifrost_vec3.h" /* Vec3f        */

#include <algorithm> /* min, max */
#include <cstdint>   /* uint32_t */

namespace bifrost
{
  namespace detail
  {
    template<typename T>
    struct Vec2T
    {
      T x;
      T y;
    };

    template<typename T>
    Vec2T<T> operator*(const Vec2T<T>& lhs, T rhs)
    {
      return Vec2T<T>{lhs.x * rhs, lhs.y * rhs};
    }

    template<typename T>
    Vec2T<T> operator*(T lhs, const Vec2T<T>& rhs)
    {
      return rhs * lhs;
    }

    template<typename T>
    Vec2T<T> operator+(const Vec2T<T>& lhs, const Vec2T<T>& rhs)
    {
      return Vec2T<T>{lhs.x + rhs.x, lhs.y + rhs.y};
    }

    template<typename T>
    Vec2T<T> operator-(const Vec2T<T>& lhs, const Vec2T<T>& rhs)
    {
      return Vec2T<T>{lhs.x - rhs.x, lhs.y - rhs.y};
    }

    template<typename T>
    bool operator-(const Vec2T<T>& lhs)
    {
      return Vec2T<T>{-lhs.x, -lhs.y};
    }

    template<typename T>
    bool operator==(const Vec2T<T>& lhs, const Vec2T<T>& rhs)
    {
      return lhs.x == rhs.x && lhs.y == rhs.y;
    }

    template<typename T>
    bool operator!=(const Vec2T<T>& lhs, const Vec2T<T>& rhs)
    {
      return lhs.x != rhs.x && lhs.y != rhs.y;
    }

    template<>
    struct Vec2T<float> : public Vec2f
    {
      Vec2T(float x, float y) :
        Vec2f{x, y}
      {
      }

      Vec2T(const Vec2f& rhs) :
        Vec2f{rhs}
      {
      }

      Vec2T() = default;
    };

    template<>
    struct Vec2T<int> : public Vec2i
    {};

    template<typename T>
    struct Vec3T
    {
      T x;
      T y;
      T z;
      T w;
    };

    template<typename T>
    Vec3T<T> operator*(const Vec3T<T>& lhs, const Vec3T<T>& rhs)
    {
      return Vec3T<T>{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
    }

    template<typename T>
    Vec3T<T> operator*(const Vec3T<T>& lhs, T rhs)
    {
      return Vec3T<T>{lhs.x * rhs, lhs.y * rhs, lhs.z * rhs};
    }

    template<typename T>
    Vec3T<T> operator*(T lhs, const Vec3T<T>& rhs)
    {
      return rhs * lhs;
    }

    template<typename T>
    Vec3T<T> operator+(const Vec3T<T>& lhs, const Vec3T<T>& rhs)
    {
      return Vec3T<T>{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
    }

    template<typename T>
    Vec3T<T> operator-(const Vec3T<T>& lhs, const Vec3T<T>& rhs)
    {
      return Vec3T<T>{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
    }

    template<typename T>
    Vec3T<T> operator-(const Vec3T<T>& lhs)
    {
      return Vec3T<T>{-lhs.x, -lhs.y, -lhs.z};
    }

    template<typename T>
    bool operator==(const Vec3T<T>& lhs, const Vec3T<T>& rhs)
    {
      return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
    }

    template<typename T>
    bool operator!=(const Vec3T<T>& lhs, const Vec3T<T>& rhs)
    {
      return lhs.x != rhs.x && lhs.y != rhs.y && lhs.y != rhs.z;
    }

    template<>
    struct Vec3T<float> : public Vec3f
    {
      constexpr Vec3T(float x, float y, float z, float w = 1.0f) :
        Vec3f{x, y, z, w}
      {
      }

      constexpr Vec3T(float xyzw) :
        Vec3f{xyzw, xyzw, xyzw, xyzw}
      {
      }

      constexpr Vec3T(const Vec3f& rhs) :
        Vec3f{rhs}
      {
      }

      // This leaves the base 'Vec3f' uninitialized for performance reasons.
      // thus it cannot be constexpr.
      Vec3T() = default;

      Vec3T& operator*=(float scalar)
      {
        *this = *this * scalar;
        return *this;
      }

      Vec3T& operator+=(const Vec3T& rhs)
      {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        this->w += rhs.w;

        return *this;
      }

      Vec3T& operator-=(const Vec3T& rhs)
      {
        this->x -= rhs.x;
        this->y -= rhs.y;
        this->z -= rhs.z;
        this->w -= rhs.w;

        return *this;
      }
    };

    template<typename T>
    class Rect2T
    {
      using VectorT = Vec2T<T>;

     private:
      VectorT m_Min;
      VectorT m_Max;

     public:
      Rect2T(T x = T(0), T y = T(0), T width = T(0), T height = T(0)) :
        m_Min{x, y},
        m_Max{x + width, y + height}
      {
      }

      // Min-Max Constructor
      Rect2T(const VectorT& min, const VectorT& max) :
        m_Min{std::min(min.x, max.x), std::min(min.y, max.y)},
        m_Max{std::max(min.x, max.x), std::max(min.y, max.y)}
      {
      }

      // Circle Based Conversion
      Rect2T(const VectorT& pos, T radius) :
        Rect2T{pos.x - radius, pos.y - radius, radius * T(2), radius * T(2)}
      {
      }

      [[nodiscard]] VectorT topLeft() const { return m_Min; }
      [[nodiscard]] VectorT topRight() const { return VectorT{m_Max.x, m_Min.y}; }
      [[nodiscard]] VectorT bottomRight() const { return m_Max; }
      [[nodiscard]] VectorT bottomLeft() const { return VectorT{m_Min.x, m_Max.y}; }
      [[nodiscard]] VectorT center() const { return VectorT{centerX(), centerY()}; }
      [[nodiscard]] T       left() const { return m_Min.x; }
      [[nodiscard]] T       right() const { return m_Max.x; }
      [[nodiscard]] T       top() const { return m_Min.y; }
      [[nodiscard]] T       bottom() const { return m_Max.y; }
      [[nodiscard]] T       width() const { return right() - left(); }
      [[nodiscard]] T       height() const { return bottom() - top(); }
      [[nodiscard]] T       centerX() const { return left() + width() / T(2.0); }
      [[nodiscard]] T       centerY() const { return top() + height() / T(2.0); }
      void                  setLeft(T value) { m_Min.x = value; }
      void                  setRight(T value) { m_Max.x = value; }
      void                  setTop(T value) { m_Min.y = value; }
      void                  setBottom(T value) { m_Max.y = value; }
      void                  setWidth(T value) { m_Max.x = m_Min.x + value; }
      void                  setHeight(T value) { m_Max.y = m_Min.y + value; }

      void setX(T value)
      {
        const T old_w = width();
        m_Min.x       = value;
        setWidth(old_w);
      }

      void setY(T value)
      {
        const T old_h = height();
        m_Min.y       = value;
        setHeight(old_h);
      }

      void setMiddleX(T value)
      {
        const T old_w = width();
        m_Min.x       = value - (old_w / 2);
        setWidth(old_w);
      }

      void setMiddleY(T value)
      {
        const T old_h = height();
        m_Min.y       = value - (old_h / 2);
        setHeight(old_h);
      }

      template<typename F>
      Rect2T<T> merge(const Rect2T<F>& rhs) const
      {
        const T l = std::min(left(), static_cast<T>(rhs.left()));
        const T r = std::max(right(), static_cast<T>(rhs.right()));
        const T t = std::min(top(), static_cast<T>(rhs.top()));
        const T b = std::max(bottom(), static_cast<T>(rhs.bottom()));

        return Rect2T<T>(l, t, r - l, b - t);
      }

      // Merges two rectangles with an AND operation
      template<typename F>
      Rect2T<T> mergeAND(const Rect2T<F>& rhs) const
      {
        const T l = std::max(left(), static_cast<T>(rhs.left()));
        const T r = std::min(right(), static_cast<T>(rhs.right()));
        const T t = std::max(top(), static_cast<T>(rhs.top()));
        const T b = std::min(bottom(), static_cast<T>(rhs.bottom()));

        return Rect2T<T>(l, t, r - l, b - t);
      }

      // Merges the rectangle with a Vec2
      // It could return a new rectangle with the merge, but you do not always want a copy
      // In the cases that you do, just copy before the merge operation
      void mergePoint(const VectorT& rhs)
      {
        setBottom(std::max(bottom(), rhs.y));
        setTop(std::min(top(), rhs.y));
        setRight(std::max(right(), rhs.x));
        setLeft(std::min(left(), rhs.x));
      }

      template<typename F>
      bool intersectsRect(const Rect2T<F>& rhs) const
      {
        return !(rhs.right() < left() || rhs.bottom() < top() || rhs.left() > right() || rhs.top() > bottom());
      }

      template<typename F>
      bool contains(const Rect2T<F>& rhs) const
      {
        return (left() <= rhs.left() && right() >= rhs.right() && top() <= rhs.top() && bottom() >= rhs.bottom());
      }

      template<typename F>
      bool canContain(const Rect2T<F>& rhs) const
      {
        return width() >= rhs.width() && height() >= rhs.height();
      }

      // NOTE(Shareef):
      //   Unlike 'contains' this returns false if the two rectangles are exactly alike
      template<typename F>
      bool encompasses(const Rect2T<F>& rhs) const
      {
        return (left() <= rhs.left() && right() >= rhs.right() && top() <= rhs.top() && bottom() >= rhs.bottom());
      }

      template<typename F>
      bool operator==(const Rect2T<F>& rhs) const
      {
        return m_Min == rhs.m_Min && m_Max == rhs.m_Max;
      }

      template<typename F>
      bool operator!=(const Rect2T<F>& rhs) const
      {
        return !(*this == rhs);
      }

      template<typename F>
      Rect2T<F> operator+(const Vec2T<F>& vector) const
      {
        return Rect2T<F>(m_Min + vector, m_Max + vector);
      }

      template<typename F>
      Rect2T<F> operator-(const Vec2T<F>& vector) const
      {
        return Rect2T<F>(m_Min - vector, m_Max - vector);
      }

      Rect2T operator-() const
      {
        return Rect2T{-m_Min, -m_Max};
      }

      template<typename F>
      bool intersects(const Vec2T<F>& point) const
      {
        return (left() <= point.x && point.x <= right()) && (top() <= point.y && point.y <= bottom());
      }

      [[nodiscard]] T area() const
      {
        return width() * height();
      }

      template<typename F>
      T distanceSqFromPoint(F point_x, F point_y) const
      {
        T cx = std::max(std::min(static_cast<T>(point_x), left() + width()), left());
        T cy = std::max(std::min(static_cast<T>(point_y), top() + height()), top());
        return ((static_cast<T>(point_x) - cx) * (static_cast<T>(point_x) - cx)) + ((static_cast<T>(point_y) - cy) * (static_cast<T>(point_y) - cy));
      }
    };

    template<typename T>
    Rect2T<T> operator+(const Rect2T<T>& lhs, const Rect2T<T>& rhs)
    {
      return Rect2T<T>{
       lhs.left() + rhs.left(),
       lhs.top() + rhs.top(),
       lhs.width() + rhs.width(),
       lhs.height() + rhs.height()};
    }

    template<typename T>
    Rect2T<T> operator-(const Rect2T<T>& lhs, const Rect2T<T>& rhs)
    {
      return Rect2T<T>{
       lhs.left() - rhs.left(),
       lhs.top() - rhs.top(),
       lhs.width() - rhs.width(),
       lhs.height() - rhs.height()};
    }

    template<typename T>
    Rect2T<T> operator*(const Rect2T<T>& lhs, T rhs)
    {
      return Rect2T<T>{lhs.topLeft() * rhs, lhs.bottomRight() * rhs};
    }

    template<typename T>
    Rect2T<T> operator*(T lhs, const Rect2T<T>& rhs)
    {
      return rhs * lhs;
    }
  }  // namespace detail

  using Rect2i   = detail::Rect2T<int>;
  using Rect2u   = detail::Rect2T<unsigned int>;
  using Rect2f   = detail::Rect2T<float>;
  using Vector2i = detail::Vec2T<int>;
  using Vector2u = detail::Vec2T<unsigned int>;
  using Vector2f = detail::Vec2T<float>;
  using Vector3i = detail::Vec3T<int>;
  using Vector3u = detail::Vec3T<unsigned int>;
  using Vector3f = detail::Vec3T<float>;

  // NOTE(Shareef):
  //   This namespace contains some utilities for manipulating rectangles.
  namespace rect
  {
    Rect2i aspectRatioDrawRegion(std::uint32_t aspect_w, std::uint32_t aspect_h, std::uint32_t window_w, std::uint32_t window_h);
  }

  namespace vec
  {
    static Vector3f cross(const Vector3f& a, const Vector3f& b, float w = 0.0f)
    {
      Vector3f result;
      Vec3f_cross(&a, &b, &result);
      result.w = w;

      return result;
    }

    template<typename T>
    detail::Vec3T<T> min(const detail::Vec3T<T>& a, const detail::Vec3T<T>& b, float w = 1.0f)
    {
      return {
       std::min(a.x, b.x),
       std::min(a.y, b.y),
       std::min(a.z, b.z),
       w,
      };
    }

    template<typename T>
    detail::Vec3T<T> max(const detail::Vec3T<T>& a, const detail::Vec3T<T>& b, float w = 1.0f)
    {
      return {
       std::max(a.x, b.x),
       std::max(a.y, b.y),
       std::max(a.z, b.z),
       w,
      };
    }
  }  // namespace vec

}  // namespace bifrost

#endif /* BIFROST_RECT2_HPP */
