#pragma once

#include "surge/asset/Asset.hpp"

#include <numeric>

namespace surge::entity
{
struct Entity
{
    struct Animation
    {
        Animation(const VkDescriptorPool descriptorPool, const std::vector<asset::Skin>& skins)
            : jointMatricesSSBO { sizeof(core::math::Matrix<4, 4>) *
                                      std::accumulate(skins.begin(), skins.end(), 0L,
                                                      [](const core::Size total, const asset::Skin& skin)
                                                      { return total + skin.joints.size(); }),
                                  descriptorPool }
            , state {
                .active        = true,
                .progress      = {},
                .jointMatrices = {},
            }
        {
        }

        asset::ShaderStorageBufferObject jointMatricesSSBO;

        mutable struct State
        {
            bool                                  active { true };
            float                                 progress { 0 };
            std::vector<core::math::Matrix<4, 4>> jointMatrices;
        } state;
    };


    struct State
    {
        bool                     active { true };
        core::math::Matrix<4, 4> modelMatrix;
    };

    const asset::Asset&            asset;
    core::utils::Tree<asset::Node> nodes;
    VkPipelineLayout               pipelineLayout;
    VkPipeline                     pipeline;
    std::optional<Animation>       animation;
    mutable State                  state;

    Entity(const asset::Asset& asset, const VkPipelineLayout pipelineLayout, const VkPipeline pipeline,
           const core::Index sceneIndex, const core::math::StaticMatrix auto& modelMatrix)
        : asset { asset }
        , nodes { asset.scenes.at(sceneIndex).treenNodes }
        , pipelineLayout { pipelineLayout }
        , pipeline { pipeline }
        , animation { !asset.skins.empty() ?
                          std::optional<entity::Entity::Animation> { std::in_place, asset.descriptorPool,
                                                                     asset.skins } :
                          std::optional<entity::Entity::Animation> {} }
        , state { State {
              .active      = true,
              .modelMatrix = core::math::fullMatrix(modelMatrix),
          } }
    {
    }

    void update(const core::Index animationIndex, const float elapsedTime)
    {
        if (animation)
        {
            const auto& anim     = asset.animations.at(animationIndex);
            auto&       progress = animation->state.progress;
            progress += elapsedTime;
            if (progress > anim.end)
            {
                progress -= anim.end;
            }
            anim.update(nodes, progress);
        }
        nodes.traverse<core::utils::Traversal::depthFirst>(&asset::Node::update, state.modelMatrix);
        if (animation)
        {
            assert(!asset.skins.empty());
            nodes.traverse<core::utils::Traversal::linear>(
                [&](const asset::Node& node)
                {
                    if (node.skinIndex)
                    {
                        assert(animation);
                        const auto& skin          = asset.skins.at(node.skinIndex.value());
                        auto&       jointMatrices = animation->state.jointMatrices;
                        jointMatrices.clear();
                        jointMatrices.reserve(skin.joints.size());
                        const auto inverse = core::math::inverse(node.state.globalMatrix);
                        for (const auto& [jointNodeIndex, inverseBindMatrix] : skin.joints)
                        {
                            jointMatrices.emplace_back(inverse * nodes.get(jointNodeIndex).state.globalMatrix *
                                                       inverseBindMatrix);
                        }

                        memcpy(animation->jointMatricesSSBO.buffer.mapped, jointMatrices.data(),
                               jointMatrices.size() * sizeof(core::math::Matrix<4, 4>));
                    }
                });
        }
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

        if (animation)
        {
            // bind joint matrices ssbo
            constexpr uint32_t jointMatricesSSBOIndex = 2;
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                    jointMatricesSSBOIndex, 1, &animation->jointMatricesSSBO.descriptorSet, 0, nullptr);
        }

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