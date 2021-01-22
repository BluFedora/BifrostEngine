#include "particlecontacts.h"

#include "particle.h"
#include "prismtypes.h"

namespace prism
{
    // ParticleContact

    void ParticleContact::resolve(real duration)
    {
        this->resolveVelocity(duration);
        this->resolveInterpenetration(duration);
    }

    real ParticleContact::calculateSeparatingVelocity() const
    {
        Vec3 relativeVelocity(this->particle[0]->m_Velocity.clone());

        if (this->particle[1]) {
            relativeVelocity -= particle[1]->m_Velocity;
        }

        return relativeVelocity.scalarProduct(this->contactNormal);
    }

    void ParticleContact::resolveVelocity(real duration)
    {
        real separatingVelocity = this->calculateSeparatingVelocity();

        if (separatingVelocity > 0) return;

        real newSepVel = -separatingVelocity * this->resitution;

        auto accCausedVel = this->particle[0]->acceleration();
        if (this->particle[1]) accCausedVel -= this->particle[1]->acceleration();
        real accCausedSepVel = accCausedVel.scalarProduct(this->contactNormal * duration);

        if (accCausedSepVel < 0)
        {
            newSepVel += this->resitution * accCausedSepVel;

            if (newSepVel < 0) {
                newSepVel = 0;
            }
        }

        real deltaVel = newSepVel - separatingVelocity;

        real totalInverseMass = particle[0]->inverseMass();

        if (this->particle[1]) {
            totalInverseMass += this->particle[1]->inverseMass();
        }

        if (totalInverseMass <= 0) return;

        real impulse = deltaVel / totalInverseMass;
        Vec3 inpulsePerMass = this->contactNormal * impulse;

        particle[0]->m_Velocity.set(particle[0]->m_Velocity +
                inpulsePerMass * this->particle[0]->inverseMass()
                );

        if (this->particle[1])
        {
            particle[1]->m_Velocity.set(particle[1]->m_Velocity +
                    inpulsePerMass * -this->particle[1]->inverseMass()
                    );
        }
    }

    void ParticleContact::resolveInterpenetration(real duration)
    {
        (void) duration;

        if (this->penetration <= 0) return;

        real totalInverseMass = particle[0]->inverseMass();

        if (this->particle[1]) {
            totalInverseMass += this->particle[1]->inverseMass();
        }

        if (totalInverseMass <= 0) return;

        auto movePerIMass = contactNormal * (-this->penetration / totalInverseMass);

        particleMovement[0] = movePerIMass * particle[0]->inverseMass();

        if (particle[1]) {
            particleMovement[1] = movePerIMass * -particle[1]->inverseMass();
        } else {
            particleMovement[1].set();
        }

        particle[0]->setPosition(particle[0]->getPosition() + particleMovement[0]);

        if (this->particle[1]) {
            particle[1]->setPosition(particle[1]->getPosition() + particleMovement[1]);
        }
    }


    // ParticleContactResolver

    ParticleContactResolver::ParticleContactResolver(uint iterations) :
        m_Iterations(iterations),
        m_IterationsUsed(0)
    {
    }

    void ParticleContactResolver::setIterations(uint iterations)
    {
        this->m_Iterations = iterations;
    }

    void ParticleContactResolver::resolveContacts(ParticleContact *contactArray, uint numContacts, real duration)
    {
        uint i = 0;

        this->m_IterationsUsed = 0;

        while(m_IterationsUsed < m_Iterations)
        {
            real max = max_real;
            uint maxIndex = numContacts;

            for (i = 0; i < numContacts; ++i)
            {
                auto sepVel = contactArray[i].calculateSeparatingVelocity();

                if (sepVel < max && (sepVel < 0 || contactArray[i].penetration > 0))
                {
                    max = sepVel;
                    maxIndex = i;
                }
            }

            if (maxIndex == numContacts) break;

            contactArray[maxIndex].resolve(duration);

            Vec3 *move = contactArray[maxIndex].particleMovement;

            for (i = 0; i < numContacts; ++i)
            {
                if (contactArray[i].particle[0] == contactArray[maxIndex].particle[0])
                {
                    contactArray[i].penetration -= move[0].scalarProduct(contactArray[i].contactNormal);
                }
                else if (contactArray[i].particle[0] == contactArray[maxIndex].particle[1])
                {
                    contactArray[i].penetration -= move[1].scalarProduct(contactArray[i].contactNormal);
                }

                if (contactArray[i].particle[1])
                {
                    if (contactArray[i].particle[1] == contactArray[maxIndex].particle[0])
                    {
                        contactArray[i].penetration += move[0].scalarProduct(contactArray[i].contactNormal);
                    }
                    else if (contactArray[i].particle[1] == contactArray[maxIndex].particle[1])
                    {
                        contactArray[i].penetration += move[1].scalarProduct(contactArray[i].contactNormal);
                    }
                }
            }

            ++m_IterationsUsed;
        }
    }
}