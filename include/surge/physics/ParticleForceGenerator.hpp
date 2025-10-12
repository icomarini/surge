#pragma once

#include "surge/physics/Particle.hpp"

namespace surge::physics
{
class ParticleForceGenerator
{
public:
    virtual void updateForce(Particle& particle, const Time duration) = 0;
};


class ParticleGravity : public ParticleForceGenerator
{
public:
    constexpr ParticleGravity(const Acceleration& gravity)
        : gravity { gravity }
    {
    }


    virtual void updateForce(Particle& particle, const Time /*duration*/)
    {
        if (!particle.finiteMass())
        {
            return;
        }
        particle.addForce(gravity * particle.mass);
    }

    Acceleration gravity;
};

class ParticleSpring : public ParticleForceGenerator
{
    /** The particle at the other end of the spring. */
    const Particle& other;

    /** Holds the sprint constant. */
    Scalar springConstant;

    /** Holds the rest length of the spring. */
    Scalar restLength;
    Scalar damping;

public:
    /** Creates a new spring with the given parameters. */
    ParticleSpring(const Particle& other, const Scalar springConstant, const Scalar restLength, const Scalar damping)
        : other { other }
        , springConstant { springConstant }
        , restLength { restLength }
        , damping { damping }
    {
    }

    /** Applies the spring force to the given particle. */
    virtual void updateForce(Particle& particle, const Time)
    {
        const auto force     = particle.position - other.position;
        const auto intensity = core::math::norm(force);
        if (core::math::equal(intensity, 0.0, 1e-6))
        {
            return;
        }
        const auto magnitude = (restLength - intensity) * springConstant / intensity;
        particle.addForce(magnitude * force - damping * particle.velocity);
    }
};

class Spring : public ParticleForceGenerator
{
public:
    Particle& first;
    Particle& second;

    Scalar springConstant;
    Scalar restLength;
    Scalar damping;


    /** Creates a new spring with the given parameters. */
    Spring(Particle& first, Particle& second, const Scalar springConstant, const Scalar restLength,
           const Scalar damping)
        : first { first }
        , second { second }
        , springConstant { springConstant }
        , restLength { restLength }
        , damping { damping }
    {
    }

    /** Applies the spring force to the given particle. */
    virtual void updateForce(Particle&, const Time)
    {
        const auto distance  = first.position - second.position;
        const auto intensity = core::math::norm(distance);
        if (core::math::equal(intensity, 0.0, 1e-6))
        {
            return;
        }
        const auto magnitude = (restLength - intensity) * springConstant / intensity;
        first.addForce(magnitude * distance - damping * first.velocity);
        second.addForce(-magnitude * distance - damping * second.velocity);
    }
};


class ParticleAnchoredSpring : public ParticleForceGenerator
{
public:
    ParticleAnchoredSpring(const Anchor& anchor, const Scalar springConstant, const Scalar restLength, Scalar damping)
        : anchor { anchor }
        , springConstant { springConstant }
        , restLength { restLength }
        , damping { damping }
    {
    }

    virtual void updateForce(Particle& particle, const Time)
    {
        const auto force     = particle.position - anchor.position;
        const auto intensity = core::math::norm(force);
        if (core::math::equal(intensity, 0.0, 1e-6))
        {
            return;
        }
        const auto magnitude = (restLength - intensity) * springConstant / intensity;
        particle.addForce(magnitude * force - damping * particle.velocity);
    }

    Anchor anchor;
    Scalar springConstant;
    Scalar restLength;
    Scalar damping;
};

class AnchoredSpring : public ParticleForceGenerator
{
public:
    AnchoredSpring(const Anchor& anchor, Particle& particle, const Scalar springConstant, const Scalar restLength,
                   Scalar damping)
        : anchor { anchor }
        , particle { particle }
        , springConstant { springConstant }
        , restLength { restLength }
        , damping { damping }
    {
    }

    virtual void updateForce(Particle& particle, const Time) override
    {
        const auto force     = particle.position - anchor.position;
        const auto intensity = core::math::norm(force);
        if (core::math::equal(intensity, 0.0, 1e-6))
        {
            return;
        }
        const auto magnitude = (restLength - intensity) * springConstant / intensity;
        particle.addForce(magnitude * force - damping * particle.velocity);
    }

    // void update(const Time)
    // {
    //     const auto force     = particle.position - anchor.position;
    //     const auto intensity = math::norm(force);
    //     if (math::equal(intensity, 0.0, 1e-6))
    //     {
    //         return;
    //     }
    //     const auto magnitude = (restLength - intensity) * springConstant / intensity;
    //     particle.addForce(magnitude * force - damping * particle.velocity);
    // }

    const Anchor& anchor;
    Particle&     particle;
    Scalar        springConstant;
    Scalar        restLength;
    Scalar        damping;
};

}  // namespace surge::physics