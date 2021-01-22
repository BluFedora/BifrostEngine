#ifndef FORCEGENERATORS_H
#define FORCEGENERATORS_H

#include "../ds/pair.h"
#include "../utils.h"
#include "particle.h"

namespace bifrost
{
  class Particle;
}

namespace prism
{
  class IParticleForceGenerator
  {
   public:
    virtual void updateForce(bifrost::Particle *particle, real duration);

    virtual ~IParticleForceGenerator() {}
  };

  class FGParticleGravity : public IParticleForceGenerator
  {
   private:
    Vec3 m_Gravity;

   public:
    FGParticleGravity(const Vec3 &gravity) :
      IParticleForceGenerator(),
      m_Gravity(gravity)
    {
    }

    void updateForce(Particle *particle, real duration) override
    {
      (void)duration;

      if (particle->hasFiniteMass())
      {
        particle->addForce(this->m_Gravity * particle->mass());
      }
    }
  };

  class FGParticleDrag : public IParticleForceGenerator
  {
   private:
    /* first:  Velocity Drag Coefficent */
    /* second: Velocity Squared Drag Coefficent */
    Pair<real, real> m_Coeffs;

   public:
    FGParticleDrag(real k1, real k2) :
      IParticleForceGenerator(),
      m_Coeffs(k1, k2)
    {
    }

    void updateForce(Particle *particle, real duration) override
    {
      (void)duration;

      Vec3 force(particle->m_Velocity.clone());

      real dragCoeff = force.length();
      dragCoeff      = m_Coeffs.first * dragCoeff + m_Coeffs.second * dragCoeff * dragCoeff;

      force.normalize();
      force *= -dragCoeff;

      particle->addForce(force);
    }
  };

  class FGParticleSpring : public IParticleForceGenerator
  {
   private:
    Particle *m_Other;
    real      m_SpringConstant;
    real      m_RestLength;

   public:
    FGParticleSpring(Particle *other, real sprintKonst, real length) :
      IParticleForceGenerator(),
      m_Other(other),
      m_SpringConstant(sprintKonst),
      m_RestLength(length)
    {
    }

    void updateForce(Particle *particle, real duration) override
    {
      (void)duration;

      Vec3 force(particle->m_Position.clone());
      force -= m_Other->m_Position;

      real magnitude = force.length();
      magnitude      = magnitude - this->m_RestLength;  //abs_real(magnitude - this->m_RestLength);
      magnitude *= this->m_SpringConstant;

      force.normalize();
      force *= -magnitude;

      particle->addForce(force);
    }
  };

  class FGParticleBungee : public IParticleForceGenerator
  {
   private:
    Particle *m_Other;
    real      m_SpringConstant;
    real      m_RestLength;

   public:
    FGParticleBungee(Particle *other, real sprintKonst, real length) :
      IParticleForceGenerator(),
      m_Other(other),
      m_SpringConstant(sprintKonst),
      m_RestLength(length)
    {
    }

    void updateForce(Particle *particle, real duration) override
    {
      (void)duration;

      Vec3 force(particle->m_Position.clone());
      force -= m_Other->m_Position;

      real magnitude = force.length();

      if (magnitude <= this->m_RestLength) return;

      magnitude = this->m_SpringConstant * (this->m_RestLength - magnitude);

      force.normalize();
      force *= -magnitude;

      particle->addForce(force);
    }
  };

  class FGParticleAnchoredSpring : public IParticleForceGenerator
  {
   private:
    Vec3 *m_Anchor;
    real  m_SpringConstant;
    real  m_RestLength;

   public:
    FGParticleAnchoredSpring(Vec3 *anchor, real sprintKonst, real length) :
      IParticleForceGenerator(),
      m_Anchor(anchor),
      m_SpringConstant(sprintKonst),
      m_RestLength(length)
    {
    }

    void updateForce(Particle *particle, real duration) override
    {
      (void)duration;

      Vec3 force(particle->m_Position.clone());
      force -= *this->m_Anchor;

      real magnitude = force.length();
      magnitude      = magnitude - this->m_RestLength;  //abs_real(magnitude - this->m_RestLength);
      magnitude *= this->m_SpringConstant;

      force.normalize();
      force *= -magnitude;

      particle->addForce(force);
    }
  };

  class FGParticleBouyancy : public IParticleForceGenerator
  {
   private:
    real m_MaxDepth;
    real m_Volume;
    real m_WaterHeight;
    real m_LiquidDensity;

   public:
    FGParticleBouyancy(real depth, real volume, real height, real density = 1000.0) :
      IParticleForceGenerator(),
      m_MaxDepth(depth),
      m_Volume(volume),
      m_WaterHeight(height),
      m_LiquidDensity(density)
    {
    }

    void updateForce(Particle *particle, real duration) override
    {
      (void)duration;

      real depth = particle->m_Position.y;

      if (depth >= this->m_WaterHeight + this->m_MaxDepth) return;

      Vec3 force(0, 0, 0);

      if (depth <= this->m_WaterHeight - this->m_MaxDepth)
      {
        force.y = this->m_LiquidDensity * this->m_Volume;
        particle->addForce(force);
        return;
      }

      force.y = m_LiquidDensity * m_Volume *
                (depth - m_MaxDepth - m_WaterHeight) / 2 * m_MaxDepth;

      particle->addForce(force);
    }
  };

  class ParticleForceRegistry
  {
   protected:
    typedef Pair<Particle *, IParticleForceGenerator *> ParticleForcePair;
    typedef std::vector<ParticleForcePair>              Registry;

   protected:
    Registry m_Registry;

   public:
    ParticleForceRegistry()
    {
    }

    void add(Particle *particle, IParticleForceGenerator *forceGen)
    {
      this->m_Registry.emplace_back(particle, forceGen);
    }

    void remove(Particle *particle, IParticleForceGenerator *forceGe)
    {
      uint i(0);

      for (ParticleForcePair &pair : this->m_Registry)
      {
        if (pair.first == particle && pair.second == forceGe)
        {
          Utils::swapAndPopAt(this->m_Registry, i);
          break;
        }

        ++i;
      }
    }

    void clear()
    {
      this->m_Registry.clear();
    }

    void updateForces(real duration)
    {
      for (ParticleForcePair &pair : this->m_Registry)
      {
        pair.second->updateForce(pair.first, duration);
      }
    }
  };
}  // namespace prism

#endif  // FORCEGENERATORS_H