#pragma once

#include "surge/physics/Particle.hpp"
#include "surge/physics/ParticleForceRegistry.hpp"

namespace surge::physics
{
constexpr Acceleration earthGravity { 0, -9.81, 0 };

class Physics
{
public:
    ForceRegistry               registry;
    ParticleGravity             gravity;
    std::vector<Anchor>         anchors;
    std::vector<Particle>       particles;
    std::vector<AnchoredSpring> anchoredSprings;
    std::vector<Spring>         springs;

    Physics(const Acceleration& gravity)
        : gravity { gravity }
    {
        allocate(256);
    }

    void allocate(const std::size_t size)
    {
        anchors.reserve(size);
        particles.reserve(size);
        anchoredSprings.reserve(size);
        springs.reserve(size);
    }

    void clear()
    {
        registry.clear();
        anchors.clear();
        particles.clear();
        anchoredSprings.clear();
        allocate(256);
    }

    physics::Anchor& addAnchor(const Position& position)
    {
        auto& anchor = anchors.emplace_back(Anchor {
            .position = position,
        });
        return anchor;
    }

    physics::Particle& addParticle(const Mass mass, const Position& position, const Velocity& velocity = { 0, 0, 0 },
                                   const Acceleration& acceleration = { 0, 0, 0 })
    {
        auto& particle = particles.emplace_back(Particle {
            .mass             = mass,
            .position         = position,
            .velocity         = velocity,
            .acceleration     = acceleration,
            .damping          = 0.995,
            .accumulatedForce = {},
        });
        registry.add(particle, gravity);
        return particle;
    }

    void addAnchoredSpring(const Anchor& anchor, Particle& particle, const Scalar springConstant,
                           const Scalar restLength, const Scalar damping = 0.05)
    {
        auto& anchoredSpring = anchoredSprings.emplace_back(anchor, particle, springConstant, restLength, damping);
        registry.add(particle, anchoredSpring);
    }

    void addSpring(Particle& first, Particle& second, const Scalar springConstant, const Scalar restLength,
                   const Scalar damping = 0.05)
    {
        // anchoredSprings.emplace_back(anchor, particle, springConstant, restLength, damping);
        auto& spring = springs.emplace_back(first, second, springConstant, restLength, damping);
        registry.add(first, spring);
    }

    void update(const Time duration)
    {
        registry.updateForces(duration);
        for (auto& particle : particles)
        {
            particle.integrate(duration);
        }
    }
};
}  // namespace surge::physics