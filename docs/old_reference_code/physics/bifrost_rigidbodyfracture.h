#ifndef RIGIDBODYFRACTURE_H
#define RIGIDBODYFRACTURE_H

#include "collisionfine.h"
#include "particleforcegenerators.h"
#include "rigidbodyforcegenerators.h"

namespace prism
{
    class Block : public Box
    {
        public:
            bool exists;

            Block() :
                Box(),
                exists(false)
            {
                this->body = new RigidBody();
            }

            ~Block()
            {
                delete this->body;
            }

            void setState(const Vec3 &position,
                          const Quat &orientation,
                          const Vec3 &extents,
                          const Vec3 &velocity);

            void calculateMassProperties(real invDensity);

            void divideBlock(const Contact &contact, Block* target, Block* blocks);

    };

    class RigidBodyFracture
    {
        public:
            RigidBodyFracture();

            //TODO(BluFedora):: Implement "RigidBodyFracture"

            /*
            void generateContacts()
            {
                for (auto block : blocks)
                {
                    if (!block.exists) continue;

                    // Bottom Plane
                    if (!cData.hasMoreContacts()) return;
                    cyclone::CollisionDetector::boxAndHalfSpace(*block, plane, &cData);

                    if (ball_active)
                    {
                        // Ball
                        if (!cData.hasMoreContacts()) return;
                        if (cyclone::CollisionDetector::boxAndSphere(*block, ball, &cData))
                        {
                            hit = true;
                            fracture_contact = cData.contactCount-1;
                        }
                    }
                }

                ...Later in update

                // Handle fractures.
                if (hit)
                {
                    blocks[0].divideBlock(
                        cData.contactArray[fracture_contact],
                        blocks,
                        blocks+1
                        );
                    ball_active = false;
                }
            }
        */
    };

    class Explosion : public IRigidBodyForceGenerator, public IParticleForceGenerator
    {
        protected:
            Vec3 m_Center;
            real m_ExplosionForce;
            real m_MaxRadius;
            real m_MinRadius;
            real timePassed;

        public:
            Vec3 detonation;
            real implosionMaxRadius;
            real implosionMinRadius;
            real implosionDuration;
            real implosionForce;
            real shockwaveSpeed;
            real shockwaveThickness;
            real peakConcussionForce;
            real concussionDuration;
            real peakConvectionForce;
            real chimneyRadius;
            real chimneyHeight;
            real convectionDuration;

        public:
            Explosion(const Vec3 &center, const real maxRadius, const real minRadius) :
                m_Center(center),
                m_ExplosionForce(10.0),
                m_MaxRadius(maxRadius),
                m_MinRadius(minRadius),
                timePassed(0.0),
                detonation(0.0, 0.0, 0.0),
                implosionMaxRadius(0.0),
                implosionMinRadius(0.0),
                implosionDuration(0.0),
                implosionForce(0.0),
                shockwaveSpeed(0.0),
                shockwaveThickness(0.0),
                peakConcussionForce(0.0),
                concussionDuration(0.0),
                peakConvectionForce(0.0),
                chimneyRadius(0.0),
                chimneyHeight(0.0),
                convectionDuration(0.0)
            {
            }

            void updateForce(Particle* particle, real duration) override // TODO(BluFedora) Implement Explosion for particles
            {
                (void) particle;
                (void) duration;
            }

            void updateForce(RigidBody* body, real duration) override
            {
                auto dist = body->getPosition().distance(this->m_Center);

                if (dist <= this->m_MaxRadius && dist >= this->m_MinRadius)
                {
                    real distFactor = (1 - (dist / this->m_MaxRadius));
                    auto dir        = (body->getPosition() - this->m_Center);
                    auto force      = ((dir * this->m_ExplosionForce) * distFactor);

                    body->addForce(force * duration);
                }
            }

            ~Explosion();

    };
}

#endif // RIGIDBODYFRACTURE_H