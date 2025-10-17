#pragma once

#include "surge/asset/Texture.hpp"
#include "surge/asset/Model.hpp"
#include "surge/core/geometry/shapes.hpp"
#include "surge/core/input/input.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/load/LoadedTexture.hpp"

namespace surge
{

class Skybox
{
    using CubeImageInfo =
        core::ImageInfo<VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE>;
    using CubeTextureInfo = asset::TextureInfo<CubeImageInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>;

public:
    Skybox(const core::Command& command, const load::LoadedTexture::Handle& handle)
        : camera { 16.0 / 9.0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }
        , uniformBuffer { sizeof(core::math::Matrix<4, 4>), core::UniformBufferInfo {} }
        , texture { command, load::LoadedTexture { handle }, CubeTextureInfo {} }
        , model { command, core::geometry::cubeFill, true, asset::SceneModelInfo {} }
        , descriptor { 1, core::UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT> { uniformBuffer },
                       core::Description<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT,
                                         asset::Texture> { texture } }
        , pipelineLayout { core::createPipelineLayout(descriptor.setLayout) }
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

    void update(const VkExtent2D, const UserInteraction& ui) const
    {
        update({}, ui);
    }

    void update(const UserInteraction& ui) const
    {
        camera.update(ui);
        const auto viewProjection = camera.mats.perspective * camera.mats.view;
        memcpy(uniformBuffer.mapped, &viewProjection, sizeof(core::math::Matrix<4, 4>));
    }

    void drawOffscreen(const VkCommandBuffer) const
    {
    }

    void draw(const VkCommandBuffer commandBuffer) const
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        constexpr std::array<VkDeviceSize, 1> offsets { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor.set,
                                0, nullptr);

        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(core::context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);

        vkCmdDrawIndexed(commandBuffer, model.indexCount, 1, 0, 0, 0);
    }


    ~Skybox()
    {
        core::context().destroy(pipeline);
        core::context().destroy(pipelineLayout);
    }

private:
    mutable Camera<false, true> camera;
    const core::Buffer          uniformBuffer;
    const asset::Texture        texture;
    const asset::Model          model;
    const core::Descriptor      descriptor;

    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;
};

}  // namespace surge
