#pragma once

#include "surge/math/angles.hpp"
#include "surge/math/matrices.hpp"
#include "surge/math/Vector.hpp"
#include "surge/asset/Mesh.hpp"

#include <optional>

namespace surge::entity
{
struct Node
{
    struct PushConstants
    {
        math::Matrix<4, 4> matrix;
        math::Vector<4>    baseColorFactor;
        uint32_t           vertexStageFlag;
        uint32_t           fragmentStageFlag;
    };

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

    void draw(const VkCommandBuffer commandBuffer, const asset::Mesh& mesh, const VkPipelineLayout pipelineLayout) const
    {
        if (!state.active)
        {
            return;
        }

        for (const auto& primitive : mesh.primitives)
        {
            auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
                vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
            assert(setPolygonMode);
            setPolygonMode(commandBuffer, translate(state.polygonMode));

            // bind material
            constexpr uint32_t materialIndex = 1;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, materialIndex, 1,
                                    &primitive.material.descriptorSet, 0, nullptr);

            const PushConstants pushConstants {
                .matrix            = state.globalMatrix,
                .baseColorFactor   = primitive.material.baseColorFactor,
                .vertexStageFlag   = state.vertexStageFlag,
                .fragmentStageFlag = state.fragmentStageFlag,
            };
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, sizeof(PushConstants), &pushConstants);

            vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
        }
    }
};
}  // namespace surge::entity