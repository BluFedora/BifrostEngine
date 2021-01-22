#include "bifrost_rigidbody.h"

namespace bf
{
  const static real sleepEpsilon = (real)0.3;

  static void _transformInertiaTensor(Mat3x3 &      iitWorld,
                                      const Mat3x3 &iitBody,
                                      const Mat4x3 &rotmat)
  {
    const real t4 = rotmat.data[0] * iitBody.data[0] +
                    rotmat.data[1] * iitBody.data[3] +
                    rotmat.data[2] * iitBody.data[6];
    const real t9 = rotmat.data[0] * iitBody.data[1] +
                    rotmat.data[1] * iitBody.data[4] +
                    rotmat.data[2] * iitBody.data[7];
    const real t14 = rotmat.data[0] * iitBody.data[2] +
                     rotmat.data[1] * iitBody.data[5] +
                     rotmat.data[2] * iitBody.data[8];
    const real t28 = rotmat.data[4] * iitBody.data[0] +
                     rotmat.data[5] * iitBody.data[3] +
                     rotmat.data[6] * iitBody.data[6];
    const real t33 = rotmat.data[4] * iitBody.data[1] +
                     rotmat.data[5] * iitBody.data[4] +
                     rotmat.data[6] * iitBody.data[7];
    const real t38 = rotmat.data[4] * iitBody.data[2] +
                     rotmat.data[5] * iitBody.data[5] +
                     rotmat.data[6] * iitBody.data[8];
    const real t52 = rotmat.data[8] * iitBody.data[0] +
                     rotmat.data[9] * iitBody.data[3] +
                     rotmat.data[10] * iitBody.data[6];
    const real t57 = rotmat.data[8] * iitBody.data[1] +
                     rotmat.data[9] * iitBody.data[4] +
                     rotmat.data[10] * iitBody.data[7];
    const real t62 = rotmat.data[8] * iitBody.data[2] +
                     rotmat.data[9] * iitBody.data[5] +
                     rotmat.data[10] * iitBody.data[8];

    iitWorld.data[0] = t4 * rotmat.data[0] +
                       t9 * rotmat.data[1] +
                       t14 * rotmat.data[2];
    iitWorld.data[1] = t4 * rotmat.data[4] +
                       t9 * rotmat.data[5] +
                       t14 * rotmat.data[6];
    iitWorld.data[2] = t4 * rotmat.data[8] +
                       t9 * rotmat.data[9] +
                       t14 * rotmat.data[10];
    iitWorld.data[3] = t28 * rotmat.data[0] +
                       t33 * rotmat.data[1] +
                       t38 * rotmat.data[2];
    iitWorld.data[4] = t28 * rotmat.data[4] +
                       t33 * rotmat.data[5] +
                       t38 * rotmat.data[6];
    iitWorld.data[5] = t28 * rotmat.data[8] +
                       t33 * rotmat.data[9] +
                       t38 * rotmat.data[10];
    iitWorld.data[6] = t52 * rotmat.data[0] +
                       t57 * rotmat.data[1] +
                       t62 * rotmat.data[2];
    iitWorld.data[7] = t52 * rotmat.data[4] +
                       t57 * rotmat.data[5] +
                       t62 * rotmat.data[6];
    iitWorld.data[8] = t52 * rotmat.data[8] +
                       t57 * rotmat.data[9] +
                       t62 * rotmat.data[10];
  }

  /**
     * Inline function that creates a transform matrix from a
     * position and orientation.
     */
  static void _calculateTransformMatrix(Mat4x3 &    transformMatrix,
                                               const Vec3 &position,
                                               const Quat &orientation)
  {
    transformMatrix.data[0] = 1 - 2 * orientation.j * orientation.j -
                              2 * orientation.k * orientation.k;
    transformMatrix.data[1] = 2 * orientation.i * orientation.j -
                              2 * orientation.r * orientation.k;
    transformMatrix.data[2] = 2 * orientation.i * orientation.k +
                              2 * orientation.r * orientation.j;
    transformMatrix.data[3] = position.x;

    transformMatrix.data[4] = 2 * orientation.i * orientation.j +
                              2 * orientation.r * orientation.k;
    transformMatrix.data[5] = 1 - 2 * orientation.i * orientation.i -
                              2 * orientation.k * orientation.k;
    transformMatrix.data[6] = 2 * orientation.j * orientation.k -
                              2 * orientation.r * orientation.i;
    transformMatrix.data[7] = position.y;

    transformMatrix.data[8] = 2 * orientation.i * orientation.k -
                              2 * orientation.r * orientation.j;
    transformMatrix.data[9] = 2 * orientation.j * orientation.k +
                              2 * orientation.r * orientation.i;
    transformMatrix.data[10] = 1 - 2 * orientation.i * orientation.i -
                               2 * orientation.j * orientation.j;
    transformMatrix.data[11] = position.z;
  }

  RigidBody::RigidBody() :
    position(0.0, 0.0, 0.0),
    velocity(0.0, 0.0, 0.0),
    acceleration(0.0, 0.0, 0.0),
    orientation(1.0, 0.0, 0.0, 0.0),
    rotation(0.0, 0.0, 0.0),
    forceAccum(0.0, 0.0, 0.0),
    torqueAccum(0.0, 0.0, 0.0),
    inverseInertiaTensor(),
    inverseMass(0.0),
    linearDamping(0.0),
    angularDamping(0.0),
    motion(0.0),
    isAwake(true),
    canSleep(true),
    transformMatrix(),
    inverseInertiaTensorWorld(),
    lastFrameAcceleration(0.0, 0.0, 0.0)
  {
  }

