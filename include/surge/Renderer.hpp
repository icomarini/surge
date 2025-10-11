#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Camera.hpp"
#include "surge/asset/Asset.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/Pipeline.hpp"
#include "surge/geometry/shapes.hpp"
#include "surge/asset/Line.hpp"

namespace surge
{

constexpr VkPolygonMode translate(const PolygonMode polygonMode)
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
    struct NodePushBlock
    {
        math::Matrix<4, 4> matrix;
        math::Vector<4>    baseColorFactor;
        uint32_t           vertexStageFlag;
        uint32_t           fragmentStageFlag;
    };

    struct Renderable
    {
        const asset::Asset&      asset;
        VkPipelineLayout         pipelineLayout;
        VkPipeline               pipeline;
        std::vector<asset::Node> nodes;

        struct Animation
        {
            // asset::ShaderStorageBufferObject jointMatricesSSBO;

            mutable struct State
            {
                bool                            active { true };
                float                           progress { 0 };
                std::vector<math::Matrix<4, 4>> jointMatrices;
            } state;
        };
        std::optional<Animation> animation;

        struct State
        {
            bool               active { true };
            math::Matrix<4, 4> modelMatrix;
        };
        mutable State state;

        void drawNode(const asset::Asset& asset, const VkCommandBuffer commandBuffer, const asset::Node& node,
                      const math::Matrix<4, 4>& globalMatrix) const
        {
            if (!node.state.active)
            {
                return;
            }

            NodePushBlock nodePushBlock {
                .matrix            = globalMatrix * node.state.localMatrix,
                .baseColorFactor   = {},
                .vertexStageFlag   = node.state.vertexStageFlag,
                .fragmentStageFlag = node.state.fragmentStageFlag,
            };

            if (node.meshIndex)
            {
                for (const auto& primitive : asset.meshes.at(node.meshIndex.value()).primitives)
                {
                    auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
                        vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
                    assert(setPolygonMode);
                    setPolygonMode(commandBuffer, translate(node.state.polygonMode));

                    // bind material
                    constexpr uint32_t materialIndex = 1;
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                            materialIndex, 1, &primitive.material.descriptorSet, 0, nullptr);

                    nodePushBlock.baseColorFactor   = primitive.material.baseColorFactor;
                    nodePushBlock.fragmentStageFlag = 0;
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
                drawNode(asset, commandBuffer, child, nodePushBlock.matrix);
            }
        }

        void draw(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
        {
            if (!asset.state.active)
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
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, sceneUniformIndex,
                                    1, &sceneDescriptor, 0, nullptr);

            if (asset.jointMatricesSSBO)
            {
                // bind joint matrices ssbo
                constexpr uint32_t jointMatricesSSBOIndex = 2;
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                        jointMatricesSSBOIndex, 1, &asset.jointMatricesSSBO->descriptorSet, 0, nullptr);
            }
            for (const auto& node : asset.mainScene().nodes)
            {
                drawNode(asset, commandBuffer, node, asset.state.modelMatrix);
            }
            for (const auto& node : nodes)
            {
                drawNode(asset, commandBuffer, node, asset.state.modelMatrix * math::Translation({ 1, 0, 0 }));
            }
        }

        void update() const
        {
            for (const auto& node : nodes)
            {
                node.update(math::identity<4>);
            }
        }

        void updateJoints(const asset::Node& node)
        {
            if (node.skinIndex)
            {
                assert(animation);
                const auto& skin          = asset.skins.at(node.skinIndex.value());
                auto&       jointMatrices = animation->state.jointMatrices;
                jointMatrices.clear();
                jointMatrices.reserve(skin.joints.size());

                const auto inverse = math::inverse(node.state.globalMatrix);

                for (const auto& [jointNode, jointNodeIndex, inverseBindMatrix] : skin.joints)
                {
                    jointMatrices.emplace_back(inverse * jointNode.state.globalMatrix * inverseBindMatrix);
                }

                // assert(jointMatricesSSBO);
                // memcpy(jointMatricesSSBO->buffer.mapped, state.jointMatrices.data(),
                //        state.jointMatrices.size() * sizeof(math::Matrix<4, 4>));
            }

            for (const auto& child : node.children)
            {
                updateJoints(child);
            }
        }

