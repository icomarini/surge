#pragma once

#include "surge/math/angles.hpp"
#include "surge/math/matrices.hpp"
#include "surge/math/Vector.hpp"

#include <optional>

namespace surge::entity
{
struct Node
{
    struct State
    {
        bool               active;
        PolygonMode        polygonMode;
        uint32_t           vertexStageFlag;
        uint32_t           fragmentStageFlag;
        math::Vector<3>    translation { 0, 0, 0 };
        math::Quaternion<> rotation { 0, 0, 0, 0 };
        math::Vector<3>    scale { 1, 1, 1 };
        math::Matrix<4, 4> localMatrix {};
        math::Matrix<4, 4> globalMatrix {};
    };

    const std::optional<Index> meshIndex;
    const std::optional<Index> skinIndex;
    mutable State              state;

    static auto update(Node& node, const math::Matrix<4, 4>& parentMatrix)
    {
        node.state.localMatrix = math::Translation { node.state.translation } * math::Rotation { node.state.rotation } *
                                 math::Scaling { node.state.scale };
        node.state.globalMatrix = parentMatrix * node.state.localMatrix;
        return node.state.globalMatrix;
    }
};
}  // namespace surge::entity