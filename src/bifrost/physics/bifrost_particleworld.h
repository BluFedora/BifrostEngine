#ifndef PARTICLEWORLD_H
#define PARTICLEWORLD_H

#include "particle.h"
#include "particlecontacts.h"
#include "particleforcegenerators.h"

namespace prism
{
  class IParticleContactGenerator
  {
   public:
    virtual uint addContact(ParticleContact* contact, uint limit) = 0;
  };

  class ParticleWorld
  {
   private:
    struct ParticleRegistration
    {
      Particle*             particle;
      ParticleRegistration* next;
    };

    struct ContectGenRegistration
    {
      IParticleContactGenerator* gen;
      ContectGenRegistration*    next;
    };

   protected:
    ParticleRegistration*   prRoot;
    ParticleForceRegistry*  registry;
    ParticleContactResolver resolver;

    ContectGenRegistration* cgRoot;
    ParticleContact*        contacts;
    uint                    maxContacts;

   public:
    ParticleWorld(uint maxContacts, uint iterations = 0) :
      prRoot(nullptr),
      registry(nullptr),
      resolver(iterations),
      cgRoot(nullptr),
      contacts(nullptr),
      maxContacts(maxContacts)
    {
    }

    uint generateContacts()
    {
      auto                    limit       = this->maxContacts;
      ParticleContact*        nextContact = this->contacts;
      ContectGenRegistration* reg         = this->cgRoot;

      while (reg)
      {
        auto used = reg->gen->addContact(nextContact, limit);
        limit -= used;
        nextContact += used;

        if (limit <= 0) break;

        reg = reg->next;
      }

      return maxContacts - limit;
    }

    void startFrame()
    {
      auto reg = this->prRoot;

      while (reg)
      {
        reg->particle->clearAccumlulator();
        reg = reg->next;
      }
    }

    void integrate(real duration)
    {
      auto reg = this->prRoot;

      while (reg)
      {
        reg->particle->integrate(duration);
        reg = reg->next;
      }
    }

    void runPhysics(real duration)
    {
      static const bool calculateIterations = true;

      registry->updateForces(duration);
      this->integrate(duration);
      uint usedContacts = this->generateContacts();

      if (calculateIterations)
      {
        this->resolver.setIterations(usedContacts * 2);
      }

      this->resolver.resolveContacts(contacts, usedContacts, duration);
    }

    // Do:
    // startFrame();
    // updateGameplay();
    // runPhysics();
  };
}  // namespace prism

#endif  // PARTICLEWORLD_H