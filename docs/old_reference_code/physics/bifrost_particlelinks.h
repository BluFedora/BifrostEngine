#ifndef PARTICLELINKS_H
#define PARTICLELINKS_H

#include "particle.h"
#include "particlecontacts.h"

class ParticleLink
{
 public:
  Particle *particle[2];

 protected:
  real currentLength() const
  {
    auto relativePos = particle[0]->m_Position - particle[0]->m_Position;
    return relativePos.length();
  }

  virtual uint fillContect(ParticleContact *contact, uint limit) = 0;
};

class ParticleCable : public ParticleLink
{
 public:
  real maxLength;
  real restitution;

  // ParticleLink interface
 protected:
  uint fillContect(ParticleContact *contact, uint limit) override
  {
    (void)limit;

    auto length = this->currentLength();

    if (length < maxLength)
    {
      return 0;
    }

    contact->particle[0] = this->particle[0];
    contact->particle[1] = this->particle[1];

    auto normal = particle[1]->m_Position - particle[0]->m_Position;
    normal.normalize();
    contact->contactNormal = normal;

    contact->penetration = length - maxLength;
    contact->resitution  = this->restitution;

    return 1;
  }
};

class ParticleRod : public ParticleLink
{
 public:
  real length;

  // ParticleLink interface
 protected:
  uint fillContect(ParticleContact *contact, uint limit) override
  {
    (void)limit;

    auto currLength = this->currentLength();

    if (currLength == length)
    {
      return 0;
    }

    contact->particle[0] = this->particle[0];
    contact->particle[1] = this->particle[1];

    auto normal = particle[1]->m_Position - particle[0]->m_Position;
    normal.normalize();

    if (currLength > length)
    {
      //contact->contactNormal = normal;
      contact->penetration = length - currLength;
    }
    else
    {
      normal.invert();  //contact->contactNormal = normal * -1;
      contact->penetration = length - currLength;
    }

    contact->contactNormal = normal;
    contact->resitution    = 0;

    return 1;
  }
};

#endif  // PARTICLELINKS_H