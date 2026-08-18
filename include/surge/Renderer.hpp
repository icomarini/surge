#pragma once

#include "surge/Storage.hpp"
#include "surge/Camera.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/core/Pipeline.hpp"

#include "surge/core/Descriptor.hpp"

namespace surge {

class Renderer : public core::Contextualized {
public:
    Renderer(const Storage& storage)
        : Contextualized { storage.command.context }
        , storage { storage } {
    }

    void draw(const VkCommandBuffer commandBuffer, const Scene& scene) {
        for (const auto& entity : scene.entities) {
            draw(commandBuffer, entity, scene.materialId);
        }
    }

    void draw(const VkCommandBuffer commandBuffer, const Entity& entity, const MaterialID sceneMaterialId) {
        // bind pipeline
        const auto pipelineLayout = storage.pipelines.get(entity.pipelineId).layout();
        // const auto& pipeline = storage.getPipeline()

        // bind pipeline and main camera
        storage.pipelines.apply(entity.pipelineId, [&](const Pipeline& pipeline) {
            core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
            vkCmdSetLineWidth(commandBuffer, 2.0);
            vkCmdBindPipeline(commandBuffer, Storage::graphicsBindPoint, pipeline.get());
            if (sceneMaterialId) {
                const auto         sceneDescriptorSet = storage.materials.get(sceneMaterialId);
                constexpr uint32_t sceneIndex { 0 };
                vkCmdBindDescriptorSets(commandBuffer, Storage::graphicsBindPoint, pipelineLayout, sceneIndex, 1,
                                        &sceneDescriptorSet, 0, nullptr);
            }
        });

        // bind model
        storage.models.apply(entity.modelId, [&](const asset::Model& model) {
            constexpr VkDeviceSize offset { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        });

        // bind animation
        if (entity.animationChannelId) {
            const auto& animationChannel = storage.animationChannels.at(entity.animationChannelId);
            storage.materials.apply(animationChannel.jointMatricesMaterialId, [&](const VkDescriptorSet& material) {
                constexpr uint32_t jointMatricesIndex { 2 };
                vkCmdBindDescriptorSets(commandBuffer, Storage::graphicsBindPoint, pipelineLayout, jointMatricesIndex,
                                        1, &material, 0, nullptr);
            });
        }

        // traverse nodes
        const auto& nodeTree = storage.nodeTrees.at(entity.nodeTreeId);
        nodeTree.traverse<core::utils::Traversal::linear>([&](const asset::Node2& node) {
            if (node.meshId) {
                for (const auto& primitive : storage.meshes.at(node.meshId).primitives) {
                    // bind material
                    if (primitive.materialId) {
                        storage.materials.apply(primitive.materialId, [&](const VkDescriptorSet& material) {
                            constexpr uint32_t materialIndex { 1 };
                            vkCmdBindDescriptorSets(commandBuffer, Storage::graphicsBindPoint, pipelineLayout,
                                                    materialIndex, 1, &material, 0, nullptr);
                        });
                    }

                    // push constants
                    vkCmdPushConstants(commandBuffer, pipelineLayout, Storage::shaderStages, 0, sizeof(ModelMatrix),
                                       &node.transformation);

                    // draw
                    vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
                }
            }
        });
    }

    void draw(const VkCommandBuffer commandBuffer, const Scene& scene, const asset::Line& line) const {
        // bind main camera
        const auto linePipelineId = storage.pipelineIds.at(core::shader::Type::line);
        const auto pipelineLayout = storage.pipelines.get(linePipelineId).layout();

        storage.pipelines.apply(linePipelineId, [&](const Pipeline& pipeline) {
            core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
            vkCmdBindPipeline(commandBuffer, Storage::graphicsBindPoint, pipeline.get());
            constexpr uint32_t sceneIndex { 0 };
            vkCmdBindDescriptorSets(commandBuffer, Storage::graphicsBindPoint, pipelineLayout, sceneIndex, 1,
                                    &storage.materials.get(scene.materialId), 0, nullptr);
        });
        vkCmdPushConstants(commandBuffer, pipelineLayout, Storage::shaderStages, 0, sizeof(asset::Line), &line);
        vkCmdDraw(commandBuffer, 2, 1, 0, 0);
    }

    const Storage& storage;
};

}  // namespace surge
