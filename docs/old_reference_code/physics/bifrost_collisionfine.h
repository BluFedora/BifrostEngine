#ifndef COLLISIONFINE_H
#define COLLISIONFINE_H

#include "prismtypes.h"

namespace prism
{
  class CollisionData
  {
   public:
    Contact *contactArray;
    Contact *contacts;
    int      contactsLeft;
    uint     contactCount;
    real     friction;
    real     restitution;
    real     tolerance;

   public:
    bool hasMoreContacts();
    void reset(uint maxContacts);
    void addContacts(uint count);
  };

  class Primitive
  {
    friend class CollisionDetector;

   protected:
    Mat4x3 transform;

   public:
    RigidBody *body;
    Mat4x3     offset;

    void calculateInternals();

    Vec3 getAxis(uint index) const;

    const Mat4x3 &getTransform() const;
  };

  class Sphere : public Primitive
  {
   private:
    real m_Radius;

   public:
    Sphere(real radius = 0.0) :
      Primitive(),
      m_Radius(radius)
    {
    }

    real radius() const
    {
      return this->m_Radius;
    }

    void setRadius(real value)
    {
      this->m_Radius = value;
    }
  };

  class Box : public Primitive
  {
   public:
    Vec3 halfSize;

    Sphere toSphere() const
    {
      Sphere sphere;

      sphere.body   = this->body;
      sphere.offset = this->offset;

      sphere.setRadius(this->halfSize.x);
      if (halfSize.y < sphere.radius()) sphere.setRadius(halfSize.y);
      if (halfSize.z < sphere.radius()) sphere.setRadius(halfSize.z);

      sphere.calculateInternals();

      return sphere;
    }
  };

  class Plane
  {
   public:
    Vec3 direction;
    real offset;
  };

  class IntersectionTests
  {
   public:
    static bool sphereAndHalfSpace(const Sphere &sphere, const Plane &plane);
    static bool sphereAndSphere(const Sphere &sphere1, const Sphere &sphere2);
    static bool boxAndBox(const Box &box1, const Box &box2);
    static bool boxAndHalfSpace(const Box &box, const Plane &plane);
  };

  class CollisionDetector
  {
   public:
    static uint sphereAndSphere(const Sphere &one, const Sphere &two, CollisionData *data);
    static uint sphereAndHalfSpace(const Sphere &sphere, const Plane &plane, CollisionData *data);
    static uint sphereAndTruePlane(const Sphere &sphere, const Plane &plane, CollisionData *data);
    static uint sphereAndPoint(const Sphere &sphere, const Vec3 &point, CollisionData *data);
    static uint boxAndHalfSpace(const Box &box, const Plane &plane, CollisionData *data);
    static uint boxAndBox(const Box &one, const Box &two, CollisionData *data);
    static uint boxAndPoint(const Box &box, const Vec3 &point, CollisionData *data);
    static uint boxAndSphere(const Box &box, const Sphere &sphere, CollisionData *data);
  };
}  // namespace prism

#endif  // COLLISIONFINE_H