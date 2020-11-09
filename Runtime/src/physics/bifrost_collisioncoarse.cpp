#include "bifrost_collisioncoarse.h"

namespace prism
{
  // BoundingSphere

  BoundingSphere::BoundingSphere(const Vec3 &center, real radius) :
    m_Center(center),
    m_Radius(radius)
  {
  }

  BoundingSphere::BoundingSphere(const BoundingSphere &one, const BoundingSphere &two)
  {
    Vec3 centerOffset = (one.m_Center - two.m_Center);
    real distance     = centerOffset.lengthSq();
    real radiusDiff   = two.m_Radius - one.m_Radius;

    // Check if the larger sphere encloses the small one
    if (radiusDiff * radiusDiff >= distance)
    {
      if (one.m_Radius > two.m_Radius)
      {
        this->m_Center = one.m_Center;
        this->m_Radius = one.m_Radius;
      }
      else
      {
        this->m_Center = two.m_Center;
        this->m_Radius = two.m_Radius;
      }
    }
    else  // Otherwise we need to work with partially overlapping spheres
    {
      distance       = sqrt_real(distance);
      this->m_Radius = (distance + one.m_Radius + two.m_Radius) * ((real)0.5);

      // The new centre is based on one's centre, moved towards
      // two's centre by an ammount proportional to the spheres'
      // radii.
      this->m_Center = one.m_Center;

      if (distance > 0)
      {
        this->m_Center += centerOffset * ((m_Radius - one.m_Radius) / distance);
      }
    }
  }

  bool BoundingSphere::overlaps(const BoundingSphere *other) const
  {
    real distanceSquared = (m_Center - other->m_Center).lengthSq();
    return distanceSquared < (m_Radius + other->m_Radius) * (m_Radius + other->m_Radius);
  }

  real BoundingSphere::getGrowth(const BoundingSphere &other) const
  {
    BoundingSphere newSphere(*this, other);
    return newSphere.m_Radius * newSphere.m_Radius - this->m_Radius * this->m_Radius;
  }

  real BoundingSphere::getSize() const
  {
#if PRISM_PHYSICS_PRECISION == PRISM_PHYSICS_DBL
    return ((real)1.333333) * Calc::PI_d * m_Radius * m_Radius * m_Radius;
#elif PRISM_PHYSICS_PRECISION == PRISM_PHYSICS_FLT
    return ((real)1.333333) * Calc::PI_f * radius * radius * radius;
#endif
  }

  // PotentialContact

  PotentialContact::PotentialContact(RigidBody *one, RigidBody *two) :
    body{one, two}
  {
  }

  PotentialContact::~PotentialContact()
  {
    this->body[0] = nullptr;
    this->body[1] = nullptr;
  }
}  // namespace prism