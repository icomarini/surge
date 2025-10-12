#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Camera.hpp"
#include "surge/asset/Asset.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/Pipeline.hpp"
#include "surge/geometry/shapes.hpp"
#include "surge/asset/Line.hpp"
#include "surge/entity/Entity.hpp"

namespace surge
{

// constexpr VkPolygonMode translate(const PolygonMode polygonMode)
// {
//     switch (polygonMode)
//     {
//     case PolygonMode::point:
//         return VK_POLYGON_MODE_POINT;
//     case PolygonMode::line:
//         return VK_POLYGON_MODE_LINE;
//     case PolygonMode::fill:
//         return VK_POLYGON_MODE_FILL;
//     default:
//         throw;
//     }
// }

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


    Renderer(std::map<std::string, asset::Asset>& assets, const physics::Physics& physics)
        : assets { assets }
        , physics { physics }
        , camera { 16.0 / 9.0, { 0.0f, 1.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , scene { 2 * sizeof(math::Matrix<4, 4>), UniformBufferInfo {} }
        , descriptor { 1, UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT> { scene } }
        , renderables { createRenderables(descriptor, assets) }
        , pipelines { createPipelines(assets, descriptor.setLayout) }
    {
    }

    entity::Entity createEntity(const std::string& name, const Index sceneIndex,
                                const math::StaticMatrix auto& modelMatrix) const
    {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = pipelines.at(name);
        return entity::Entity {
            .asset          = asset,
            .nodes          = asset.scenes.at(sceneIndex).treenNodes,
            .pipelineLayout = pipelineLayout,
            .pipeline       = pipeline,
            .animation =
                !asset.skins.empty() ?
                    std::optional<entity::Entity::Animation> { std::in_place, asset.descriptorPool, asset.skins } :
                    std::optional<entity::Entity::Animation> {},
            .state =
                entity::Entity::State {
                    .active      = true,
                    .modelMatrix = math::fullMatrix(modelMatrix),
                },
        };
    }

    ~Renderer()
    {
        for (const auto& [name, pipeline] : pipelines)
        {
            context().destroy(pipeline.first);
            context().destroy(pipeline.second);
        }
    }

    void drawLine(const VkCommandBuffer commandBuffer, const VkPipelineLayout pipelineLayout,
                  const asset::Line& line) const
    {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(asset::Line), &line);
        vkCmdDraw(commandBuffer, 2, 1, 0, 0);
    }

    void drawParticles(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    {
        if (physics.particles.empty() && physics.anchors.empty())
        {
            return;
        }

        const auto [pipelineLayout, pipeline] = pipelines.at("point");

        constexpr uint32_t sceneUniformIndex = 0;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, sceneUniformIndex, 1,
                                &sceneDescriptor, 0, nullptr);

        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, translate(PolygonMode::point));

        vkCmdSetLineWidth(commandBuffer, 1.0);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        for (const auto& particle : physics.particles)
        {
            drawPoint(commandBuffer, pipelineLayout,
                      asset::Point {
                          .p     = particle.position,
                          .color = colors::green,
                      });
        }
        for (const auto& anchor : physics.anchors)
        {
            drawPoint(commandBuffer, pipelineLayout,
                      asset::Point {
                          .p     = anchor.position,
                          .color = colors::red,
                      });
        }
    }

