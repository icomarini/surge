#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Camera.hpp"
#include "surge/asset/Asset.hpp"
#include "surge/Pipeline.hpp"

#include "surge/geometry/shapes.hpp"

namespace surge
{

class Renderer
{
public:
    struct NodePushBlock
    {
        math::Matrix<4, 4> matrix;
        uint32_t           vertexStageFlag;
        uint32_t           fragmentStageFlag;
    };
    // static_assert(sizeof(PushBlock) < 128);

    Renderer(const Command& command, const std::filesystem::path& shaders, std::vector<asset::Asset>& assets)
        : assets { assets }
        , camera { 16.0 / 9.0, { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , cube { command, geometry::cubeLine, true, SceneModelInfo {} }
        , coordinateSystem { command, geometry::coordinateSystem, true, SceneModelInfo {} }
        , descriptorSetLayouts { {
              assets.front().materialDescriptorSetLayout,
              assets.front().meshDescriptorSetLayout,
              assets.front().jointMatrices->descriptorSetLayout,
          } }
        , assetPipelineLayout { createPipelineLayout(
              descriptorSetLayouts.front(),
              createPushConstantRange<NodePushBlock>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)) }
        , descriptorlessPipelineLayout { createPipelineLayout(
              createPushConstantRange<NodePushBlock>(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)) }
        , pipelines {
            { PipelineID { PolygonMode::line, Topology::lineList },
              createGraphicPipeline<geometry::PositionAndColor>(
                  command.renderPass, VK_NULL_HANDLE, descriptorlessPipelineLayout,
                  Shader { ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "bbox.vert.spv", nullptr },
                           ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "bbox.frag.spv", nullptr } },
                  createRasterizationStateInfo(VK_POLYGON_MODE_LINE),
                  VkPipelineInputAssemblyStateCreateInfo {
                      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                      .pNext                  = nullptr,
                      .flags                  = {},
                      .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                      .primitiveRestartEnable = VK_FALSE,
                  }) },
            { PipelineID { PolygonMode::point, Topology::vertexList },
              createGraphicPipeline<asset::Asset::Vertex>(
                  command.renderPass, VK_NULL_HANDLE, assetPipelineLayout,
                  Shader { ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "asset.vert.spv", nullptr },
                           ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "asset.frag.spv", nullptr } },
                  createRasterizationStateInfo(VK_POLYGON_MODE_POINT)) },
            { PipelineID { PolygonMode::line, Topology::vertexList },
              createGraphicPipeline<asset::Asset::Vertex>(
                  command.renderPass, VK_NULL_HANDLE, assetPipelineLayout,
                  Shader { ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "asset.vert.spv", nullptr },
                           ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "asset.frag.spv", nullptr } },
                  createRasterizationStateInfo(VK_POLYGON_MODE_LINE)) },
            { PipelineID { PolygonMode::fill, Topology::vertexList },
              createGraphicPipeline<asset::Asset::Vertex>(
                  command.renderPass, VK_NULL_HANDLE, assetPipelineLayout,
                  Shader { ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "asset.vert.spv", nullptr },
                           ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "asset.frag.spv", nullptr } },
                  createRasterizationStateInfo(VK_POLYGON_MODE_FILL)) },
        }
    {
    }

    std::vector<asset::Asset>&  assets;
    mutable Camera<true, false> camera;

    Model cube;
    Model coordinateSystem;

    std::vector<std::array<VkDescriptorSetLayout, 3>> descriptorSetLayouts;
    VkPipelineLayout                                  assetPipelineLayout;
    VkPipelineLayout                                  descriptorlessPipelineLayout;

    enum class Topology
    {
        vertexList,
        lineList,
    };

    struct PipelineID
    {
        PolygonMode polygonMode;
        Topology    topology;
    };


    struct compare
    {
        bool operator()(PipelineID lhs, PipelineID rhs) const
        {
            return lhs.polygonMode == rhs.polygonMode ? lhs.topology < rhs.topology : lhs.polygonMode < rhs.polygonMode;
        }
    };

    using Pipelines = std::map<PipelineID, VkPipeline, compare>;
    Pipelines pipelines;

    void update(const VkExtent2D, const UserInteraction& ui)
    {
        camera.update(ui);
        // pushConstantBlock.fragmentStageFlag = ui.wireframe;
        for (auto& asset : assets)
        {
            asset.update(ui.elapsedTime);
        }
    }

    // void drawOffscreen(const VkCommandBuffer commandBuffer) const
    // {
    // }

    void draw(const VkCommandBuffer commandBuffer, const Model& model, const asset::Node& node,
              const math::Matrix<4, 4>& globalMatrix) const
    {
        if (!node.state.active)
        {
            return;
        }

        // const math::Scaling     scaling { node.state.scale };
        // const math::Rotation    rotation { node.state.attitude };
        // const math::Translation translation { node.state.translation };

        // const math::Transformation transformation { node.state.scale, node.state.translation, node.state.attitude };

        // const auto transformation = translation * rotation * scaling;

        const NodePushBlock nodePushBlock { node.matrix() * globalMatrix, node.state.vertexStageFlag,
                                            node.state.fragmentStageFlag };


        // assert(equalm(math::Scaling { node.state.scale }, math::Scaling { node.state.transformation.scale }));

        // assert(equalm(math::Translation { node.state.translation },
        //               math::Translation { node.state.transformation.translation }));

        // const math::Rotation rot { node.state.rotation };
        // std::cout << "old rotation from " << node.state.rotation << std::endl;
        // std::cout << toString(rot) << std::endl;

        // const math::Rotation trot { node.state.transformation.rotation };
        // std::cout << "new rotation:" << std::endl;
        // std::cout << toString(trot) << std::endl;

        // assert(equalm(rot, trot));

        // assert(equalm(nodePushBlock.matrix, node.state.transformation.matrix() * globalMatrix));

        // const NodePushBlock nodePushBlock { node.state.transformation.matrix() * globalMatrix,
        //                                     node.state.vertexStageFlag, node.state.fragmentStageFlag };


        if (node.mesh)
        {
            for (const auto& primitive : node.mesh->primitives)
            {
                constexpr VkDeviceSize offset { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                const PipelineID pipelineId { node.state.polygonMode, Topology::vertexList };
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.at(pipelineId));

                const std::array descriptorSets { primitive.material.descriptorSet, node.mesh->descriptorSet };
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, assetPipelineLayout, 0,
                                        static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0,
                                        nullptr);

                vkCmdPushConstants(commandBuffer, assetPipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(NodePushBlock),
                                   &nodePushBlock);

                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);

                if (primitive.state.boundingBox)
                {
                    [[maybe_unused]] const auto scale = 0.5f * node.state.scale * (primitive.bb.max - primitive.bb.min);
                    // const math::Vector<3> scale       = { 1, 1, 1 };
                    [[maybe_unused]] const auto translation =
                        node.state.translation + 0.5f * (primitive.bb.max + primitive.bb.min);
                    const NodePushBlock bboxPushBlock { nodePushBlock.matrix, nodePushBlock.vertexStageFlag,
                                                        nodePushBlock.fragmentStageFlag };
                    vkCmdPushConstants(commandBuffer, descriptorlessPipelineLayout,
                                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                       sizeof(NodePushBlock), &bboxPushBlock);

                    constexpr VkDeviceSize offset { 0 };
                    // vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cube.vertexBuffer.buffer, &offset);
                    // vkCmdBindIndexBuffer(commandBuffer, cube.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    //                   pipelines.at(PipelineID { PolygonMode::line, Topology::lineList }));
                    // vkCmdDrawIndexed(commandBuffer, cube.indexCount, 1, 0, 0, 0);

                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &coordinateSystem.vertexBuffer.buffer, &offset);
                    vkCmdBindIndexBuffer(commandBuffer, coordinateSystem.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                      pipelines.at(PipelineID { PolygonMode::line, Topology::lineList }));
                    vkCmdDrawIndexed(commandBuffer, coordinateSystem.indexCount, 1, 0, 0, 0);
                }
            }
        }
        for (const auto& child : node.children)
        {
            draw(commandBuffer, model, child, nodePushBlock.matrix);
        }
    }

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

        for (const auto& asset : assets)
        {
            if (!asset.state.active)
            {
                continue;
            }

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, assetPipelineLayout, 2, 1,
                                    &asset.jointMatrices->descriptorSet, 0, nullptr);
            for (const auto& node : asset.mainScene().nodes)
            {
                draw(commandBuffer, asset.model, node, camera.viewProjection());
            }
        }
    }

    ~Renderer()
    {
        for (auto pipeline : pipelines)
        {
            context().destroy(pipeline.second);
        }
        context().destroy(assetPipelineLayout);
        context().destroy(descriptorlessPipelineLayout);
    }
};

}  // namespace surge
