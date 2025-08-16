#pragma once

#include "surge/physics/Particle.hpp"

namespace surge::physics
{
class ParticleForceGenerator
{
public:
    /**
     * Overload this in implementations of the interface to calculate
     * and update the force applied to the given particle.
     */
    virtual void updateForce(Particle& particle, const float duration) = 0;
};


class ParticleGravity : public ParticleForceGenerator
{
    /** Holds the acceleration due to gravity. */
    math::Vector<3> gravity;

public:
    /** Creates the generator with the given acceleration. */
    constexpr ParticleGravity(const math::Vector<3>& gravity)
        : gravity { gravity }
    {
    }

    /** Applies the gravitational force to the given particle. */
    virtual void updateForce(Particle& particle, const float /*duration*/)
    {
        // Check that we do not have infinite mass
        if (!particle.finiteMass())
        {
            return;
        }

        // Apply the mass-scaled force to the particle
        particle.addForce(gravity * particle.mass);
    }
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
    virtual void updateForce(Particle& particle, float duration)
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
    /** The location of the anchored end of the spring. */
    math::Vector<3> anchor;

    /** Holds the sprint constant. */
    float springConstant;

    /** Holds the rest length of the spring. */
    float restLength;

    float damping;

public:
    // ParticleAnchoredSpring();

    /** Creates a new spring with the given parameters. */
    ParticleAnchoredSpring(const math::Vector<3>& anchor, const float springConstant, const float restLength,
                           float damping)
        : anchor { anchor }
        , springConstant { springConstant }
        , restLength { restLength }
        , damping { damping }
    {
    }

    /** Retrieve the anchor point. */
    // const Vector3* getAnchor() const
    // {
    //     return anchor;
    // }

    /** Set the spring's properties. */
    // void init(Vector3* anchor, real springConstant, real restLength);

    /** Applies the spring force to the given particle. */
    virtual void updateForce(Particle& particle, const float duration)
    {
        // Calculate the vector of the spring
        // Vector3 force;
        // particle->getPosition(&force);
        // force -= *anchor;
        const auto force = particle.position - anchor;

        // Calculate the magnitude of the force
        // const auto magnitude = math::norm(force);
        const auto magnitude = (restLength - math::norm(force)) * springConstant;

        // Calculate the final force and apply it
        // force.normalise();
        // force *= magnitude;
        particle.addForce(magnitude * math::normalize(force) - damping * particle.velocity);
    }
};

}  // namespace surge::physics