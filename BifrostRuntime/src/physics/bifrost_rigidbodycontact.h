#ifndef RIGIDBODYCONTACT_H
#define RIGIDBODYCONTACT_H

#include "bifrost_prismtypes.h"

PRISM_PHYSICS_NAMESPACE_BEGIN

class RigidBody;
class ContactResolver;

class Contact
{
  friend class ContactResolver;

 public:
  RigidBody *body[2];
  real       friction;
  real       restitution;
  Vec3       contactPoint;
  Vec3       contactNormal;
  real       penetration;

 protected:
  Mat3x3 contactToWorld;
  Vec3   contactVelocity;
  real   desiredDeltaVelocity;
  Vec3   relativeContactPosition[2];

  void calculateInternals(real duration);

  void swapBodies();
  void matchAwakeState();

  void calculateDesiredDeltaVelocity(real duration);
  Vec3 calculateLocalVelocity(unsigned bodyIndex, real duration);
  void calculateContactBasis();

  void applyVelocityChange(Vec3 velocityChange[2],
                           Vec3 rotationChange[2]);

  void applyPositionChange(Vec3 linearChange[2],
                           Vec3 angularChange[2],
                           real penetration);

  Vec3 calculateFrictionlessImpulse(Mat3x3 *inverseInertiaTensor);
  Vec3 calculateFrictionImpulse(Mat3x3 *inverseInertiaTensor);

 public:
  void setBodyData(RigidBody *one, RigidBody *two, real friction, real restitution);
};

class ContactResolver
{
 protected:
  uint velocityIterations;
  uint positionIterations;
  real velocityEpsilon;
  real positionEpsilon;

 public:
  uint velocityIterationsUsed;
  uint positionIterationsUsed;

 private:
  bool validSettings;

 public:
  ContactResolver(uint iterations, real velocityEpsilon = (real)0.01, real positionEpsilon = (real)0.01);
  ContactResolver(uint velocityIterations, uint positionIterations, real velocityEpsilon = (real)0.01, real positionEpsilon = (real)0.01);

  bool isValid()
  {
    return (velocityIterations > 0) &&
           (positionIterations > 0) &&
           (positionEpsilon >= 0.0) &&
           (positionEpsilon >= 0.0);
  }

  void setIterations(uint velocityIterations, uint positionIterations);
  void setIterations(uint iterations);
  void setEpsilon(real velocityEpsilon, real positionEpsilon);

  void resolveContacts(Contact *contactArray, uint numContacts, real duration);

 protected:
  void prepareContacts(Contact *contactArray, uint numContacts, real duration);
  void adjustVelocities(Contact *contactArray, uint numContacts, real duration);
  void adjustPositions(Contact *contacts, uint numContacts, real duration);
};

class ContactGenerator
{
 public:
  virtual uint addContact(Contact *contact, uint limit) const = 0;
};

PRISM_PHYSICS_NAMESPACE_END

#endif  // RIGIDBODYCONTACT_H