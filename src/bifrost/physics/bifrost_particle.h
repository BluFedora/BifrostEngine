#ifndef PARTICLE_H
#define PARTICLE_H

#include "../math/vector3.h"

namespace prism
{
  class Particle  // TODO(BluFedora):: Make "m_Position", "m_Velocity", and "m_Acceleration" private
  {
   public:
    Vec3 m_Position     = Vec3{0.0, 0.0, 0.0};
    Vec3 m_Velocity     = Vec3{0.0, 0.0, 0.0};
    Vec3 m_Acceleration = Vec3{0.0, 0.0, 0.0};

   private:
    real m_Damping    = 0.0;
    real m_InvMass    = 1.0;
    Vec3 m_TotalForce = Vec3{0.0, 0.0, 0.0};

   public:
    Particle() = default;

    const Vec3 &getPosition() const;
    void        setPosition(const Vec3 &pos);

    Vec3 getVelocity() const;
    void setVelocity(const Vec3 &vel);

    const Vec3 &acceleration() const;

    bool hasFiniteMass() const;
    real mass() const;
    real inverseMass() const;

    void addForce(const Vec3 &force);
    void clearAccumlulator();
    void integrate(const real duration);
  };
}  // namespace prism

#endif  // PARTICLE_H