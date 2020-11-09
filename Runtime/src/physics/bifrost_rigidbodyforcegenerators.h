#ifndef RIGIDBODYFORCEGENERATORS_H
#define RIGIDBODYFORCEGENERATORS_H

#include "../ds/pair.h"
#include "rigidbody.h"

#include <vector>

namespace prism
{
  class IRigidBodyForceGenerator
  {
   public:
    virtual void updateForce(RigidBody *body, real duration) = 0;
  };

  class FGRigidBodyGravity : public IRigidBodyForceGenerator
  {
   protected:
    Vec3 gravity;

   public:
    FGRigidBodyGravity(const Vec3 &gravity) :
      IRigidBodyForceGenerator(),
      gravity(gravity)
    {
    }

    // IRigidBodyForceGenerator interface
   public:
    void updateForce(RigidBody *body, real duration) override
    {
      (void)duration;

      if (!body->hasFiniteMass()) return;

      body->addForce(this->gravity * body->getMass());
    }
  };

  class FGRigidBodyAero : public IRigidBodyForceGenerator
  {
   protected:
    Mat3x3      tensor;
    Vec3        position;
    const Vec3 *windspeed;

   public:
    FGRigidBodyAero(const Mat3x3 &tensor, const Vec3 &position, const Vec3 *windspeed) :
      IRigidBodyForceGenerator(),
      tensor(tensor),
      position(position),
      windspeed(windspeed)
    {
    }

    // IRigidBodyForceGenerator interface
   public:
    void updateForce(RigidBody *body, real duration) override
    {
      (void)duration;
      (void)body;
    }
  };

  class FGRigidBodyAeroControl : public FGRigidBodyAero
  {
   protected:
    Mat3x3 maxTensor;
    Mat3x3 minTensor;
    real   controllSetting;

   public:
    FGRigidBodyAeroControl(const Mat3x3 &tensor, const Vec3 &position, const Vec3 *windspeed) :
      FGRigidBodyAero(tensor, position, windspeed)
    {
    }

    void setControl(real value)
    {
      this->controllSetting = value;
    }

    // IRigidBodyForceGenerator interface
   public:
    void updateForce(RigidBody *body, real duration) override
    {
      (void)duration;
      (void)body;
    }
  };

  class FGRigidBodySpring : public IRigidBodyForceGenerator
  {
   protected:
    Vec3       connectionPoint;
    Vec3       otherConnectionPoint;
    RigidBody *other;
    real       springConstant;
    real       restLength;

   public:
    FGRigidBodySpring(const auto &cp, const auto &ocp, RigidBody *other, real springK, real restLen) :
      IRigidBodyForceGenerator(),
      connectionPoint(cp),
      otherConnectionPoint(ocp),
      other(other),
      springConstant(springK),
      restLength(restLen)
    {
    }

    // IRigidBodyForceGenerator interface
   public:
    void updateForce(RigidBody *body, real duration) override
    {
      (void)duration;

      auto lws   = body->getPointInWorldSpace(this->connectionPoint);
      auto ows   = other->getPointInWorldSpace(this->otherConnectionPoint);
      auto force = lws - ows;
      real mag   = force.length();

      mag = /*abs_real*/ (mag - this->restLength);
      mag *= springConstant;

      force.normalize();
      force *= -mag;

      body->addForceAtBodyPoint(force, lws);
    }
  };

  class RigidBodyForceRegistry
  {
   protected:
    typedef Pair<RigidBody *, IRigidBodyForceGenerator *> RigidBodyForcePair;
    typedef std::vector<RigidBodyForcePair>               Registry;

   protected:
    Registry m_Registry;

   public:
    RigidBodyForceRegistry();

    void add(RigidBody *body, IRigidBodyForceGenerator *forceGen);
    void remove(RigidBody *body, IRigidBodyForceGenerator *forceGen);
    void updateForces(real duration);
    void clear();

    ~RigidBodyForceRegistry();
  };
}  // namespace prism

#endif  // RIGIDBODYFORCEGENERATORS_H