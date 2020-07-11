#ifndef PARTICLE_H
#define PARTICLE_H

#include "bifrost/bifrost_math.hpp" /* Vector3f */
#include "bifrost_prismtypes.h"     /* Scalar   */

using namespace prism;

namespace bifrost
{
  class Particle final
  {
   public:  // TODO(SR): Rename these to reflect their public nature.
    Vector3f m_Position     = {0.0, 0.0, 0.0};
    Vector3f m_Velocity     = {0.0, 0.0, 0.0};
    Vector3f m_Acceleration = {0.0, 0.0, 0.0};
    Scalar   m_Damping      = Scalar(0.0);
    Scalar   m_InvMass      = Scalar(1.0);
    Vector3f m_TotalForce   = {0.0, 0.0, 0.0};

   public:
    Particle() = default;

    bool   hasFiniteMass() const;
    Scalar mass() const;
    Scalar inverseMass() const;
    void   addForce(const Vector3f &force);
    void   clearAccumulator();
    void   integrate(const Scalar duration);
  };
}  // namespace bifrost

#endif  // PARTICLE_H