        ~Renderable()
        {
            context().destroy(pipeline);
            context().destroy(pipelineLayout);
        }
    };


    Renderer(std::vector<asset::Asset>& assets, const physics::Physics& physics /*, std::vector<asset::Line>& lines,
             std::vector<asset::Point>& points*/)
        : assets { assets }
        , physics { physics }
        , camera { 16.0 / 9.0, { 0.0f, 1.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , scene { 2 * sizeof(math::Matrix<4, 4>), UniformBufferInfo {} }
        , descriptor { 1, UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT> { scene } }
        , renderables { createRenderables(descriptor, assets) }
        , linePipelineLayout { createPipelineLayout(createPushConstantRange<asset::Line>(VK_SHADER_STAGE_VERTEX_BIT),
                                                    descriptor.setLayout) }
        , linePipeline { createGraphicPipeline(
              geometry::createVertexInputState(), VK_NULL_HANDLE, linePipelineLayout,
              shader::Shader {
                  shader::ShaderInfo<shader::Type::line, shader::Stage::vertex> { nullptr },
                  shader::ShaderInfo<shader::Type::line, shader::Stage::fragment> { nullptr },
              },
              VkPipelineInputAssemblyStateCreateInfo {
                  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                  .pNext                  = nullptr,
                  .flags                  = {},
                  .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                  .primitiveRestartEnable = VK_FALSE,
              }) }
        , pointPipelineLayout { createPipelineLayout(createPushConstantRange<asset::Point>(VK_SHADER_STAGE_VERTEX_BIT),
                                                     descriptor.setLayout) }
        , pointPipeline { createGraphicPipeline(
              geometry::createVertexInputState(), VK_NULL_HANDLE, pointPipelineLayout,
              shader::Shader {
                  shader::ShaderInfo<shader::Type::point, shader::Stage::vertex> { nullptr },
                  shader::ShaderInfo<shader::Type::point, shader::Stage::fragment> { nullptr },
              },
              VkPipelineInputAssemblyStateCreateInfo {
                  .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                  .pNext                  = nullptr,
                  .flags                  = {},
                  .topology               = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                  .primitiveRestartEnable = VK_FALSE,
              }) }
    {
    }

    // entity::Entity createEntity(const Index sceneIndex) const
    // {
    //     return entity::Entity {
    //         .nodes     = createTree(sceneIndex),
    //         .animation = createAnimation(),
    //         .state =
    //             entity::Entity::State {
    //                 .active      = true,
    //                 .modelMatrix = math::fullMatrix(math::identity<4>),
    //             },
    //     };
    // }

    ~Renderer()
    {
        context().destroy(pointPipeline);
        context().destroy(pointPipelineLayout);
        context().destroy(linePipeline);
        context().destroy(linePipelineLayout);
    }

    void drawLine(const asset::Line& line, const VkCommandBuffer commandBuffer,
                  const VkDescriptorSet sceneDescriptor) const
    {
        constexpr uint32_t sceneUniformIndex = 0;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipelineLayout, sceneUniformIndex,
                                1, &sceneDescriptor, 0, nullptr);

        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, translate(PolygonMode::line));

        vkCmdSetLineWidth(commandBuffer, 1.0);

        vkCmdPushConstants(commandBuffer, linePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(asset::Line),
                           &line);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);

        vkCmdDraw(commandBuffer, 2, 1, 0, 0);
    }

    // void drawLines(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    // {
    //     for (const auto& line : lines)
    //     {
    //         drawLine(line, commandBuffer, sceneDescriptor);
    // // bind scene uniform
    // constexpr uint32_t sceneUniformIndex = 0;
    // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipelineLayout,
    //                         sceneUniformIndex, 1, &sceneDescriptor, 0, nullptr);

    // auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
    //     vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
    // assert(setPolygonMode);
    // setPolygonMode(commandBuffer, translate(PolygonMode::line));

    // vkCmdSetLineWidth(commandBuffer, 1.0);

    // vkCmdPushConstants(commandBuffer, linePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(asset::Line),
    //                    &line);

    // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);

    // vkCmdDraw(commandBuffer, 2, 1, 0, 0);
    //     }
    // }

    void drawAnchors(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    {
        for (const auto& anchor : physics.anchors)
        {
            drawPoint(
                asset::Point {
                    .p     = anchor.position,
                    .color = colors::red,
                },
                commandBuffer, sceneDescriptor);
        }
    }

    void drawParticles(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    {
        for (const auto& particle : physics.particles)
        {
            drawPoint(
                asset::Point {
                    .p     = particle.position,
                    .color = colors::green,
                },
                commandBuffer, sceneDescriptor);
        }
    }

    void drawAnchoredSprings(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    {
        for (const auto& anchoredSpring : physics.anchoredSprings)
        {
            drawLine(
                asset::Line {
                    .a     = anchoredSpring.anchor.position,
                    .b     = anchoredSpring.particle.position,
                    .color = colors::white,
                },
                commandBuffer, sceneDescriptor);
        }
    }

    void drawSprings(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    {
        for (const auto& spring : physics.springs)
        {
            drawLine(
                asset::Line {
                    .a     = spring.first.position,
                    .b     = spring.second.position,
                    .color = colors::white,
                },
                commandBuffer, sceneDescriptor);
        }
    }

    void drawPoint(const asset::Point& point, const VkCommandBuffer commandBuffer,
                   const VkDescriptorSet sceneDescriptor) const
    {
        constexpr uint32_t sceneUniformIndex = 0;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipelineLayout, sceneUniformIndex,
                                1, &sceneDescriptor, 0, nullptr);

        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, translate(PolygonMode::point));

        vkCmdPushConstants(commandBuffer, pointPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(asset::Point),
                           &point);

        vkCmdSetLineWidth(commandBuffer, 1.0);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline);

        vkCmdDraw(commandBuffer, 1, 1, 0, 0);
    }

    // void drawPoints(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    // {
    //     for (const auto& point : points)
    //     {
    //         drawPoint(point, commandBuffer, sceneDescriptor);
    // // bind scene uniform
    // constexpr uint32_t sceneUniformIndex = 0;
    // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipelineLayout,
    //                         sceneUniformIndex, 1, &sceneDescriptor, 0, nullptr);

    // auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
    //     vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
    // assert(setPolygonMode);
    // setPolygonMode(commandBuffer, translate(PolygonMode::point));

    // vkCmdPushConstants(commandBuffer, pointPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
    // sizeof(asset::Point),
    //                    &point);

    // vkCmdSetLineWidth(commandBuffer, 1.0);

    // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline);

    // vkCmdDraw(commandBuffer, 1, 1, 0, 0);
    //     }
    // }

    std::vector<asset::Asset>&  assets;
    const physics::Physics&     physics;
    mutable Camera<true, false> camera;
    Buffer                      scene;
    Descriptor                  descriptor;
    std::vector<Renderable>     renderables;

    VkPipelineLayout linePipelineLayout;
    VkPipeline       linePipeline;

    VkPipelineLayout pointPipelineLayout;
    VkPipeline       pointPipeline;

    void update(const VkExtent2D, const UserInteraction& ui)
    {
        camera.update(ui);
        const std::array sceneMatrices {
            math::fullMatrix(camera.mats.perspective),
            math::fullMatrix(camera.mats.view),
        };
        memcpy(scene.mapped, sceneMatrices.data(), 2 * sizeof(math::Matrix<4, 4>));

        for (auto& asset : assets)
        {
            asset.update(ui.elapsedTime);
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

        for (const auto& renderable : renderables)
        {
            renderable.update();
        }

        for (const auto& renderable : renderables)
        {
            renderable.draw(commandBuffer, descriptor.set);
        }

        if (!physics.particles.empty())
        {
            drawParticles(commandBuffer, descriptor.set);
        }
        drawAnchors(commandBuffer, descriptor.set);
        drawAnchoredSprings(commandBuffer, descriptor.set);
        drawSprings(commandBuffer, descriptor.set);

        // drawLines(commandBuffer, descriptor.set);
        // drawPoints(commandBuffer, descriptor.set);
    }


private:
    static std::vector<Renderable> createRenderables(const Descriptor&                descriptor,
                                                     const std::vector<asset::Asset>& assets)
    {
        std::vector<Renderable> renderables;
        renderables.reserve(assets.size());
        for (const auto& asset : assets)
        {
            constexpr VkPushConstantRange pushConstantRange { createPushConstantRange<NodePushBlock>(
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) };

            const VkPipelineLayout pipelineLayout {
                asset.jointMatricesSSBO ?
                    createPipelineLayout(pushConstantRange, descriptor.setLayout, asset.materialDescriptorSetLayout,
                                         asset.jointMatricesSSBO->descriptorSetLayout) :
                    createPipelineLayout(pushConstantRange, descriptor.setLayout, asset.materialDescriptorSetLayout)
            };

            renderables.emplace_back(asset, pipelineLayout,
                                     createGraphicPipeline(asset.vertexInputState, pipelineLayout, asset.shader),
                                     asset.mainScene().nodes, std::nullopt,
                                     Renderable::State {
                                         .active      = true,
                                         .modelMatrix = math::fullMatrix(math::identity<4>),
                                     });
        }
        return renderables;
    }
};

}  // namespace surge
