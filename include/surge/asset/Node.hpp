#pragma once

#include "surge/Defaults.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/math/angles.hpp"

namespace surge::asset
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

    std::string             name;
    Node* const             parent;
    std::vector<Node>       children;
    const Mesh*             mesh;
    std::optional<uint32_t> skinIndex;
    mutable State           state;

    void updateMatrices(const math::StaticMatrix auto& parentMatrix) const
    {
        state.localMatrix =
            math::Translation { state.translation } * math::Rotation { state.rotation } * math::Scaling { state.scale };
        state.globalMatrix = parentMatrix * state.localMatrix;
        for (const auto& child : children)
        {
            child.updateMatrices(state.globalMatrix);
        }
    }
};
}  // namespace surge::asset