  real RigidBody::getMass() const
  {
    return (this->inverseMass == 0.0) ? 0.0 : (static_cast<real>(1.0) / this->inverseMass);
  }

  void RigidBody::setMass(const real mass)
  {
    this->inverseMass = (mass == 0.0) ? 0.0 : (static_cast<real>(1.0) / mass);
  }

  real RigidBody::getInverseMass() const
  {
    return this->inverseMass;
  }

  void RigidBody::setInverseMass(const real invMass)
  {
    this->inverseMass = invMass;
  }

  bool RigidBody::hasFiniteMass() const
  {
    return (inverseMass >= 0.0);
  }

  void RigidBody::setLinearDamping(const real damping)
  {
    this->linearDamping = damping;
  }

  void RigidBody::setAngularDamping(const real damping)
  {
    this->angularDamping = damping;
  }

  bool RigidBody::getAwake() const
  {
    return this->isAwake;
  }

  void RigidBody::setAwake(const bool awake)
  {
    if (awake)
    {
      this->motion = sleepEpsilon * 2.0;
    }
    else
    {
      this->velocity.set();
      this->rotation.set();
    }

    this->isAwake = awake;
  }

  void RigidBody::setCanSleep(const bool canSleep)
  {
    this->canSleep = canSleep;
  }

  Mat4x3 RigidBody::getTransform() const
  {
    return this->transformMatrix;
  }

  void RigidBody::getInverseInertiaTensorWorld(Mat3x3 *mat) const
  {
    *mat = this->inverseInertiaTensorWorld;
  }

  Mat3x3 RigidBody::getInverseInertiaTensor() const
  {
    return this->inverseInertiaTensor;
  }

  void RigidBody::setInverseInertiaTensor(const Mat3x3 &mat)
  {
    this->inverseInertiaTensor = mat;
  }

  void RigidBody::setInertiaTensor(const Mat3x3 &initiaTensor)
  {
    this->inverseInertiaTensor.setInverse(initiaTensor);
  }

  Vec3 RigidBody::getLastFrameAcceleration() const
  {
    return this->lastFrameAcceleration;
  }

  void RigidBody::clearAccumulators()
  {
    this->forceAccum.set();
    this->torqueAccum.set();
  }

  void RigidBody::addForce(const Vec3 &force)
  {
    forceAccum += force;
    isAwake = true;
  }

  void RigidBody::addForceAtBodyPoint(const Vec3 &force, const Vec3 &point)
  {
    auto pt = getPointInWorldSpace(point);
    this->addForceAtPoint(force, pt);
  }

  void RigidBody::addForceAtPoint(const Vec3 &force, const Vec3 &point)
  {
    auto pt = point.clone();
    pt -= position;

    this->addForce(force);
    this->addTorque(pt % force);

    isAwake = true;
  }

  void RigidBody::addTorque(const Vec3 &torque)
  {
    this->torqueAccum += torque;
    this->isAwake = true;
  }

  void RigidBody::integrate(real duration)
  {
    static const real MAX_MOTION = (10 * sleepEpsilon);

    if (!isAwake) return;

    auto angularAcceleration = inverseInertiaTensor.transform(this->torqueAccum);

    lastFrameAcceleration = this->acceleration.clone();
    lastFrameAcceleration.addScaledVector(this->forceAccum, this->getInverseMass());

    velocity.addScaledVector(lastFrameAcceleration, duration);

    rotation.addScaledVector(angularAcceleration, duration);

    velocity *= pow_real(this->linearDamping, duration);
    rotation *= pow_real(this->angularDamping, duration);

    position.addScaledVector(this->velocity, duration);

    this->orientation.addScaledVector(this->rotation, duration);

    velocity *= pow_real(this->linearDamping, duration);
    rotation *= pow_real(this->angularDamping, duration);

    this->calculateDerivedData();
    this->clearAccumulators();

    if (this->canSleep)
    {
      auto currentMotion = velocity.scalarProduct(velocity) + rotation.scalarProduct(rotation);
      auto bias          = pow_real(0.5, duration);
      motion             = bias * motion + (1 - bias) * currentMotion;

      if (motion < sleepEpsilon)
      {
        this->setAwake(false);
      }
      else if (motion > MAX_MOTION)
      {
        motion = MAX_MOTION;
      }
    }
  }

  void RigidBody::calculateDerivedData()
  {
    this->orientation.normalize();
    _calculateTransformMatrix(this->transformMatrix, this->position, this->orientation);
    _transformInertiaTensor(this->inverseInertiaTensorWorld, this->orientation, this->inverseInertiaTensor, this->transformMatrix);
  }

  Vec3 RigidBody::getDirectionInLocalSpace(const Vec3 &direction)
  {
    return transformMatrix.transformInverseDirection(direction);
  }

  Vec3 RigidBody::getPointInLocalSpace(const Vec3 &point)
  {
    return this->transformMatrix.transformInverseDirection(point);
  }

  Vec3 RigidBody::getPointInWorldSpace(const Vec3 &point)
  {
    return this->transformMatrix.transform(point);
  }
}  // namespace bifrost
