#include "rigidbodyfracture.h"

#include "rigidbody.h"
#include "rigidbodycontact.h"

namespace prism
{
    RigidBodyFracture::RigidBodyFracture()
    {
    }

    // class Block;

    void Block::setState(const Vec3 &position, const Quat &orientation, const Vec3 &extents, const Vec3 &velocity)
    {
        this->halfSize = extents;

        auto mass = halfSize.x * halfSize.y * halfSize.z * 8.0;
        Mat3x3 tensor;

        tensor.setBlockInertiaTensor(this->halfSize, mass);

        this->body->setPosition(position);
        this->body->setOrientation(orientation);
        this->body->setVelocity(velocity);
        this->body->setRotation(Vec3(0.0, 0.0, 0.0));
        this->body->setMass(mass);
        this->body->setInertiaTensor(tensor);
        this->body->setLinearDamping(0.95);
        this->body->setAngularDamping(0.8);
        this->body->clearAccumulators();
        this->body->setAcceleration(Vec3(0, -10.0, 0));
        this->body->setCanSleep(false);
        this->body->setAwake(true);
        this->body->calculateDerivedData();
    }

    void Block::calculateMassProperties(real invDensity)
    {
        if (invDensity <= 0.0)
        {
            this->body->setInverseMass(0.0);
            this->body->setInertiaTensor(Mat3x3());
        }
        else
        {
            auto volume = halfSize.length() * 2.0;
            auto mass   = volume / invDensity;

            body->setMass(mass);

            mass *= 0.333;

            Mat3x3 tensor;
            tensor.setInertiaTensorCoeffs(
                        mass * halfSize.y * halfSize.y + halfSize.z * halfSize.z,
                        mass * halfSize.y * halfSize.x + halfSize.z * halfSize.z,
                        mass * halfSize.y * halfSize.x + halfSize.z * halfSize.y
                        );

            this->body->setInertiaTensor(tensor);
        }
    }

    void Block::divideBlock(const Contact &contact, Block *target, Block *blocks)
    {
        static const Vec3 GRAVITY = Vec3(0.0, -9.81, 0.0);

        auto normal = contact.contactNormal;
        auto body = contact.body[0];

        if (body != target->body)
        {
            normal.invert();
            body = contact.body[1];
        }

        auto point = body->getPointInLocalSpace(contact.contactPoint);
        normal = body->getDirectionInLocalSpace(normal);
        point = point - normal * (point * normal);

        auto size = target->halfSize;

        RigidBody tempBody;
        tempBody.setPosition(body->getPosition());
        tempBody.setOrientation(body->getOrientation());
        tempBody.setVelocity(body->getVelocity());
        tempBody.setRotation(body->getRotation());
        tempBody.setLinearDamping(body->getLinearDamping());
        tempBody.setAngularDamping(body->getAngularDamping());
        tempBody.setInverseInertiaTensor(body->getInverseInertiaTensor());
        tempBody.calculateDerivedData();

        target->exists = false;

        auto invDensity = halfSize.length() * 8.0 * body->getInverseMass();

        for (uint i = 0; i < 8; ++i)
        {
            Vec3 min, max;

            if ((i & 1) == 0) {
                min.x = -size.x;
                max.x = point.x;
            } else {
                min.x = point.x;
                max.x = size.x;
            }
            if ((i & 2) == 0) {
                min.y = -size.y;
                max.y = point.y;
            } else {
                min.y = point.y;
                max.y = size.y;
            }
            if ((i & 4) == 0) {
                min.z = -size.z;
                max.z = point.z;
            } else {
                min.z = point.z;
                max.z = size.z;
            }

            Vec3 halfSize = (max - min) * 0.5;
            Vec3 newPos = halfSize + min;

            newPos = tempBody.getPointInWorldSpace(newPos);

            auto direction = newPos - contact.contactPoint;
            direction.normalize();

            blocks[i].body->setPosition(newPos);
            blocks[i].body->setVelocity(tempBody.getVelocity() + direction * 10.0);
            blocks[i].body->setOrientation(tempBody.getOrientation());
            blocks[i].body->setRotation(tempBody.getRotation());
            blocks[i].body->setLinearDamping(tempBody.getLinearDamping());
            blocks[i].body->setAngularDamping(tempBody.getAngularDamping());
            blocks[i].body->setAwake(true);
            blocks[i].body->setAcceleration(GRAVITY);
            blocks[i].body->clearAccumulators();
            blocks[i].body->calculateDerivedData();

            blocks[i].offset = Mat4x3();
            blocks[i].exists = true;
            blocks[i].halfSize = halfSize;

            blocks[i].calculateMassProperties(invDensity);
        }
    }

    Explosion::~Explosion()
    {

    }
}