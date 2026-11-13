#pragma once

#include "surge/asset/Mesh.hpp"
#include "surge/core/math/matrices.hpp"

#include <optional>

namespace surge::asset {
struct Node {
    MeshID                   meshId;
    SkinID                   skinId;
    core::math::Vector<3>    translation;
    core::math::Quaternion<> rotation;
    core::math::Vector<3>    scale;
    core::math::Matrix<4, 4> transformation;
};
}  // namespace surge::asset