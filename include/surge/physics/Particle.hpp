#pragma once

#include "surge/core/math/Vector.hpp"

namespace surge::physics
{

using Time         = float;
using Mass         = float;
using Position     = core::math::Vector<3>;
using Velocity     = core::math::Vector<3>;
using Acceleration = core::math::Vector<3>;
using Force        = core::math::Vector<3>;
using Scalar       = float;

class Anchor
{
public:
    Position position;
};

class Particle
{
public:
    // enum class Type
    // {
    //     anchored,
    //     free
    // };
    // Type         type;
    Mass         mass;
    Position     position;
    Velocity     velocity;
    Acceleration acceleration;
    Scalar       damping;
    Force        accumulatedForce;

    void integrate(const Time duration)
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
        return mass < std::numeric_limits<Mass>::max();
    }

    void addForce(const Force& force)
    {
        accumulatedForce += force;
    }
};
}  // namespace surge::physics