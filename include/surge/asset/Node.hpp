#pragma once

#include "surge/Defaults.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/math/angles.hpp"
// #include "glm/gtx/quaternion.hpp"

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
    };

    std::string             name;
    Node* const             parent;
    std::vector<Node>       children;
    const Mesh*             mesh;
    std::optional<uint32_t> skinIndex;
    mutable State           state;


    math::Matrix<4, 4> matrix() const
    {
        const auto               sintheta { std::sin(math::deg2rad(180.0f)) };
        const auto               costheta { std::cos(math::deg2rad(180.0f)) };
        const math::Matrix<4, 4> correction {
            costheta,  0, sintheta, 0,  //
            0,         1, 0,        0,  //
            -sintheta, 0, costheta, 0,  //
            0,         0, 0,        1,  //
        };

        // return translation * rotation * scaling;
        // const math::Vector<3> translation {
        //     -math::get<0>(state.translation),
        //     math::get<1>(state.translation),
        //     math::get<2>(state.translation),
        // };
        // return math::transpose(math::Translation { translation } * math::Rotation { state.rotation } *
        //                        math::Scaling { state.scale } * correction);
        return math::Translation { state.translation } * math::Rotation { state.rotation } *
               math::Scaling { state.scale };
    }

    math::Matrix<4, 4> nodeMatrix() const
    {
        auto  nodeMatrix    = matrix();
        auto* currentParent = parent;
        while (currentParent)
        {
            nodeMatrix    = currentParent->matrix() * nodeMatrix;
            currentParent = currentParent->parent;
        }
        return nodeMatrix;
    }


    // template<typename Camera>
    // void update(const Camera& camera, const UserInteraction& ui) const
    // {
    // }

private:
};
}  // namespace surge::asset