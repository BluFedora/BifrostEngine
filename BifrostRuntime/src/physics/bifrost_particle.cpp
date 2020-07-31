#include "bifrost_particle.h"

#include <cassert> /* assert */

namespace bf
{
  bool Particle::hasFiniteMass() const
  {
    return (this->m_InvMass != k_ScalarZero);
  }

  Scalar Particle::mass() const
  {
    return hasFiniteMass() ? k_ScalarOne / this->m_InvMass : k_ScalarZero;
  }

  Scalar Particle::inverseMass() const
  {
    return this->m_InvMass;
  }

  void Particle::addForce(const Vector3f &force)
  {
    this->m_TotalForce += force;
  }

  void Particle::clearAccumulator()
  {
    this->m_TotalForce = {0.0f};
  }

  void Particle::integrate(const Scalar duration)
  {
    assert(duration > 0.0f && !"Particle::integrate parameter 'duration' is less than 0.0");

    Vec3f_addScaled(&m_Position, &m_Velocity, duration);

    Vector3f resulting_acc = m_Acceleration;

    Vec3f_addScaled(&resulting_acc, &m_TotalForce, m_InvMass);
    Vec3f_addScaled(&m_Velocity, &resulting_acc, duration);

    m_Velocity *= pow_real(m_Damping, m_Damping);

    clearAccumulator();
  }
}  // namespace bifrost