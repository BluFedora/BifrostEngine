#include "rigidbodyjoint.h"

#include "rigidbody.h"

PRISM_PHYSICS_NAMESPACE_BEGIN

RigidBodyJoint::RigidBodyJoint(RigidBody *a, const Vec3 &aPos, RigidBody *b, const Vec3 &bPos, real error) :
  body{a, b},
  position{aPos, bPos},
  error(error)
{
}

void RigidBodyJoint::set(RigidBody *a, const Vec3 &aPos, RigidBody *b, const Vec3 &bPos, real error)
{
  this->body[0]     = a;
  this->body[1]     = b;
  this->position[0] = aPos;
  this->position[1] = bPos;
  this->error       = error;
}

uint RigidBodyJoint::addContact(Contact *contact, uint limit) const
{
  Vec3 aPosWorld = body[0]->getPointInWorldSpace(position[0]);
  Vec3 bPosWorld = body[1]->getPointInWorldSpace(position[1]);

  Vec3 aToB   = bPosWorld - aPosWorld;
  Vec3 normal = aToB;
  normal.normalize();

  if (limit > 0)
  {
    real length = aToB.length();

    if (abs_real(length) > error)
    {
      contact->body[0]       = this->body[0];
      contact->body[1]       = this->body[1];
      contact->contactNormal = normal;
      contact->contactPoint  = (aPosWorld + bPosWorld) * 0.5;
      contact->friction      = 1.0;
      contact->restitution   = 0.0;

      return 1;
    }
  }

  return 0;
}

PRISM_PHYSICS_NAMESPACE_END