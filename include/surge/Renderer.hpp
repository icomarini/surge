#pragma once

#include "surge/Camera.hpp"
#include "surge/asset/Asset.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/core/Pipeline.hpp"
// #include "surge/geometry/shapes.hpp"
#include "surge/asset/Line.hpp"
#include "surge/entity/Entity.hpp"

namespace surge
{

class Renderer
{
public:
    Renderer(std::map<std::string, asset::Asset>& assets, const physics::Physics& physics)
        : assets { assets }
        , physics { physics }
        , camera { 16.0 / 9.0, { 0.0f, 1.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , scene { 2 * sizeof(math::Matrix<4, 4>), UniformBufferInfo {} }
        , descriptor { 1, UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT> { scene } }
        , pipelines { createPipelines(assets, descriptor.setLayout) }
    {
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
    std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;

    void update(const UserInteraction& ui)
    {
        camera.update(ui);
        const std::array sceneMatrices {
            math::fullMatrix(camera.mats.perspective),
            math::fullMatrix(camera.mats.view),
        };
        memcpy(scene.mapped, sceneMatrices.data(), 2 * sizeof(math::Matrix<4, 4>));
    }

    void draw(const VkCommandBuffer commandBuffer) const
    {
        drawParticles(commandBuffer, descriptor.set);
        drawSprings(commandBuffer, descriptor.set);
    }


private:
    static std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>>
    createPipelines(const std::map<std::string, asset::Asset>& assets,
                    const VkDescriptorSetLayout                sceneDescriptorSetLayout)
    {
        std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;

        constexpr VkPipelineVertexInputStateCreateInfo emptyVertexInputState = createVertexInputState();
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
            constexpr VkPushConstantRange nodePpushConstantRange { createPushConstantRange<entity::Node::PushConstants>(
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) };

            auto& [pipelineLayout, pipeline] = pipelines[name];
            pipelineLayout =
                asset.jointMatricesDescriptorSetLayout != VK_NULL_HANDLE ?
                    createPipelineLayout(nodePpushConstantRange, sceneDescriptorSetLayout,
                                         asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout) :
                    createPipelineLayout(nodePpushConstantRange, sceneDescriptorSetLayout,
                                         asset.materialDescriptorSetLayout);
            pipeline = createGraphicPipeline(asset.vertexInputState, pipelineLayout, asset.shader);
        }
        return pipelines;
    }
};

}  // namespace surge
