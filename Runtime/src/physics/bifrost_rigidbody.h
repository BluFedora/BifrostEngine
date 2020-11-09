#ifndef RIGIDBODY_H
#define RIGIDBODY_H

#include "bifrost_prismtypes.h"

#include "bifrost/bifrost_math.hpp"

using namespace prism;

namespace bf
{
  class RigidBody
  {
   public:
    Vector3f    position;
    Vector3f    velocity;
    Vector3f    acceleration;
    Quaternionf orientation;  // Keep this field normalized.
    Vector3f    rotation;     // Angular Acceleration
    Vector3f    forceAccum;
    Vector3f    torqueAccum;
    Mat3x3      inverseInertiaTensor;
    real        inverseMass;
    real        linearDamping;
    real        angularDamping;
    real        motion;    // Holds the amount of motion of the body. This is a recency weighted mean that can be used to put a body to sleap.
    bool        isAwake;   // TODO(SR): This should be a bitset.
    bool        canSleep;  // TODO(SR): This should be a bitset.
    Mat4x3      transformMatrix;
    Mat3x3      inverseInertiaTensorWorld;
    Vector3f    lastFrameAcceleration;

   public:
    RigidBody();

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
}  // namespace bifrost

#endif  // RIGIDBODY_H