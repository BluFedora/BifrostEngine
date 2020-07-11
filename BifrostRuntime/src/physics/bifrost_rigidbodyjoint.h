#ifndef RIGIDBODYJOINT_H
#define RIGIDBODYJOINT_H

#include "prismtypes.h"
#include "rigidbodycontact.h"

PRISM_PHYSICS_NAMESPACE_BEGIN

class RigidBody;

class RigidBodyJoint : public ContactGenerator
{
 public:
  RigidBody *body[2];
  Vec3       position[2];
  real       error;

 public:
  RigidBodyJoint(RigidBody *a = nullptr, const Vec3 &aPos = Vec3(0.0, 0.0, 0.0), RigidBody *b = nullptr, const Vec3 &bPos = Vec3(0.0, 0.0, 0.0), real error = 0.0);

  void set(RigidBody *a, const Vec3 &aPos, RigidBody *b, const Vec3 &bPos, real error);

  uint addContact(Contact *contact, uint limit) const override;
};

PRISM_PHYSICS_NAMESPACE_END

#endif  // RIGIDBODYJOINT_H