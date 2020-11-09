#ifndef COLLISIONCOARSE_H
#define COLLISIONCOARSE_H

#include "../math/vector3.h"
#include "bifrost_prismtypes.h"

namespace prism
{
  class BoundingSphere
  {
   private:
    Vec3 m_Center;
    real m_Radius;

   public:
    BoundingSphere(const Vec3 &center, real radius);
    BoundingSphere(const BoundingSphere &one, const BoundingSphere &two);

    Vec3 center() const { return this->m_Center; }
    void setCenter(const Vec3 &value) { this->m_Center = value; }

    real radius() const { return this->m_Radius; }
    void setRadius(const real value) { this->m_Radius = value; }

    bool overlaps(const BoundingSphere *other) const;
    real getGrowth(const BoundingSphere &other) const;

    real getSize() const;
  };

  class PotentialContact
  {
   public:
    RigidBody *body[2];

    PotentialContact(RigidBody *one = nullptr, RigidBody *two = nullptr);

    ~PotentialContact();
  };

  // BoundingVolumeClass interface:
  //  > BoundingVolumeClass(const BoundingVolumeClass &one, const BoundingVolumeClass &two);
  //  > bool overlaps(const BoundingVolumeClass *other) const;
  //  > real getSize() const;                                     // Volume
  //  > real getGrowth(const BoundingVolumeClass &other) const;

  template<class BoundingVolumeClass>
  class BVHNode
  {
   private:
    BVHNode *           m_Parent;
    BVHNode *           m_Children[2];
    BoundingVolumeClass m_Volume;
    RigidBody *         m_Body;

   public:
    BVHNode(BVHNode *parent, const BoundingVolumeClass &volume, RigidBody *body = nullptr) :
      m_Parent(parent),
      m_Children{nullptr, nullptr},
      m_Volume(volume),
      m_Body(body)
    {
    }

    bool isLeaf() const
    {
      return (this->m_Body != nullptr);
    }

    bool overlaps(const BVHNode<BoundingVolumeClass> *other)
    {
      return this->m_Volume->overlaps(other->m_Volume);
    }

    uint getPotentialContactsWith(const BVHNode<BoundingVolumeClass> *other, PotentialContact *contacts, uint limit) const
    {
      if (!overlaps(other) || limit == 0) return 0;

      if (isLeaf() && other->isLeaf())
      {
        contacts->body[0] = m_Body;
        contacts->body[1] = other->m_Body;

        return 1;
      }

      if (other->isLeaf() || (!isLeaf() && m_Volume->getSize() >= other->m_Volume->getSize()))
      {
        uint count = m_Children[0]->getPotentialContactsWith(other, contacts, limit);

        if (limit > count)
        {
          return count + m_Children[0]->getPotentialContactsWith(other, contacts + count, limit - count);
        }
        else
        {
          return count;
        }
      }
      else
      {
        uint count = m_Children[0]->getPotentialContactsWith(other->m_Children[0], contacts, limit);

        if (limit > count)
        {
          return count + getPotentialContactsWith(other->m_Children[1], contacts + count, limit - count);
        }
        else
        {
          return count;
        }
      }
    }

    void recalculateboundingVolume(bool recurse = true)
    {
      (void)recurse;

      if (isLeaf()) return;

      m_Volume = BoundingVolumeClass(m_Children[0]->m_Volume, m_Children[1]->m_Volume);

      if (m_Parent)
      {
        m_Parent->recalculateboundingVolume(true);
      }
    }

    uint getPotentialContacts(PotentialContact *contacts, uint limit) const
    {
      if (isLeaf() || limit == 0) return 0;

      return this->m_Children[0]->getPotentialContactsWith(this->m_Children[1], contacts, limit);
    }

    void insert(RigidBody *newBody, BoundingVolumeClass &newVolume)
    {
      if (isLeaf())
      {
        m_Children[0] = new BVHNode<BoundingVolumeClass>(this, m_Volume, m_Body);
        m_Children[1] = new BVHNode<BoundingVolumeClass>(this, newVolume, newBody);

        this->m_Body = nullptr;

        this->recalculateboundingVolume();
      }
      else
      {
        if (m_Children[0]->m_Volume.getGrowth(newVolume) < m_Children[1]->m_Volume.getGrowth(newVolume))
        {
          m_Children[0]->insert(newBody, newVolume);
        }
        else
        {
          m_Children[1]->insert(newBody, newVolume);
        }
      }
    }

    ~BVHNode()
    {
      if (m_Parent)
      {
        auto sibling = (m_Parent->m_Children[0] == this) ? this->m_Parent->m_Children[1] : this->m_Parent->m_Children[0];

        this->m_Parent->m_Volume      = sibling->m_Volume;
        this->m_Parent->m_Body        = sibling->m_Body;
        this->m_Parent->m_Children[0] = sibling->m_Children[0];
        this->m_Parent->m_Children[1] = sibling->m_Children[1];

        sibling->m_Parent      = nullptr;
        sibling->m_Body        = nullptr;
        sibling->m_Children[0] = nullptr;
        sibling->m_Children[1] = nullptr;

        delete sibling;

        this->m_Parent->recalculateboundingVolume();
      }

      if (m_Children[0])
      {
        m_Children[0]->m_Parent = nullptr;
        delete m_Children[0];
      }

      if (m_Children[1])
      {
        m_Children[1]->m_Parent = nullptr;
        delete m_Children[1];
      }
    }
  };

  using BVHSphereNode = BVHNode<BoundingSphere>;
}  // namespace prism

#endif  // COLLISIONCOARSE_H