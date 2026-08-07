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

    // ~Renderer() {
    // }

    void draw(const VkCommandBuffer commandBuffer, const Scene& scene) {
        for (const auto& entity : scene.entities) {
            draw(commandBuffer, entity, scene.materialId);
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
                const auto&        scene              = storage.scenes.at(pipeline.sceneId);
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

    void draw(const VkCommandBuffer commandBuffer, const Entity2& entity) {
        // bind pipeline
        // const auto pipelineId     = pipelines.at(ShaderType::primitiveTexturedNormal);
        const auto pipelineLayout = storage.pipelines.get(entity.pipelineId).layout();

        // bind pipeline and main camera
        storage.pipelines.apply(entity.pipelineId, [&](const Pipeline& pipeline) {
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
        storage.models.apply(entity.modelId, [&](const asset::Model& model) {
            constexpr VkDeviceSize offset { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        });

        // traverse nodes
        const auto& nodeTree = storage.nodeTrees.at(entity.nodeTreeId);
        nodeTree.traverse<core::utils::Traversal::linear>([&](const asset::Node2& node) {
            if (node.meshId) {
                const auto& mesh = storage.meshes.at(node.meshId);
                for (const auto& primitive : mesh.primitives) {
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

    void draw(const VkCommandBuffer commandBuffer, const Entity2& entity, const MaterialID sceneMaterialId) {
        // bind pipeline
        const auto pipelineLayout = storage.pipelines.get(entity.pipelineId).layout();

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

        // traverse nodes
        const auto& nodeTree = storage.nodeTrees.at(entity.nodeTreeId);
        nodeTree.traverse<core::utils::Traversal::linear>([&](const asset::Node2& node) {
            if (node.meshId) {
                const auto& mesh = storage.meshes.at(node.meshId);
                for (const auto& primitive : mesh.primitives) {
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
        const auto pipelineLayout = storage.pipelines.get(storage.linePipelineId).layout();

        storage.pipelines.apply(storage.linePipelineId, [&](const Pipeline& pipeline) {
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
