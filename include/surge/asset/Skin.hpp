#pragma once

#include "surge/asset/Node.hpp"
#include "surge/math/matrices.hpp"

#include <string>
#include <vector>

namespace surge::asset
{
// struct Node;

struct Skin
{
    struct Joint
    {
        const Node&        node;
        math::Matrix<4, 4> inverseBindMatrix;
    };


    std::string name;
    Node* const skeleton;
    // std::vector<math::Matrix<4, 4>> inverseBindMatrices;
    std::vector<Joint> joints;
};
}  // namespace surge::asset