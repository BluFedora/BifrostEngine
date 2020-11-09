#ifndef PRISIMTYPES_H
#define PRISIMTYPES_H

#include "bifrost/bifrost_math.h"

#include "bifrost/bifrost_std.h" /* bfFloat32 */

#include <cmath>
#include <float.h>

#include "bifrost/math/bifrost_rect2.hpp"

#define PRISM_PHYSICS_DBL 0
#define PRISM_PHYSICS_FLT 1

namespace bf
{
#ifdef PRISM_USE_DOUBLE
#define PRISM_PHYSICS_PRECISION PRISM_PHYSICS_DBL
  using Scalar = bfFloat64;
#else
#define PRISM_PHYSICS_PRECISION PRISM_PHYSICS_FLT
  using Scalar = bfFloat32;
#endif
}  // namespace bifrost

#if PRISM_PHYSICS_PRECISION == PRISM_PHYSICS_DBL
#define max_real DBL_MAX
#define pow_real pow
#define abs_real fabs
#define sqrt_real sqrt
#define epsilon_real DBL_EPSILON
#elif PRISM_PHYSICS_PRECISION == PRISM_PHYSICS_FLT
#define max_real FLT_MAX
#define pow_real powf
#define abs_real fabsf
#define sqrt_real sqrtf
#define epsilon_real FLT_EPSILON
#endif

using namespace bf;

namespace prism
{
  static constexpr Scalar k_ScalarZero = Scalar(0.0);
  static constexpr Scalar k_ScalarOne  = Scalar(1.0);

  using Vec3 = Vector3f;

  class BoundingSphere;
  class PotentialContact;
  template<class BoundingVolumeClass>
  class BVHNode;

  class RigidBody;
  class Contact;
  class CollisionDetector;

  class Mat4x3
  {
   public:
    /**
             * Holds the transform matrix data in array form.
             */
    Scalar data[12];

    /**
             * Creates an identity matrix.
             */
    Mat4x3() :
      data{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0}
    {
    }

    /**
             * Sets the matrix to be a diagonal matrix with the given coefficients.
             */
    void setDiagonal(Scalar a, Scalar b, Scalar c)
    {
      data[0]  = a;
      data[5]  = b;
      data[10] = c;
    }

    /**
             * Returns a matrix which is this matrix multiplied by the given
             * other matrix.
             */
    Mat4x3 operator*(const Mat4x3 &o) const
    {
      Mat4x3 result;
      result.data[0] = (o.data[0] * data[0]) + (o.data[4] * data[1]) + (o.data[8] * data[2]);
      result.data[4] = (o.data[0] * data[4]) + (o.data[4] * data[5]) + (o.data[8] * data[6]);
      result.data[8] = (o.data[0] * data[8]) + (o.data[4] * data[9]) + (o.data[8] * data[10]);

      result.data[1] = (o.data[1] * data[0]) + (o.data[5] * data[1]) + (o.data[9] * data[2]);
      result.data[5] = (o.data[1] * data[4]) + (o.data[5] * data[5]) + (o.data[9] * data[6]);
      result.data[9] = (o.data[1] * data[8]) + (o.data[5] * data[9]) + (o.data[9] * data[10]);

      result.data[2]  = (o.data[2] * data[0]) + (o.data[6] * data[1]) + (o.data[10] * data[2]);
      result.data[6]  = (o.data[2] * data[4]) + (o.data[6] * data[5]) + (o.data[10] * data[6]);
      result.data[10] = (o.data[2] * data[8]) + (o.data[6] * data[9]) + (o.data[10] * data[10]);

      result.data[3]  = (o.data[3] * data[0]) + (o.data[7] * data[1]) + (o.data[11] * data[2]) + data[3];
      result.data[7]  = (o.data[3] * data[4]) + (o.data[7] * data[5]) + (o.data[11] * data[6]) + data[7];
      result.data[11] = (o.data[3] * data[8]) + (o.data[7] * data[9]) + (o.data[11] * data[10]) + data[11];

      return result;
    }

