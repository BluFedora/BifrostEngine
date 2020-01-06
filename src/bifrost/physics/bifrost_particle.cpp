#include "particle.h"

#include "prismtypes.h"

namespace prism
{
    const Vec3 &Particle::getPosition() const
    {
        return this->m_Position;
    }

    void Particle::setPosition(const Vec3 &pos)
    {
        this->m_Position.set(pos);
    }

    Vec3 Particle::getVelocity() const
    {
        return this->m_Velocity;
    }

    void Particle::setVelocity(const Vec3 &vel)
    {
        this->m_Velocity.set(vel);
    }

    const Vec3 &Particle::acceleration() const
    {
        return this->m_Acceleration;
    }

    bool Particle::hasFiniteMass() const
    {
        return (this->m_InvMass != 0.0);
    }

    real Particle::mass() const
    {
        return (this->m_InvMass == 0.0) ? 0.0 : (1.0 / this->m_InvMass);
    }

    real Particle::inverseMass() const
    {
        return this->m_InvMass;
    }

    void Particle::addForce(const Vec3 &force)
    {
        this->m_TotalForce += force;
    }

    void Particle::clearAccumlulator()
    {
        this->m_TotalForce.set();
    }

    void Particle::integrate(const real duration)
    {
        PRISM_ASSERT(duration > 0.0f, "Particle::integrate parameter 'duration' is less than 0.0");

        this->m_Position.addScaledVector(this->m_Velocity, duration);

        auto resultingAcc(this->m_Acceleration);
        resultingAcc.addScaledVector(this->m_TotalForce, this->m_InvMass);

        this->m_Velocity.addScaledVector(resultingAcc, duration);

        this->m_Velocity *= pow_real(this->m_Damping, this->m_Damping);

        this->clearAccumlulator();
    }
}