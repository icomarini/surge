#pragma once

#include "surge/Defaults.hpp"
#include "surge/asset/Mesh.hpp"
#include "surge/asset/GltfAsset.hpp"
// #include "surge/asset/Skin.hpp"
#include "surge/math/angles.hpp"
#include "glm/gtx/quaternion.hpp"

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

    std::string name;
    Node* const parent;
    // uint32_t          index;
    std::vector<Node> children;
    const Mesh*       mesh;
    // const uint32_t          skinId;

    std::optional<uint32_t>           skinIndex;
    std::optional<math::Matrix<4, 4>> inverseBindMatrix;

    mutable State state;


    Node(Node* const parent, const std::vector<Mesh>& meshes, const GltfAsset& gltf, const Size nodeId,
         std::vector<Node*>& nodesLut)
        : name { baptize<This::node>(gltf.node(nodeId).name, nodeId) }
        , parent { parent }
        , mesh { gltf.node(nodeId).meshIndex ? &meshes.at(gltf.node(nodeId).meshIndex.value()) : nullptr }
        // , skin { nullptr }
        , skinIndex { gltf.node(nodeId).skinIndex ?
                          std::optional<uint32_t> { static_cast<uint32_t>(gltf.node(nodeId).skinIndex.value()) } :
                          std::optional<uint32_t> {} }
        , inverseBindMatrix { /*math::identity<4>*/ }
        , state { createState(gltf.node(nodeId)) }
    {
        assert(nodesLut[nodeId] == nullptr);
        nodesLut[nodeId] = this;

        children.reserve(gltf.node(nodeId).children.size());
        for (const auto& childId : gltf.node(nodeId).children)
        {
            children.emplace_back(this, meshes, gltf, childId, nodesLut);
        }
    }

    Node(const Mesh& mesh)
        : name { baptize<This::node>(0) }
        , parent { nullptr }
        , mesh { &mesh }  // , skin { nullptr }
        , skinIndex { -1 }
        , state { State {
              .active            = false,
              .polygonMode       = PolygonMode::fill,
              .vertexStageFlag   = 0,
              .fragmentStageFlag = 0,
              .translation       = { 0, 0, 0 },
              .scale             = { 1, 1, 1 },
          } }
    {
    }

    math::Matrix<4, 4> matrix() const
    {
        constexpr auto               sintheta { std::sin(math::deg2rad(180.0)) };
        constexpr auto               costheta { std::cos(math::deg2rad(180.0)) };
        constexpr math::Matrix<4, 4> correction {
            costheta,  0, sintheta, 0,  //
            0,         1, 0,        0,  //
            -sintheta, 0, costheta, 0,  //
            0,         0, 0,        1,  //
        };

        // return translation * rotation * scaling;
        const math::Vector<3> translation {
            -math::get<0>(state.translation),
            math::get<1>(state.translation),
            math::get<2>(state.translation),
        };
        return math::transpose(math::Translation { translation } * math::Rotation { state.rotation } *
                               math::Scaling { state.scale } * correction);
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


    template<typename Camera>
    void update(const Camera& camera, const UserInteraction& ui) const
    {
        // if (mesh)
        // {
        //     for (const auto& child : children)
        //     {
        //         child.update(camera, ui);
        //     }
        // }
    }

private:
    State createState(const fastgltf::Node& node)
    {
        // State state { true, PolygonMode::fill, 0, 0 };

        // const fastgltf::visitor visitor {
        //     // [](const auto&)
        //     // {
        //     //     throw std::runtime_error("unsupported visitor");
        //     //     return {};
        //     // },
        //     // [&](const fastgltf::Node::TransformMatrix& matrix)
        //     // {
        //     //     math::Matrix<4, 4> m;
        //     //     memcpy(m.data(), matrix.data(), sizeof(matrix));
        //     //     state.transformation = math::Transformation { m };
        //     // },
        //     [&](const fastgltf::TRS& trs)
        //     {
        //         // state.scale       = { trs.scale[0], trs.scale[1], trs.scale[2] };
        //         // state.translation = { trs.translation[0], trs.translation[1], trs.translation[2] };
        //         // state.rotation    = { trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3] };
        //         // state.attitude    = math::toEulerAngles(state.rotation);
        //         // state.matrix      = math::Scaling { state.scale } * math::Rotation { state.rotation } *
        //         //                math::Translation { state.translation };
        //         // state.transformation = math::Transformation { state.scale, state.translation, state.rotation };

        //         return std::make_tuple(math::Vector<3> { trs.scale[0], trs.scale[1], trs.scale[2] },
        //                                math::Vector<3> { trs.translation[0], trs.translation[1], trs.translation[2]
        //                                }, math::toEulerAngles(math::Quaternion { trs.rotation[0], trs.rotation[1],
        //                                                                       trs.rotation[2], trs.rotation[3] }));
        //     },
        // };
        assert(std::holds_alternative<fastgltf::TRS>(node.transform));
        const auto& trs = std::get<fastgltf::TRS>(node.transform);
        // const auto& [scale, translation, attitude] = std::visit(visitor, node.transform);
        return State {
            .active            = true,
            .polygonMode       = PolygonMode::fill,
            .vertexStageFlag   = 0,
            .fragmentStageFlag = 0,
            .translation       = math::Vector<3> { trs.translation[0], trs.translation[1], trs.translation[2] },
            .rotation          = { trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3] },
            .scale             = math::Vector<3> { trs.scale[0], trs.scale[1], trs.scale[2] },
            // .attitude          = math::toEulerAngles(
            // math::Quaternion<> { trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3] }),
        };
    }
};
}  // namespace surge::asset