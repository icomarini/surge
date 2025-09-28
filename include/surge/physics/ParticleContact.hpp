#pragma once

#include "surge/physics/Particle.hpp"

namespace surge::physics
{

struct ParticleContact
{
    Particle&       particleA;
    Particle*       particleB;
    float           restitution;
    math::Vector<3> contactNormal;
};
}  // namespace surge::physics