    /**
             * Transform the given vector by this matrix.
             *
             * @param vector The vector to transform.
             */
    Vec3 operator*(const Vec3 &vector) const
    {
      return Vec3(
       vector.x * data[0] +
        vector.y * data[1] +
        vector.z * data[2] + data[3],

       vector.x * data[4] +
        vector.y * data[5] +
        vector.z * data[6] + data[7],

       vector.x * data[8] +
        vector.y * data[9] +
        vector.z * data[10] + data[11]);
    }

    /**
             * Transform the given vector by this matrix.
             *
             * @param vector The vector to transform.
             */
    Vec3 transform(const Vec3 &vector) const
    {
      return (*this) * vector;
    }

    /**
             * Returns the determinant of the matrix.
             */
    Scalar getDeterminant() const
    {
      return -data[8] * data[5] * data[2] +
             data[4] * data[9] * data[2] +
             data[8] * data[1] * data[6] -
             data[0] * data[9] * data[6] -
             data[4] * data[1] * data[10] +
             data[0] * data[5] * data[10];
    }

    /**
             * Sets the matrix to be the inverse of the given matrix.
             *
             * @param m The matrix to invert and use to set this.
             */
    void setInverse(const Mat4x3 &m)
    {
      // Make sure the determinant is non-zero.
      Scalar det = getDeterminant();
      if (det == 0) return;
      det = k_ScalarOne / det;

      data[0] = (-m.data[9] * m.data[6] + m.data[5] * m.data[10]) * det;
      data[4] = (m.data[8] * m.data[6] - m.data[4] * m.data[10]) * det;
      data[8] = (-m.data[8] * m.data[5] + m.data[4] * m.data[9]) * det;

      data[1] = (m.data[9] * m.data[2] - m.data[1] * m.data[10]) * det;
      data[5] = (-m.data[8] * m.data[2] + m.data[0] * m.data[10]) * det;
      data[9] = (m.data[8] * m.data[1] - m.data[0] * m.data[9]) * det;

      data[2]  = (-m.data[5] * m.data[2] + m.data[1] * m.data[6]) * det;
      data[6]  = (+m.data[4] * m.data[2] - m.data[0] * m.data[6]) * det;
      data[10] = (-m.data[4] * m.data[1] + m.data[0] * m.data[5]) * det;

      data[3]  = (m.data[9] * m.data[6] * m.data[3] - m.data[5] * m.data[10] * m.data[3] - m.data[9] * m.data[2] * m.data[7] + m.data[1] * m.data[10] * m.data[7] + m.data[5] * m.data[2] * m.data[11] - m.data[1] * m.data[6] * m.data[11]) * det;
      data[7]  = (-m.data[8] * m.data[6] * m.data[3] + m.data[4] * m.data[10] * m.data[3] + m.data[8] * m.data[2] * m.data[7] - m.data[0] * m.data[10] * m.data[7] - m.data[4] * m.data[2] * m.data[11] + m.data[0] * m.data[6] * m.data[11]) * det;
      data[11] = (m.data[8] * m.data[5] * m.data[3] - m.data[4] * m.data[9] * m.data[3] - m.data[8] * m.data[1] * m.data[7] + m.data[0] * m.data[9] * m.data[7] + m.data[4] * m.data[1] * m.data[11] - m.data[0] * m.data[5] * m.data[11]) * det;
    }

    /** Returns a new matrix containing the inverse of this matrix. */
    Mat4x3 inverse() const
    {
      Mat4x3 result;
      result.setInverse(*this);
      return result;
    }

    /**
             * Inverts the matrix.
             */
    void invert()
    {
      setInverse(*this);
    }

    /**
             * Transform the given direction vector by this matrix.
             *
             * @note When a direction is converted between frames of
             * reference, there is no translation required.
             *
             * @param vector The vector to transform.
             */
    Vec3 transformDirection(const Vec3 &vector) const
    {
      return Vec3(
       vector.x * data[0] +
        vector.y * data[1] +
        vector.z * data[2],

       vector.x * data[4] +
        vector.y * data[5] +
        vector.z * data[6],

       vector.x * data[8] +
        vector.y * data[9] +
        vector.z * data[10]);
    }

