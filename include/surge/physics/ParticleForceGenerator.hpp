#pragma once

#include "surge/physics/Particle.hpp"

namespace surge::physics
{
class ParticleForceGenerator
{
public:
    virtual void updateForce(Particle& particle, const float duration) = 0;
};


class ParticleGravity : public ParticleForceGenerator
{
public:
    constexpr ParticleGravity(const math::Vector<3>& gravity)
        : gravity { gravity }
    {
    }

    virtual void updateForce(Particle& particle, const float /*duration*/)
    {
        if (!particle.finiteMass())
        {
            return;
        }
        particle.addForce(gravity * particle.mass);
    }

    math::Vector<3> gravity;
};

class ParticleSpring : public ParticleForceGenerator
{
    /** The particle at the other end of the spring. */
    Particle* other;

    /** Holds the sprint constant. */
    float springConstant;

    /** Holds the rest length of the spring. */
    float restLength;

public:
    /** Creates a new spring with the given parameters. */
    ParticleSpring(Particle* other, float springConstant, float restLength)
        : other { other }
        , springConstant { springConstant }
        , restLength { restLength }
    {
    }

    /** Applies the spring force to the given particle. */
    virtual void updateForce(Particle& particle, float)
    {
        // Calculate the vector of the spring
        // Vector3 force;
        // particle->getPosition(&force);
        // force -= other->getPosition();
        const auto force          = particle.position - other->position;
        const auto forceIntensity = math::norm(force);

        // Calculate the magnitude of the force
        // real magnitude = force.magnitude();
        // magnitude      = real_abs(magnitude - restLength);
        // magnitude *= springConstant;
        const auto magnitude = springConstant * std::abs(forceIntensity - restLength);

        // Calculate the final force and apply it
        // force.normalise();
        // force *= -magnitude;
        particle.addForce((magnitude / forceIntensity) * force);
    }
};


class ParticleAnchoredSpring : public ParticleForceGenerator
{
public:
    ParticleAnchoredSpring(const math::Vector<3>& anchor, const float springConstant, const float restLength,
                           float damping)
        : anchor { anchor }
        , springConstant { springConstant }
        , restLength { restLength }
        , damping { damping }
    {
    }

    virtual void updateForce(Particle& particle, const float)
    {
        const auto force = particle.position - anchor;
        const auto norm  = math::norm(force);
        if (math::equal(norm, 0.0, 1e-6))
        {
            return;
        }
        const auto magnitude = (restLength - norm) * springConstant / norm;
        particle.addForce(magnitude * force - damping * particle.velocity);
    }

    math::Vector<3> anchor;
    float           springConstant;
    float           restLength;
    float           damping;
};

}  // namespace surge::physics