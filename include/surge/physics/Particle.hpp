#pragma once

#include "surge/math/Vector.hpp"

namespace surge::physics
{


class Particle
{
public:
    float           mass;
    math::Vector<3> position;
    math::Vector<3> velocity;
    math::Vector<3> acceleration;
    float           damping;
    math::Vector<3> accumulatedForce;

    void integrate(float duration)
    {
        assert(duration > 0.0);


        velocity += duration * (acceleration + (accumulatedForce / mass));
        position += duration * velocity;
        // velocity *= std::pow(damping, duration);

        accumulatedForce = {};
        // Impose drag.
        // velocity *= real_pow(damping, duration);
    }

    bool finiteMass() const
    {
        return mass < std::numeric_limits<float>::max();
    }

    void addForce(const math::Vector<3>& force)
    {
        accumulatedForce += force;
    }
};
}  // namespace surge::physics