    /**
             * Transform the given direction vector by the
             * transformational inverse of this matrix.
             *
             * @note This function relies on the fact that the inverse of
             * a pure rotation matrix is its transpose. It separates the
             * translational and rotation components, transposes the
             * rotation, and multiplies out. If the matrix is not a
             * scale and shear free transform matrix, then this function
             * will not give correct results.
             *
             * @note When a direction is converted between frames of
             * reference, there is no translation required.
             *
             * @param vector The vector to transform.
             */
    Vec3 transformInverseDirection(const Vec3 &vector) const
    {
      return Vec3(
       vector.x * data[0] +
        vector.y * data[4] +
        vector.z * data[8],

       vector.x * data[1] +
        vector.y * data[5] +
        vector.z * data[9],

       vector.x * data[2] +
        vector.y * data[6] +
        vector.z * data[10]);
    }

    /**
             * Transform the given vector by the transformational inverse
             * of this matrix.
             *
             * @note This function relies on the fact that the inverse of
             * a pure rotation matrix is its transpose. It separates the
             * translational and rotation components, transposes the
             * rotation, and multiplies out. If the matrix is not a
             * scale and shear free transform matrix, then this function
             * will not give correct results.
             *
             * @param vector The vector to transform.
             */
    Vec3 transformInverse(const Vec3 &vector) const
    {
      Vec3 tmp = vector;
      tmp.x -= data[3];
      tmp.y -= data[7];
      tmp.z -= data[11];
      return Vec3(
       tmp.x * data[0] +
        tmp.y * data[4] +
        tmp.z * data[8],

       tmp.x * data[1] +
        tmp.y * data[5] +
        tmp.z * data[9],

       tmp.x * data[2] +
        tmp.y * data[6] +
        tmp.z * data[10]);
    }

    /**
     * Gets a vector representing one axis (i.e. one column) in the matrix.
     *
     * @param i The row to return. Row 3 corresponds to the position
     * of the transform matrix.
     *
     * @return The vector.
     */
    Vec3 getAxisVector(int i) const
    {
      return Vec3(data[i], data[i + 4], data[i + 8]);
    }

    /**
     * Sets this matrix to be the rotation matrix corresponding to
     * the given quaternion.
     */
    void setOrientationAndPos(const Quaternionf &q, const Vec3 &pos)
    {
      data[0] = 1 - (2 * q.j * q.j + 2 * q.k * q.k);
      data[1] = 2 * q.i * q.j + 2 * q.k * q.r;
      data[2] = 2 * q.i * q.k - 2 * q.j * q.r;
      data[3] = pos.x;

      data[4] = 2 * q.i * q.j - 2 * q.k * q.r;
      data[5] = 1 - (2 * q.i * q.i + 2 * q.k * q.k);
      data[6] = 2 * q.j * q.k + 2 * q.i * q.r;
      data[7] = pos.y;

      data[8]  = 2 * q.i * q.k + 2 * q.j * q.r;
      data[9]  = 2 * q.j * q.k - 2 * q.i * q.r;
      data[10] = 1 - (2 * q.i * q.i + 2 * q.j * q.j);
      data[11] = pos.z;
    }

    /**
             * Fills the given array with this transform matrix, so it is
             * usable as an open-gl transform matrix. OpenGL uses a column
             * major format, so that the values are transposed as they are
             * written.
             */
    void fillGLArray(float array[16]) const
    {
      array[0] = static_cast<float>(data[0]);
      array[1] = static_cast<float>(data[4]);
      array[2] = static_cast<float>(data[8]);
      array[3] = static_cast<float>(0.0f);

      array[4] = static_cast<float>(data[1]);
      array[5] = static_cast<float>(data[5]);
      array[6] = static_cast<float>(data[9]);
      array[7] = static_cast<float>(0.0f);

      array[8]  = static_cast<float>(data[2]);
      array[9]  = static_cast<float>(data[6]);
      array[10] = static_cast<float>(data[10]);
      array[11] = static_cast<float>(0.0f);

      array[12] = static_cast<float>(data[3]);
      array[13] = static_cast<float>(data[7]);
      array[14] = static_cast<float>(data[11]);
      array[15] = static_cast<float>(1.0f);
    }
  };

