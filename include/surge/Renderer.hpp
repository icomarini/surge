#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Camera.hpp"
#include "surge/asset/Asset.hpp"
#include "surge/Pipeline.hpp"

#include "surge/geometry/shapes.hpp"

namespace surge
{

VkPolygonMode translate(const PolygonMode polygonMode);
VkPolygonMode translate(const PolygonMode polygonMode)
{
    switch (polygonMode)
    {
    case PolygonMode::point:
        return VK_POLYGON_MODE_POINT;
    case PolygonMode::line:
        return VK_POLYGON_MODE_LINE;
    case PolygonMode::fill:
        return VK_POLYGON_MODE_FILL;
    default:
        throw;
    }
}

class Renderer
{
public:
    // enum class Topology
    // {
    //     vertexList,
    //     lineList,
    // };

    // struct PipelineID
    // {
    //     PolygonMode polygonMode;
    //     Topology    topology;
    // };


    // struct compare
    // {
    //     bool operator()(PipelineID lhs, PipelineID rhs) const
    //     {
    //         return lhs.polygonMode == rhs.polygonMode ? lhs.topology < rhs.topology : lhs.polygonMode <
    //         rhs.polygonMode;
    //     }
    // };

    // using Pipelines = std::map<PipelineID, VkPipeline, compare>;

    struct NodePushBlock
    {
        math::Matrix<4, 4> matrix;
        uint32_t           vertexStageFlag;
        uint32_t           fragmentStageFlag;
    };
    // static_assert(sizeof(PushBlock) < 128);


    struct Renderable
    {
        const asset::Asset& asset;
        VkPipelineLayout    pipelineLayout;
        VkPipeline          pipeline;
        // Pipelines           pipelines;

        void drawNode(const VkCommandBuffer commandBuffer, const asset::Node& node,
                      const math::Matrix<4, 4>& globalMatrix) const
        {
            if (!node.state.active)
            {
                return;
            }

            const NodePushBlock nodePushBlock { node.matrix() * globalMatrix, node.state.vertexStageFlag,
                                                node.state.fragmentStageFlag };

            if (node.mesh)
            {
                for (const auto& primitive : node.mesh->primitives)
                {
                    constexpr VkDeviceSize offset { 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &asset.model.vertexBuffer.buffer, &offset);
                    vkCmdBindIndexBuffer(commandBuffer, asset.model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                    auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
                        vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
                    assert(setPolygonMode);
                    setPolygonMode(commandBuffer, translate(node.state.polygonMode));

                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                            &primitive.material.descriptorSet, 0, nullptr);

                    vkCmdPushConstants(commandBuffer, pipelineLayout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                       sizeof(NodePushBlock), &nodePushBlock);

                    vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);

                    // if (primitive.state.boundingBox)
                    // {
                    //     [[maybe_unused]] const auto scale =
                    //         0.5f * node.state.scale * (primitive.bb.max - primitive.bb.min);
                    //     // const math::Vector<3> scale       = { 1, 1, 1 };
                    //     [[maybe_unused]] const auto translation =
                    //         node.state.translation + 0.5f * (primitive.bb.max + primitive.bb.min);
                    //     const NodePushBlock bboxPushBlock { nodePushBlock.matrix, nodePushBlock.vertexStageFlag,
                    //                                         nodePushBlock.fragmentStageFlag };
                    //     vkCmdPushConstants(commandBuffer, descriptorlessPipelineLayout,
                    //                        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                    //                        sizeof(NodePushBlock), &bboxPushBlock);

                    //     constexpr VkDeviceSize offset { 0 };
                    //     // vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cube.vertexBuffer.buffer, &offset);
                    //     // vkCmdBindIndexBuffer(commandBuffer, cube.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    //     // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    //     //                   pipelines.at(PipelineID { PolygonMode::line, Topology::lineList }));
                    //     // vkCmdDrawIndexed(commandBuffer, cube.indexCount, 1, 0, 0, 0);

                    //     vkCmdBindVertexBuffers(commandBuffer, 0, 1, &coordinateSystem.vertexBuffer.buffer, &offset);
                    //     vkCmdBindIndexBuffer(commandBuffer, defaults.coordinateSystem.indexBuffer.buffer, 0,
                    //                          VK_INDEX_TYPE_UINT32);
                    //     vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    //                       pipelines.at(PipelineID { PolygonMode::line, Topology::lineList }));
                    //     vkCmdDrawIndexed(commandBuffer, defaults.coordinateSystem.indexCount, 1, 0, 0, 0);
                    // }
                }
            }
            for (const auto& child : node.children)
            {
                drawNode(commandBuffer, child, nodePushBlock.matrix);
            }
        }

        void draw(const VkCommandBuffer commandBuffer, const math::Matrix<4, 4>& globalMatrix) const
        {
            if (!asset.state.active)
            {
                return;
            }

            if (asset.jointMatrices)
            {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                                        &asset.jointMatrices->descriptorSet, 0, nullptr);
            }
            for (const auto& node : asset.mainScene().nodes)
            {
                drawNode(commandBuffer, node, globalMatrix);
            }
        }

