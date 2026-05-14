#pragma once

#include "surge/core/math/Vector.hpp"

namespace surge::asset {

struct Point {
    alignas(16) core::math::Vector<3> p;
    alignas(16) core::math::Vector<4> color;
};

struct Line {
    alignas(16) core::math::Vector<3> a;
    alignas(16) core::math::Vector<3> b;
    alignas(16) core::math::Vector<4> color;
};

}  // namespace surge::asset