#pragma once

#include "surge/Camera.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/asset/Line.hpp"
#include "surge/core/Descriptor.hpp"

namespace surge
{

class Renderer : public core::Contextualized
{
public:
    Renderer(const core::Context& context, const physics::Physics& physics)
        : Contextualized { context }
        , physics { physics }
        , camera { 16.0 / 9.0, { 0.0f, 1.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , scene { context, 2 * sizeof(core::math::Matrix<4, 4>), core::Buffer::uniform }
        , descriptor { context, 1,
                       core::Description<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, core::Buffer> {
                           scene } }
        , pipelines { createPipelines(context, descriptor.setLayout) }
    {
    }

    ~Renderer()
    {
        for (const auto& [name, pipeline] : pipelines)
        {
            context.destroy(pipeline.first);
            context.destroy(pipeline.second);
        }
    }

    void createPipeline(const std::string& name, const VkPipelineVertexInputStateCreateInfo& vertexInputState,
                        const core::shader::Type shader, const VkDescriptorSetLayout materialDescriptorSetLayout,
                        const std::optional<VkDescriptorSetLayout> jointMatricesDescriptorSetLayout)
    {
        constexpr VkPushConstantRange nodePushConstantRange { core::createPushConstantRange<asset::Node::PushConstants>(
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) };

        auto& [pipelineLayout, pipeline] = pipelines[name];
        pipelineLayout =
            jointMatricesDescriptorSetLayout.has_value() ?
                core::createPipelineLayout(context, nodePushConstantRange, descriptor.setLayout,
                                           materialDescriptorSetLayout, jointMatricesDescriptorSetLayout.value()) :
                core::createPipelineLayout(context, nodePushConstantRange, descriptor.setLayout,
                                           materialDescriptorSetLayout);
        pipeline = core::createGraphicPipeline(context, vertexInputState, pipelineLayout, shader);
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
            vkGetInstanceProcAddr(context.instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, core::translate(core::PolygonMode::point));

        vkCmdSetLineWidth(commandBuffer, 1.0);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        for (const auto& particle : physics.particles)
        {
            drawPoint(commandBuffer, pipelineLayout,
                      asset::Point {
                          .p     = particle.position,
                          .color = core::Colors<core::Type::rgba>::green,
                      });
        }
        for (const auto& anchor : physics.anchors)
        {
            drawPoint(commandBuffer, pipelineLayout,
                      asset::Point {
                          .p     = anchor.position,
                          .color = core::Colors<core::Type::rgba>::red,
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
            vkGetInstanceProcAddr(context.instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, core::translate(core::PolygonMode::line));

        vkCmdSetLineWidth(commandBuffer, 1.0);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        for (const auto& spring : physics.springs)
        {
            drawLine(commandBuffer, pipelineLayout,
                     asset::Line {
                         .a     = spring.first.position,
                         .b     = spring.second.position,
                         .color = core::Colors<core::Type::rgba>::white,
                     });
        }
        for (const auto& spring : physics.anchoredSprings)
        {
            drawLine(commandBuffer, pipelineLayout,
                     asset::Line {
                         .a     = spring.particle.position,
                         .b     = spring.anchor.position,
                         .color = core::Colors<core::Type::rgba>::white,
                     });
        }
    }

    void drawPoint(const VkCommandBuffer commandBuffer, const VkPipelineLayout pipelineLayout,
                   const asset::Point& point) const
    {
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(asset::Point), &point);
        vkCmdDraw(commandBuffer, 1, 1, 0, 0);
    }

    const physics::Physics&                                        physics;
    mutable Camera<true, false>                                    camera;
    core::Buffer                                                   scene;
    core::Descriptor                                               descriptor;
    std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;

    void update(const Input& input)
    {
        camera.update(input);
        const std::array sceneMatrices {
            core::math::fullMatrix(camera.mats.perspective),
            core::math::fullMatrix(camera.mats.view),
        };
        memcpy(scene.mapped, sceneMatrices.data(), 2 * sizeof(core::math::Matrix<4, 4>));
    }

    void draw(const VkCommandBuffer commandBuffer) const
    {
        drawParticles(commandBuffer, descriptor.set);
        drawSprings(commandBuffer, descriptor.set);
    }


private:
    static std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>>
    createPipelines(const core::Context& context, const VkDescriptorSetLayout sceneDescriptorSetLayout)
    {
        std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;

        constexpr VkPipelineVertexInputStateCreateInfo emptyVertexInputState = core::createVertexInputState();
        {  // line
            auto& [pipelineLayout, pipeline] = pipelines["line"];
            pipelineLayout                   = core::createPipelineLayout(
                context, core::createPushConstantRange<asset::Line>(VK_SHADER_STAGE_VERTEX_BIT),
                sceneDescriptorSetLayout);
            pipeline = core::createGraphicPipeline(
                context, emptyVertexInputState, VK_NULL_HANDLE, pipelineLayout,
                core::shader::Shader {
                    context,
                    core::shader::ShaderInfo<core::shader::Type::line, core::shader::Stage::vertex> { nullptr },
                    core::shader::ShaderInfo<core::shader::Type::line, core::shader::Stage::fragment> { nullptr },
                },
                VkPipelineInputAssemblyStateCreateInfo {
                    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .pNext                  = nullptr,
                    .flags                  = {},
                    .topology               = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                    .primitiveRestartEnable = VK_FALSE,
                });
        }

        {  // point
            auto& [pipelineLayout, pipeline] = pipelines["point"];
            pipelineLayout                   = core::createPipelineLayout(
                context, core::createPushConstantRange<asset::Point>(VK_SHADER_STAGE_VERTEX_BIT),
                sceneDescriptorSetLayout);
            pipeline = core::createGraphicPipeline(
                context, emptyVertexInputState, VK_NULL_HANDLE, pipelineLayout,
                core::shader::Shader {
                    context,
                    core::shader::ShaderInfo<core::shader::Type::point, core::shader::Stage::vertex> { nullptr },
                    core::shader::ShaderInfo<core::shader::Type::point, core::shader::Stage::fragment> { nullptr },
                },
                VkPipelineInputAssemblyStateCreateInfo {
                    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                    .pNext                  = nullptr,
                    .flags                  = {},
                    .topology               = VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                    .primitiveRestartEnable = VK_FALSE,
                });
        }
        return pipelines;
    }
};

}  // namespace surge
