#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "bifrost_prismtypes.h"

namespace prism
{
  class RigidBody
  {
   protected:
    Vec3   position;
    Vec3   velocity;
    Vec3   acceleration;
    Quat   orientation;
    Vec3   rotation;  // Angular Acceleration
    Vec3   forceAccum;
    Vec3   torqueAccum;
    Mat3x3 inverseInertiaTensor;
    real   inverseMass;
    real   linearDamping;
    real   angularDamping;
    real   motion;  // Holds the amount of motion of the body. This is a recency weighted mean that can be used to put a body to sleap.
    bool   isAwake;
    bool   canSleep;
    Mat4x3 transformMatrix;
    Mat3x3 inverseInertiaTensorWorld;
    Vec3   lastFrameAcceleration;

   public:
    RigidBody();

    Vec3 getPosition() const;
    void addPosition(const Vec3 &deltaPos);
    void setPosition(const Vec3 &pos);

    Vec3 getVelocity() const;
    void addVelocity(const Vec3 &deltaVel);
    void setVelocity(const Vec3 &vel);

    Vec3 getAcceleration() const;
    void setAcceleration(const Vec3 &accel);

    Quat getOrientation() const;
    void setOrientation(const Quat &orientiation);

    Vec3 getRotation() const;
    void addRotation(const Vec3 &deltaRot);
    void setRotation(const Vec3 &rot);

    real getMass() const;
    void setMass(const real mass);
    real getInverseMass() const;
    void setInverseMass(const real invMass);
    bool hasFiniteMass() const;

    real getLinearDamping() const { return this->linearDamping; }
    void setLinearDamping(const real damping);
    real getAngularDamping() const { return this->angularDamping; }
    void setAngularDamping(const real damping);

    bool getAwake() const;
    void setAwake(const bool awake = true);
    void setCanSleep(const bool canSleep = true);

    Mat4x3 getTransform() const;

    void getInverseInertiaTensorWorld(Mat3x3 *mat) const;

    Mat3x3 getInverseInertiaTensor() const;
    void   setInverseInertiaTensor(const Mat3x3 &mat);
    void   setInertiaTensor(const Mat3x3 &initiaTensor);

    Vec3 getLastFrameAcceleration() const;

    void clearAccumulators();

    void addForce(const Vec3 &force);
    void addForceAtBodyPoint(const Vec3 &force, const Vec3 &point);
    void addForceAtPoint(const Vec3 &force, const Vec3 &point);
    void addTorque(const Vec3 &torque);

    void integrate(real duration);

    void calculateDerivedData();

    Vec3 getDirectionInLocalSpace(const Vec3 &direction);
    Vec3 getPointInLocalSpace(const Vec3 &point);
    Vec3 getPointInWorldSpace(const Vec3 &point);
  };
}  // namespace prism

#endif  // RIGIDBODY_H