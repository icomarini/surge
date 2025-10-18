#pragma once

#include "surge/asset/Texture.hpp"
#include "surge/asset/Model.hpp"
#include "surge/core/geometry/shapes.hpp"
#include "surge/core/input/input.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/load/LoadedTexture.hpp"
#include "surge/load/Defaults.hpp"

namespace surge::entity
{

class Skybox
{
public:
    struct PushConstants
    {
        core::math::Matrix<4, 4> matrix;
    };

    Skybox(const asset::Texture& texture, const load::Defaults& defaults, const VkPipelineLayout pipelineLayout,
           const VkPipeline pipeline)
        : camera { 16.0 / 9.0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }
        , texture { texture }
        , model { defaults.cube }
        , descriptor { 1, core::Description<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                            asset::Texture> { texture } }
        , pipelineLayout { core::createPipelineLayout(core::createPushConstantRange<core::math::Matrix<4, 4>>(
                                                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
                                                      descriptor.setLayout) }
        , pipeline { core::createGraphicPipeline(
              core::createVertexInputState<core::geometry::Position>(), VK_NULL_HANDLE, pipelineLayout,
              core::shader::Shader {
                  core::shader::ShaderInfo<core::shader::Type::skybox, core::shader::Stage::vertex> { nullptr },
                  core::shader::ShaderInfo<core::shader::Type::skybox, core::shader::Stage::fragment> { nullptr } },
              VkPipelineRasterizationStateCreateInfo {
                  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                  .pNext                   = nullptr,
                  .flags                   = {},
                  .depthClampEnable        = VK_FALSE,
                  .rasterizerDiscardEnable = VK_FALSE,
                  .polygonMode             = VK_POLYGON_MODE_FILL,
                  .cullMode                = VK_CULL_MODE_FRONT_BIT,
                  .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                  .depthBiasEnable         = VK_FALSE,
                  .depthBiasConstantFactor = 0.0f,
                  .depthBiasClamp          = 0.0f,
                  .depthBiasSlopeFactor    = 0.0f,
                  .lineWidth               = 1.0f,
              },
              VkPipelineDepthStencilStateCreateInfo {
                  .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                  .pNext                 = nullptr,
                  .flags                 = {},
                  .depthTestEnable       = VK_FALSE,
                  .depthWriteEnable      = VK_FALSE,
                  .depthCompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL,
                  .depthBoundsTestEnable = VK_FALSE,
                  .stencilTestEnable     = VK_FALSE,
                  .front                 = {},
                  .back                  = {},
                  .minDepthBounds        = 0.0f,
                  .maxDepthBounds        = 1.0f,
              }) }
    {
    }

    void update(const Input& input)
    {
        camera.update(input);
        matrix = camera.mats.perspective * camera.mats.view;
    }

    void draw(const VkCommandBuffer commandBuffer) const
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        constexpr std::array<VkDeviceSize, 1> offsets { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                           sizeof(core::math::Matrix<4, 4>), &matrix);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor.set,
                                0, nullptr);

        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(core::context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);

        vkCmdDrawIndexed(commandBuffer, model.indexCount, 1, 0, 0, 0);
    }


    // ~Skybox()
    // {
    //     core::context().destroy(pipeline);
    //     core::context().destroy(pipelineLayout);
    // }

private:
    mutable Camera<false, true> camera;
    const asset::Texture&       texture;
    const asset::Model&         model;
    const core::Descriptor      descriptor;
    core::math::Matrix<4, 4>    matrix;

    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;
};

}  // namespace surge::entity