        ~Renderable()
        {
            context().destroy(pipeline);
            context().destroy(pipelineLayout);
        }
    };

    Renderer(const std::filesystem::path& shaders, std::vector<asset::Asset>& assets)
        : assets { assets }
        , camera { 16.0 / 9.0, { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , renderables { createRenderables(shaders, assets) }
    {
    }

    std::vector<asset::Asset>&  assets;
    mutable Camera<true, false> camera;
    std::vector<Renderable>     renderables;


    void update(const VkExtent2D, const UserInteraction& ui)
    {
        camera.update(ui);
        for (auto& asset : assets)
        {
            asset.update(ui.elapsedTime);
        }
    }

    // void draw(const VkCommandBuffer commandBuffer, const Model& model, const asset::Node& node,
    //           const math::Matrix<4, 4>& globalMatrix) const
    // {
    //     if (!node.state.active)
    //     {
    //         return;
    //     }

    //     const NodePushBlock nodePushBlock { node.matrix() * globalMatrix, node.state.vertexStageFlag,
    //                                         node.state.fragmentStageFlag };

    //     if (node.mesh)
    //     {
    //         for (const auto& primitive : node.mesh->primitives)
    //         {
    //             constexpr VkDeviceSize offset { 0 };
    //             vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
    //             vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    //             const PipelineID pipelineId { node.state.polygonMode, Topology::vertexList };
    //             vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.at(pipelineId));

    //             const std::array descriptorSets { primitive.material.descriptorSet, node.mesh->descriptorSet };
    //             vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, assetPipelineLayout, 0,
    //                                     static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0,
    //                                     nullptr);

    //             vkCmdPushConstants(commandBuffer, assetPipelineLayout,
    //                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
    //                                sizeof(NodePushBlock), &nodePushBlock);

    //             vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);

    //             if (primitive.state.boundingBox)
    //             {
    //                 [[maybe_unused]] const auto scale = 0.5f * node.state.scale * (primitive.bb.max -
    //                 primitive.bb.min);
    //                 // const math::Vector<3> scale       = { 1, 1, 1 };
    //                 [[maybe_unused]] const auto translation =
    //                     node.state.translation + 0.5f * (primitive.bb.max + primitive.bb.min);
    //                 const NodePushBlock bboxPushBlock { nodePushBlock.matrix, nodePushBlock.vertexStageFlag,
    //                                                     nodePushBlock.fragmentStageFlag };
    //                 vkCmdPushConstants(commandBuffer, descriptorlessPipelineLayout,
    //                                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
    //                                    sizeof(NodePushBlock), &bboxPushBlock);

    //                 constexpr VkDeviceSize offset { 0 };
    //                 // vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cube.vertexBuffer.buffer, &offset);
    //                 // vkCmdBindIndexBuffer(commandBuffer, cube.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    //                 // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                 //                   pipelines.at(PipelineID { PolygonMode::line, Topology::lineList }));
    //                 // vkCmdDrawIndexed(commandBuffer, cube.indexCount, 1, 0, 0, 0);

    //                 vkCmdBindVertexBuffers(commandBuffer, 0, 1, &coordinateSystem.vertexBuffer.buffer, &offset);
    //                 vkCmdBindIndexBuffer(commandBuffer, coordinateSystem.indexBuffer.buffer, 0,
    //                 VK_INDEX_TYPE_UINT32); vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                                   pipelines.at(PipelineID { PolygonMode::line, Topology::lineList }));
    //                 vkCmdDrawIndexed(commandBuffer, coordinateSystem.indexCount, 1, 0, 0, 0);
    //             }
    //         }
    //     }
    //     for (const auto& child : node.children)
    //     {
    //         draw(commandBuffer, model, child, nodePushBlock.matrix);
    //     }
    // }

    void draw(const VkCommandBuffer commandBuffer, const VkExtent2D extent) const
    {
        const VkViewport viewport {
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = static_cast<float>(extent.width),
            .height   = static_cast<float>(extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        const VkRect2D scissor {
            .offset = { 0, 0 },
            .extent = extent,
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        for (const auto& renderable : renderables)
        {
            renderable.draw(commandBuffer, camera.viewProjection());
        }
    }


private:
    static std::vector<Renderable> createRenderables(const std::filesystem::path&     shaders,
                                                     const std::vector<asset::Asset>& assets)
    {
        std::vector<Renderable> renderables;
        renderables.reserve(assets.size());
        for (const auto& asset : assets)
        {
            constexpr VkPushConstantRange pushConstantRange { createPushConstantRange<NodePushBlock>(
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) };

            const VkPipelineLayout pipelineLayout {
                asset.jointMatrices ? createPipelineLayout(pushConstantRange, asset.materialDescriptorSetLayout,
                                                           asset.jointMatrices->descriptorSetLayout) :
                                      createPipelineLayout(pushConstantRange, asset.materialDescriptorSetLayout)
            };

            const auto   verticesShader  = shaders / (asset.shader + ".vert.spv");
            const auto   fragmentsShader = shaders / (asset.shader + ".frag.spv");
            const Shader shader {
                ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { verticesShader, nullptr },
                ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { fragmentsShader, nullptr },
            };
            renderables.emplace_back(
                asset, pipelineLayout,
                createGraphicPipeline(asset.vertexInputState, VK_NULL_HANDLE, pipelineLayout, shader));
        }
        return renderables;
    }
};

}  // namespace surge