    void drawSprings(const VkCommandBuffer commandBuffer, const VkDescriptorSet sceneDescriptor) const
    {
        if (physics.springs.empty() && physics.anchoredSprings.empty())
        {
            return;
        }

        const auto [pipelineLayout, pipeline] = pipelines.at("line");

        constexpr uint32_t sceneUniformIndex = 0;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, sceneUniformIndex, 1,
                                &sceneDescriptor, 0, nullptr);
        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, translate(PolygonMode::line));

        vkCmdSetLineWidth(commandBuffer, 1.0);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        for (const auto& spring : physics.springs)
        {
            drawLine(commandBuffer, pipelineLayout,
                     asset::Line {
                         .a     = spring.first.position,
                         .b     = spring.second.position,
                         .color = colors::white,
                     });
        }
        for (const auto& spring : physics.anchoredSprings)
        {
            drawLine(commandBuffer, pipelineLayout,
                     asset::Line {
                         .a     = spring.particle.position,
                         .b     = spring.anchor.position,
                         .color = colors::white,
                     });
        }
    }

    void drawPoint(const VkCommandBuffer commandBuffer, const VkPipelineLayout pipelineLayout,
                   const asset::Point& point) const
    {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(asset::Point), &point);
        vkCmdDraw(commandBuffer, 1, 1, 0, 0);
    }

    std::map<std::string, asset::Asset>&                           assets;
    const physics::Physics&                                        physics;
    mutable Camera<true, false>                                    camera;
    Buffer                                                         scene;
    Descriptor                                                     descriptor;
    std::vector<Renderable>                                        renderables;
    std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;

    void update(const VkExtent2D, const UserInteraction& ui)
    {
        update({}, ui);
    }

    void update(const UserInteraction& ui)
    {
        camera.update(ui);
        const std::array sceneMatrices {
            math::fullMatrix(camera.mats.perspective),
            math::fullMatrix(camera.mats.view),
        };
        memcpy(scene.mapped, sceneMatrices.data(), 2 * sizeof(math::Matrix<4, 4>));

        // for (auto& asset : assets)
        // {
        //     asset.second.update(ui.elapsedTime);
        // }
    }

    void draw(const VkCommandBuffer commandBuffer) const
    {
        // const VkViewport viewport {
        //     .x        = 0.0f,
        //     .y        = 0.0f,
        //     .width    = static_cast<float>(extent.width),
        //     .height   = static_cast<float>(extent.height),
        //     .minDepth = 0.0f,
        //     .maxDepth = 1.0f,
        // };
        // vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        // const VkRect2D scissor {
        //     .offset = { 0, 0 },
        //     .extent = extent,
        // };
        // vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // for (const auto& renderable : renderables)
        // {
        //     renderable.update();
        // }

        // for (const auto& renderable : renderables)
        // {
        //     renderable.draw(commandBuffer, descriptor.set);
        // }

        drawParticles(commandBuffer, descriptor.set);
        drawSprings(commandBuffer, descriptor.set);
    }


private:
    static std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>>
    createPipelines(const std::map<std::string, asset::Asset>& assets,
                    const VkDescriptorSetLayout                sceneDescriptorSetLayout)
    {
        std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;

        constexpr VkPipelineVertexInputStateCreateInfo emptyVertexInputState = geometry::createVertexInputState();
        {  // line
            auto& [pipelineLayout, pipeline] = pipelines["line"];
            pipelineLayout = createPipelineLayout(createPushConstantRange<asset::Line>(VK_SHADER_STAGE_VERTEX_BIT),
                                                  sceneDescriptorSetLayout);
            pipeline =
                createGraphicPipeline(emptyVertexInputState, VK_NULL_HANDLE, pipelineLayout,
                                      shader::Shader {
                                          shader::ShaderInfo<shader::Type::line, shader::Stage::vertex> { nullptr },
                                          shader::ShaderInfo<shader::Type::line, shader::Stage::fragment> { nullptr },
                                      },
                                      VkPipelineInputAssemblyStateCreateInfo {
                                          .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                          .pNext    = nullptr,
                                          .flags    = {},
                                          .topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                                          .primitiveRestartEnable = VK_FALSE,
                                      });
        }

        {  // point
            auto& [pipelineLayout, pipeline] = pipelines["point"];
            pipelineLayout = createPipelineLayout(createPushConstantRange<asset::Point>(VK_SHADER_STAGE_VERTEX_BIT),
                                                  sceneDescriptorSetLayout);
            pipeline =
                createGraphicPipeline(emptyVertexInputState, VK_NULL_HANDLE, pipelineLayout,
                                      shader::Shader {
                                          shader::ShaderInfo<shader::Type::point, shader::Stage::vertex> { nullptr },
                                          shader::ShaderInfo<shader::Type::point, shader::Stage::fragment> { nullptr },
                                      },
                                      VkPipelineInputAssemblyStateCreateInfo {
                                          .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                                          .pNext    = nullptr,
                                          .flags    = {},
                                          .topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                                          .primitiveRestartEnable = VK_FALSE,
                                      });
        }

        // assets
        for (const auto& [name, asset] : assets)
        {
            constexpr VkPushConstantRange nodePpushConstantRange { createPushConstantRange<NodePushBlock>(
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) };

            auto& [pipelineLayout, pipeline] = pipelines[name];
            pipelineLayout                   = asset.jointMatricesSSBO ?
                                                   createPipelineLayout(nodePpushConstantRange, sceneDescriptorSetLayout,
                                                                        asset.materialDescriptorSetLayout,
                                                                        asset.jointMatricesSSBO->descriptorSetLayout) :
                                                   createPipelineLayout(nodePpushConstantRange, sceneDescriptorSetLayout,
                                                                        asset.materialDescriptorSetLayout);
            pipeline = createGraphicPipeline(asset.vertexInputState, pipelineLayout, asset.shader);
        }
        return pipelines;
    }

    static std::vector<Renderable> createRenderables(const Descriptor&                          descriptor,
                                                     const std::map<std::string, asset::Asset>& assets)
    {
        std::vector<Renderable> renderables;
        renderables.reserve(assets.size());
        for (const auto& [name, asset] : assets)
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
