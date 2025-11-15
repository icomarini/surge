#pragma once

#include "surge/asset/Texture.hpp"
#include "surge/asset/Model.hpp"
#include "surge/core/geometry/shapes.hpp"
#include "surge/core/input/input.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/load/LoadedTexture.hpp"
#include "surge/load/Defaults.hpp"

namespace surge::entity
{

class Skybox
{
public:
    struct State
    {
        bool                     active { true };
        core::math::Matrix<4, 4> modelMatrix;
    };

    const asset::Asset&            asset;
    core::utils::Tree<asset::Node> nodes;
    VkPipelineLayout               pipelineLayout;
    VkPipeline                     pipeline;
    mutable State                  state;

    Skybox(const asset::Asset& asset, const VkPipelineLayout pipelineLayout, const VkPipeline pipeline,
           const core::math::StaticMatrix auto& modelMatrix)
        : asset { asset }
        , nodes { asset.mainScene().treenNodes }
        , pipelineLayout { pipelineLayout }
        , pipeline { pipeline }
        , state { State {
              .active      = true,
              .modelMatrix = core::math::fullMatrix(modelMatrix),
          } }
    {
    }

    template<typename Camera>
    void update(const Camera& camera)
    {
        state.modelMatrix = camera.mats.perspective * camera.mats.view;
        nodes.traverse<core::utils::Traversal::depthFirst>(&asset::Node::update, state.modelMatrix);
    }

    void draw(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    {
        if (!state.active)
        {
            return;
        }

        // bind model
        constexpr VkDeviceSize offset { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &asset.model.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, asset.model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        // bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        // bind scene uniform
        constexpr uint32_t sceneUniformIndex = 0;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, sceneUniformIndex, 1,
                                &sceneDescriptor, 0, nullptr);

        nodes.traverse<core::utils::Traversal::linear>(
            [&](const asset::Node& node)
            {
                if (node.meshIndex)
                {
                    node.draw(commandBuffer, asset.meshes.at(node.meshIndex.value()), pipelineLayout);
                }
            });
    }
};

}  // namespace surge::entity