  /**
         * Holds an inertia tensor, consisting of a 3x3 row-major matrix.
         * This matrix is not padding to produce an aligned structure, since
         * it is most commonly used with a mass (single Scalar) and two
         * damping coefficients to make the 12-element characteristics array
         * of a rigid body.
         */
  class Mat3x3
  {
   public:
    /**
             * Holds the tensor matrix data in array form.
             */
    Scalar data[9];

    // ... Other Matrix3 code as before ...

    /**
             * Creates a new matrix.
             */
    Mat3x3()
    {
      data[0] = data[1] = data[2] = data[3] = data[4] = data[5] =
       data[6] = data[7] = data[8] = 0;
    }

    /**
             * Creates a new matrix with the given three vectors making
             * up its columns.
             */
    Mat3x3(const Vec3 &compOne, const Vec3 &compTwo, const Vec3 &compThree)
    {
      setComponents(compOne, compTwo, compThree);
    }

    /**
             * Creates a new matrix with explicit coefficients.
             */
    Mat3x3(Scalar c0, Scalar c1, Scalar c2, Scalar c3, Scalar c4, Scalar c5, Scalar c6, Scalar c7, Scalar c8)
    {
      data[0] = c0;
      data[1] = c1;
      data[2] = c2;
      data[3] = c3;
      data[4] = c4;
      data[5] = c5;
      data[6] = c6;
      data[7] = c7;
      data[8] = c8;
    }

    /**
             * Sets the matrix to be a diagonal matrix with the given
             * values along the leading diagonal.
             */
    void setDiagonal(Scalar a, Scalar b, Scalar c)
    {
      setInertiaTensorCoeffs(a, b, c);
    }

    /**
             * Sets the value of the matrix from inertia tensor values.
             */
    void setInertiaTensorCoeffs(Scalar ix, Scalar iy, Scalar iz, Scalar ixy = 0, Scalar ixz = 0, Scalar iyz = 0)
    {
      data[0] = ix;
      data[1] = data[3] = -ixy;
      data[2] = data[6] = -ixz;
      data[4]           = iy;
      data[5] = data[7] = -iyz;
      data[8]           = iz;
    }

    /**
             * Sets the value of the matrix as an inertia tensor of
             * a rectangular block aligned with the body's coordinate
             * system with the given axis half-sizes and mass.
             */
    void setBlockInertiaTensor(const Vec3 &halfSizes, Scalar mass)
    {
      const Vec3 squares = halfSizes * halfSizes;

      setInertiaTensorCoeffs(0.3f * mass * (squares.y + squares.z),
                             0.3f * mass * (squares.x + squares.z),
                             0.3f * mass * (squares.x + squares.y));
    }

    /**
             * Sets the matrix to be a skew symmetric matrix based on
             * the given vector. The skew symmetric matrix is the equivalent
             * of the vector product. So if a,b are vectors. a x b = A_s b
             * where A_s is the skew symmetric form of a.
             */
    void setSkewSymmetric(const Vec3 &vector)
    {
      data[0] = data[4] = data[8] = 0;
      data[1]                     = -vector.z;
      data[2]                     = vector.y;
      data[3]                     = vector.z;
      data[5]                     = -vector.x;
      data[6]                     = -vector.y;
      data[7]                     = vector.x;
    }

    /**
             * Sets the matrix values from the given three vector components.
             * These are arranged as the three columns of the vector.
             */
    void setComponents(const Vec3 &compOne, const Vec3 &compTwo, const Vec3 &compThree)
    {
      data[0] = compOne.x;
      data[1] = compTwo.x;
      data[2] = compThree.x;
      data[3] = compOne.y;
      data[4] = compTwo.y;
      data[5] = compThree.y;
      data[6] = compOne.z;
      data[7] = compTwo.z;
      data[8] = compThree.z;
    }

    /**
             * Transform the given vector by this matrix.
             *
             * @param vector The vector to transform.
             */
    Vec3 operator*(const Vec3 &vector) const
    {
      return Vec3(
       vector.x * data[0] + vector.y * data[1] + vector.z * data[2],
       vector.x * data[3] + vector.y * data[4] + vector.z * data[5],
       vector.x * data[6] + vector.y * data[7] + vector.z * data[8]);
    }

