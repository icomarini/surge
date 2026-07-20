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
        , storage { storage }
        , pipelines {} {
    }

    ~Renderer() {
        for (const auto& [name, pipeline] : pipelines) {
            context.destroy(pipeline.first);
            context.destroy(pipeline.second);
        }
    }

    template<Container T>
    void draw(const VkCommandBuffer commandBuffer, const T& entities) const {
        for (const auto& entity : entities) {
            draw(commandBuffer, entity);
        }
    }

    void draw(const VkCommandBuffer commandBuffer, const Entity& entity) const {
        vkCmdSetLineWidth(commandBuffer, 2.0);
        const auto pipelineLayout = storage.pipelines.get(entity.pipeline).layout();

        // bind pipeline and main camera
        storage.pipelines.apply(entity.pipeline, [&](const Pipeline& pipeline) {
            core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
            vkCmdBindPipeline(commandBuffer, Storage::graphicsBindPoint, pipeline.get());
            if (pipeline.sceneId) {
                const auto         scene              = storage.scenes.at(pipeline.sceneId);
                const auto         sceneDescriptorSet = storage.materials.get(scene.materialId);
                constexpr uint32_t sceneIndex { 0 };
                vkCmdBindDescriptorSets(commandBuffer, Storage::graphicsBindPoint, pipelineLayout, sceneIndex, 1,
                                        &sceneDescriptorSet, 0, nullptr);
            }
        });

        // bind model
        storage.models.apply(entity.model, [&](const asset::Model& model) {
            constexpr VkDeviceSize offset { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        });

        // bind matrix
        const core::overload visitor {
            [&](const auto& m) -> auto { return sizeof(decltype(m)); },
        };
        const auto sizeofPushConstants = std::visit(visitor, storage.matrices.at(entity.matrix));
        vkCmdPushConstants(commandBuffer, pipelineLayout, Storage::shaderStages, 0, sizeofPushConstants,
                           &storage.matrices.at(entity.matrix));

        // bind material
        if (entity.material) {
            storage.materials.apply(entity.material, [&](const VkDescriptorSet& material) {
                constexpr uint32_t materialIndex { 1 };
                vkCmdBindDescriptorSets(commandBuffer, Storage::graphicsBindPoint, pipelineLayout, materialIndex, 1,
                                        &material, 0, nullptr);
            });
        }

        vkCmdDrawIndexed(commandBuffer, storage.models.get(entity.model).indexCount, 1, 0, 0, 0);
    }

    void draw(const VkCommandBuffer commandBuffer, const asset::Line& line) const {
        // bind main camera
        const auto pipelineLayout = storage.pipelines.get(storage.linePipelineId).layout();

        storage.pipelines.apply(storage.linePipelineId, [&](const Pipeline& pipeline) {
            core::Extern::setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);
            vkCmdBindPipeline(commandBuffer, Storage::graphicsBindPoint, pipeline.get());
            constexpr uint32_t sceneIndex { 0 };
            vkCmdBindDescriptorSets(commandBuffer, Storage::graphicsBindPoint, pipelineLayout, sceneIndex, 1,
                                    &storage.sceneDescriptorSet, 0, nullptr);
        });
        vkCmdPushConstants(commandBuffer, pipelineLayout, Storage::shaderStages, 0, sizeof(asset::Line), &line);
        vkCmdDraw(commandBuffer, 2, 1, 0, 0);
    }

    void createPipeline(const std::string& name, const VkPipelineVertexInputStateCreateInfo& vertexInputState,
                        const core::shader::Type shader, const VkDescriptorSetLayout materialDescriptorSetLayout,
                        const std::optional<VkDescriptorSetLayout> jointMatricesDescriptorSetLayout) {
        constexpr VkPushConstantRange nodePushConstantRange { core::createPushConstantRange<asset::Node::PushConstants>(
            Storage::shaderStages) };
        const auto                    sceneDescriptorSetLayout = storage.descriptorPool.layout<SceneLayout>();

        auto& [pipelineLayout, pipeline] = pipelines[name];
        pipelineLayout =
            jointMatricesDescriptorSetLayout.has_value() ?
                core::createPipelineLayout(context, nodePushConstantRange, sceneDescriptorSetLayout,
                                           materialDescriptorSetLayout, jointMatricesDescriptorSetLayout.value()) :
                core::createPipelineLayout(context, nodePushConstantRange, sceneDescriptorSetLayout,
                                           materialDescriptorSetLayout);
        pipeline = core::createGraphicPipeline(context, vertexInputState, pipelineLayout, shader);
    }

    const Storage&                                                 storage;
    std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;
};

}  // namespace surge
