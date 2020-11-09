#include "rigidbodyforcegenerators.h"

#include "../utils.h"

namespace prism
{
    // class RigidBodyForceRegistry

    RigidBodyForceRegistry::RigidBodyForceRegistry()
    {
    }

    void RigidBodyForceRegistry::add(RigidBody *body, IRigidBodyForceGenerator *forceGen)
    {
        this->m_Registry.emplace_back(body, forceGen);
    }

    void RigidBodyForceRegistry::remove(RigidBody *body, IRigidBodyForceGenerator *forceGen)
    {
        auto i(0);

        for (RigidBodyForcePair &pair : this->m_Registry)
        {
            if (pair.first == body && pair.second == forceGen)
    {
                Utils::swapAndPopAt(this->m_Registry, i);
                break;
            }

            ++i;
        }
    }

    void RigidBodyForceRegistry::updateForces(real duration)
    {
        for (RigidBodyForcePair &pair : this->m_Registry)
        {
            pair.second->updateForce(pair.first, duration);
        }
    }

    void RigidBodyForceRegistry::clear()
    {
        this->m_Registry.clear();
    }

    RigidBodyForceRegistry::~RigidBodyForceRegistry()
    {
        this->clear();
    }
}