    /**
             * Transform the given vector by this matrix.
             *
             * @param vector The vector to transform.
             */
    Vec3 transform(const Vec3 &vector) const
    {
      return (*this) * vector;
    }

    /**
             * Transform the given vector by the transpose of this matrix.
             *
             * @param vector The vector to transform.
             */
    Vec3 transformTranspose(const Vec3 &vector) const
    {
      return Vec3(
       vector.x * data[0] + vector.y * data[3] + vector.z * data[6],
       vector.x * data[1] + vector.y * data[4] + vector.z * data[7],
       vector.x * data[2] + vector.y * data[5] + vector.z * data[8]);
    }

    /**
             * Gets a vector representing one row in the matrix.
             *
             * @param i The row to return.
             */
    Vec3 getRowVector(int i) const
    {
      return Vec3(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }

    /**
             * Gets a vector representing one axis (i.e. one column) in the matrix.
             *
             * @param i The row to return.
             *
             * @return The vector.
             */
    Vec3 getAxisVector(int i) const
    {
      return Vec3(data[i], data[i + 3], data[i + 6]);
    }

    /**
             * Sets the matrix to be the inverse of the given matrix.
             *
             * @param m The matrix to invert and use to set this.
             */
    void setInverse(const Mat3x3 &m)
    {
      Scalar t4  = m.data[0] * m.data[4];
      Scalar t6  = m.data[0] * m.data[5];
      Scalar t8  = m.data[1] * m.data[3];
      Scalar t10 = m.data[2] * m.data[3];
      Scalar t12 = m.data[1] * m.data[6];
      Scalar t14 = m.data[2] * m.data[6];

      // Calculate the determinant
      Scalar t16 = (t4 * m.data[8] - t6 * m.data[7] - t8 * m.data[8] +
                    t10 * m.data[7] + t12 * m.data[5] - t14 * m.data[4]);

      // Make sure the determinant is non-zero.
      if (t16 == (Scalar)0.0f) return;
      Scalar t17 = 1 / t16;

      data[0] = (m.data[4] * m.data[8] - m.data[5] * m.data[7]) * t17;
      data[1] = -(m.data[1] * m.data[8] - m.data[2] * m.data[7]) * t17;
      data[2] = (m.data[1] * m.data[5] - m.data[2] * m.data[4]) * t17;
      data[3] = -(m.data[3] * m.data[8] - m.data[5] * m.data[6]) * t17;
      data[4] = (m.data[0] * m.data[8] - t14) * t17;
      data[5] = -(t6 - t10) * t17;
      data[6] = (m.data[3] * m.data[7] - m.data[4] * m.data[6]) * t17;
      data[7] = -(m.data[0] * m.data[7] - t12) * t17;
      data[8] = (t4 - t8) * t17;
    }

    /** Returns a new matrix containing the inverse of this matrix. */
    Mat3x3 inverse() const
    {
      Mat3x3 result;
      result.setInverse(*this);
      return result;
    }

    /**
             * Inverts the matrix.
             */
    void invert()
    {
      setInverse(*this);
    }

    /**
             * Sets the matrix to be the transpose of the given matrix.
             *
             * @param m The matrix to transpose and use to set this.
             */
    void setTranspose(const Mat3x3 &m)
    {
      data[0] = m.data[0];
      data[1] = m.data[3];
      data[2] = m.data[6];
      data[3] = m.data[1];
      data[4] = m.data[4];
      data[5] = m.data[7];
      data[6] = m.data[2];
      data[7] = m.data[5];
      data[8] = m.data[8];
    }

    /** Returns a new matrix containing the transpose of this matrix. */
    Mat3x3 transpose() const
    {
      Mat3x3 result;
      result.setTranspose(*this);
      return result;
    }

    /**
             * Returns a matrix which is this matrix multiplied by the given
             * other matrix.
             */
    Mat3x3 operator*(const Mat3x3 &o) const
    {
      return Mat3x3(
       data[0] * o.data[0] + data[1] * o.data[3] + data[2] * o.data[6],
       data[0] * o.data[1] + data[1] * o.data[4] + data[2] * o.data[7],
       data[0] * o.data[2] + data[1] * o.data[5] + data[2] * o.data[8],

       data[3] * o.data[0] + data[4] * o.data[3] + data[5] * o.data[6],
       data[3] * o.data[1] + data[4] * o.data[4] + data[5] * o.data[7],
       data[3] * o.data[2] + data[4] * o.data[5] + data[5] * o.data[8],

       data[6] * o.data[0] + data[7] * o.data[3] + data[8] * o.data[6],
       data[6] * o.data[1] + data[7] * o.data[4] + data[8] * o.data[7],
       data[6] * o.data[2] + data[7] * o.data[5] + data[8] * o.data[8]);
    }

    /**
             * Multiplies this matrix in place by the given other matrix.
             */
    void operator*=(const Mat3x3 &o)
    {
      Scalar t1;
      Scalar t2;
      Scalar t3;

      t1      = data[0] * o.data[0] + data[1] * o.data[3] + data[2] * o.data[6];
      t2      = data[0] * o.data[1] + data[1] * o.data[4] + data[2] * o.data[7];
      t3      = data[0] * o.data[2] + data[1] * o.data[5] + data[2] * o.data[8];
      data[0] = t1;
      data[1] = t2;
      data[2] = t3;

      t1      = data[3] * o.data[0] + data[4] * o.data[3] + data[5] * o.data[6];
      t2      = data[3] * o.data[1] + data[4] * o.data[4] + data[5] * o.data[7];
      t3      = data[3] * o.data[2] + data[4] * o.data[5] + data[5] * o.data[8];
      data[3] = t1;
      data[4] = t2;
      data[5] = t3;

      t1      = data[6] * o.data[0] + data[7] * o.data[3] + data[8] * o.data[6];
      t2      = data[6] * o.data[1] + data[7] * o.data[4] + data[8] * o.data[7];
      t3      = data[6] * o.data[2] + data[7] * o.data[5] + data[8] * o.data[8];
      data[6] = t1;
      data[7] = t2;
      data[8] = t3;
    }

    /**
             * Multiplies this matrix in place by the given scalar.
             */
    void operator*=(const Scalar scalar)
    {
      data[0] *= scalar;
      data[1] *= scalar;
      data[2] *= scalar;
      data[3] *= scalar;
      data[4] *= scalar;
      data[5] *= scalar;
      data[6] *= scalar;
      data[7] *= scalar;
      data[8] *= scalar;
    }

    /**
             * Does a component-wise addition of this matrix and the given
             * matrix.
             */
    void operator+=(const Mat3x3 &o)
    {
      data[0] += o.data[0];
      data[1] += o.data[1];
      data[2] += o.data[2];
      data[3] += o.data[3];
      data[4] += o.data[4];
      data[5] += o.data[5];
      data[6] += o.data[6];
      data[7] += o.data[7];
      data[8] += o.data[8];
    }

    /**
             * Sets this matrix to be the rotation matrix corresponding to
             * the given quaternion.
             */
    void setOrientation(const Quaternionf &q)
    {
      data[0] = 1 - (2 * q.j * q.j + 2 * q.k * q.k);
      data[1] = 2 * q.i * q.j + 2 * q.k * q.r;
      data[2] = 2 * q.i * q.k - 2 * q.j * q.r;
      data[3] = 2 * q.i * q.j - 2 * q.k * q.r;
      data[4] = 1 - (2 * q.i * q.i + 2 * q.k * q.k);
      data[5] = 2 * q.j * q.k + 2 * q.i * q.r;
      data[6] = 2 * q.i * q.k + 2 * q.j * q.r;
      data[7] = 2 * q.j * q.k - 2 * q.i * q.r;
      data[8] = 1 - (2 * q.i * q.i + 2 * q.j * q.j);
    }

    /**
             * Interpolates a couple of matrices.
             */
    static Mat3x3 linearInterpolate(const Mat3x3 &a, const Mat3x3 &b, Scalar prop)
    {
      Mat3x3 result;
      for (unsigned i = 0; i < 9; i++)
      {
        result.data[i] = a.data[i] * (1 - prop) + b.data[i] * prop;
      }
      return result;
    }
  };
}  // namespace prism

#endif
