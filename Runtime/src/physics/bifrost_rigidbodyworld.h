#ifndef RIGIDBODYWORLD_H
#define RIGIDBODYWORLD_H

#include "rigidbody.h"
#include "rigidbodycontact.h"
#include "rigidbodyforcegenerators.h"

namespace prism
{
  class RigidBodyWorld
  {
   private:
    struct BodyRegistration
    {
      RigidBody*        body;
      BodyRegistration* next;
    };

    struct ContactGenRegistration
    {
      ContactGenerator*       gen;
      ContactGenRegistration* next;
    };

   private:
    RigidBodyForceRegistry* registry;
    BodyRegistration*       brRoot;
    ContactGenRegistration* cgRoot;
    Contact*                contacts;
    ContactResolver         resolver;
    bool                    calculateIterations;
    uint                    maxContacts;

   public:
    RigidBodyWorld(uint maxContacts, uint iterations = 0) :
      registry(nullptr),
      brRoot(nullptr),
      cgRoot(nullptr),
      contacts(new Contact[maxContacts]),
      resolver(iterations),
      calculateIterations(iterations == 0),
      maxContacts(maxContacts)
    {
    }

    ~RigidBodyWorld()
    {
      auto reg = this->brRoot;

      while (reg)
      {
        auto old = reg;
        reg      = reg->next;
        delete old;
      }

      delete[] this->contacts;
    }

    void addBody(RigidBody* body)
    {
      auto reg = this->brRoot;

      while (reg)
      {
        reg = reg->next;
      }

      reg->next = new BodyRegistration{body, nullptr};
    }

    uint generateContacts()
    {
      uint                    limit(maxContacts);
      Contact*                nextContact = contacts;
      ContactGenRegistration* reg         = this->cgRoot;

      while (reg)
      {
        uint used = reg->gen->addContact(nextContact, limit);
        limit -= used;
        nextContact += used;

        if (limit <= 0) break;

        reg = reg->next;
      }

      return maxContacts - limit;
    }

    void startFrame()
    {
      auto reg = this->brRoot;

      while (reg)
      {
        reg->body->clearAccumulators();
        reg->body->calculateDerivedData();

        reg = reg->next;
      }
    }

    void runPhysics(real duration)
    {
      registry->updateForces(duration);

      auto reg = this->brRoot;

      while (reg)
      {
        reg->body->integrate(duration);
        reg = reg->next;
      }

      uint usedContacts = generateContacts();

      if (calculateIterations)
      {
        resolver.setIterations(usedContacts * 4);
      }

      resolver.resolveContacts(contacts, usedContacts, duration);
    }
  };
}  // namespace prism

#endif  // RIGIDBODYWORLD_H