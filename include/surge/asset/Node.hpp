#pragma once

#include "surge/asset/Mesh.hpp"
#include "surge/core/math/matrices.hpp"

#include <optional>

namespace surge::asset
{
struct Node
{
    struct PushConstants
    {
        core::math::Matrix<4, 4> matrix;
        core::math::Vector<4>    baseColorFactor;
        uint32_t                 vertexStageFlag;
        uint32_t                 fragmentStageFlag;
    };

    struct State
    {
        bool                     active;
        core::PolygonMode        polygonMode;
        uint32_t                 vertexStageFlag;
        uint32_t                 fragmentStageFlag;
        core::math::Vector<3>    translation { 0, 0, 0 };
        core::math::Quaternion<> rotation { 0, 0, 0, 0 };
        core::math::Vector<3>    scale { 1, 1, 1 };
        core::math::Matrix<4, 4> localMatrix {};
        core::math::Matrix<4, 4> globalMatrix {};
    };

    const std::optional<core::Index> meshIndex;
    const std::optional<core::Index> skinIndex;
    mutable State                    state;

    static auto update(Node& node, const core::math::Matrix<4, 4>& parentMatrix)
    {
        node.state.localMatrix = core::math::Translation { node.state.translation } *
                                 core::math::Rotation { node.state.rotation } *
                                 core::math::Scaling { node.state.scale };
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
            core::Context::Extern::setPolygonMode(commandBuffer, core::translate(state.polygonMode));

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
}  // namespace surge::asset