#ifndef PARTICLECONTACTS_H
#define PARTICLECONTACTS_H

#include "../math/vector3.h"

namespace prism
{
  class ParticleContact
  {
    friend class ParticleContactResolver;

   public:
    Particle* particle[2]         = {nullptr, nullptr};
    real      resitution          = 0.0;
    Vec3      contactNormal       = Vec3{0.0, 0.0, 0.0};
    real      penetration         = 0.0;
    Vec3      particleMovement[2] = {Vec3{0.0, 0.0, 0.0}, Vec3{0.0, 0.0, 0.0}};

   public:
    ParticleContact() = default;

   protected:
    void resolve(real duration);
    real calculateSeparatingVelocity() const;

   private:
    void resolveVelocity(real duration);
    void resolveInterpenetration(real duration);
  };

  class ParticleContactResolver
  {
   private:
    uint m_Iterations;
    uint m_IterationsUsed;

   public:
    ParticleContactResolver(uint iterations);

    void setIterations(uint iterations);
    void resolveContacts(ParticleContact* contactArray, uint numContacts, real duration);
  };
}  // namespace prism

#endif  // PARTICLECONTACTS_H