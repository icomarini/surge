#pragma once

#include "surge/Camera.hpp"
#include "surge/physics/Physics.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/asset/Line.hpp"
#include "surge/core/Descriptor.hpp"

namespace surge {

class Renderer : public core::Contextualized {
public:
    struct SceneBuffer {
        core::math::Matrix<4, 4> perspective;
        core::math::Matrix<4, 4> view;
        core::math::Vector<4>    lightColor;
        core::math::Vector<3>    lightPosition;
    };


    Renderer(const core::Context& context)
        : Contextualized { context }
        , scene { context, sizeof(SceneBuffer), core::Buffer::uniform }
        , descriptor { context, 1,
                       core::Description<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                         VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_VERTEX_BIT |
                                             VK_SHADER_STAGE_FRAGMENT_BIT,
                                         core::Buffer> { scene } }
        , pipelines { createPipelines(context, descriptor.setLayout) } {
    }

    ~Renderer() {
        for (const auto& [name, pipeline] : pipelines) {
            context.destroy(pipeline.first);
            context.destroy(pipeline.second);
        }
    }

    void createPipeline(const std::string& name, const VkPipelineVertexInputStateCreateInfo& vertexInputState,
                        const core::shader::Type shader, const VkDescriptorSetLayout materialDescriptorSetLayout,
                        const std::optional<VkDescriptorSetLayout> jointMatricesDescriptorSetLayout) {
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

    core::Buffer                                                   scene;
    core::Descriptor                                               descriptor;
    std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>> pipelines;

    void update(const Camera<false>& camera, const core::math::Vector<4>& lightColor,
                const core::math::Vector<3> lightPosition) {
        const SceneBuffer sceneMatrices { core::math::fullMatrix(camera.mats.perspective),
                                          core::math::fullMatrix(camera.mats.view), lightColor, lightPosition };
        memcpy(scene.mapped, &sceneMatrices, sizeof(SceneBuffer));
    }

    // void draw(const VkCommandBuffer commandBuffer) const {
    //     drawParticles(commandBuffer, descriptor.set);
    //     drawSprings(commandBuffer, descriptor.set);
    // }

private:
    static std::map<std::string, std::pair<VkPipelineLayout, VkPipeline>>
    createPipelines(const core::Context& context, const VkDescriptorSetLayout sceneDescriptorSetLayout) {
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
