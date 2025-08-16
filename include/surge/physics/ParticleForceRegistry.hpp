#pragma once

#include "surge/physics/ParticleForceGenerator.hpp"

#include <vector>

namespace surge::physics
{
class ParticleForceRegistry
{
protected:
    /**
     * Keeps track of one force generator and the particle it
     * applies to.
     */
    struct ParticleForceRegistration
    {
        Particle&               particle;
        ParticleForceGenerator& fg;
    };

    /**
     * Holds the list of registrations.
     */
    using Registry = std::vector<ParticleForceRegistration>;
    Registry registrations;

public:
    /**
     * Registers the given force generator to apply to the
     * given particle.
     */
    void add(Particle& particle, ParticleForceGenerator& forceGenerator)
    {
        registrations.push_back(ParticleForceRegistration { particle, forceGenerator });
    }

    /**
     * Removes the given registered pair from the registry.
     * If the pair is not registered, this method will have
     * no effect.
     */
    // void remove(Particle* particle, ParticleForceGenerator* fg);

    /**
     * Clears all registrations from the registry. This will
     * not delete the particles or the force generators
     * themselves, just the records of their connection.
     */
    // void clear();

    /**
     * Calls all the force generators to update the forces of
     * their corresponding particles.
     */
    void updateForces(const float duration) const
    {
        for (auto& [particle, forceGenerator] : registrations)
        {
            forceGenerator.updateForce(particle, duration);
        }
    }
};
}  // namespace surge::physics