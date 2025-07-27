#pragma once

#include "surge/Context.hpp"
#include "surge/Descriptor.hpp"
#include "surge/Texture.hpp"
// #include "surge/asset/LoadedSkybox.hpp"
#include "surge/Pipeline.hpp"
#include "surge/geometry/shapes.hpp"

namespace surge
{

class Skybox
{
    using CubeImageInfo =
        ImageInfo<VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_FORMAT_R8G8B8A8_SRGB,
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_CUBE>;
    using CubeTextureInfo = TextureInfo<CubeImageInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>;

public:
    Skybox(const Command& command, const std::filesystem::path& shaders, const std::filesystem::path& loadedTexture)
        : camera { 16.0 / 9.0, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } }
        , uniformBuffer { sizeof(math::Matrix<4, 4>), UniformBufferInfo {} }
        , texture { command, LoadedTexture { loadedTexture }, CubeTextureInfo {} }
        , model { command, geometry::cubeFill, true, SceneModelInfo {} }
        , descriptor { 1, UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT> { uniformBuffer },
                       Description<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, Texture> {
                           texture } }
        , pipelineLayout { createPipelineLayout(descriptor.setLayout) }
        , pipeline { createGraphicPipeline(
              geometry::createVertexInputState<geometry::Position>(), VK_NULL_HANDLE, pipelineLayout,
              Shader { ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "skybox.vert.spv", nullptr },
                       ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "skybox.frag.spv", nullptr } },
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
        camera.update(ui);
        const auto viewProjection = camera.viewProjection();
        memcpy(uniformBuffer.mapped, &viewProjection, sizeof(math::Matrix<4, 4>));
    }


    void drawOffscreen(const VkCommandBuffer) const
    {
    }

    void draw(const VkCommandBuffer commandBuffer, const VkExtent2D extent) const
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        constexpr std::array<VkDeviceSize, 1> offsets { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &model.vertexBuffer.buffer, offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor.set,
                                0, nullptr);

        auto setPolygonMode = reinterpret_cast<PFN_vkCmdSetPolygonModeEXT>(
            vkGetInstanceProcAddr(context().instance, "vkCmdSetPolygonModeEXT"));
        assert(setPolygonMode);
        setPolygonMode(commandBuffer, VK_POLYGON_MODE_FILL);

        vkCmdDrawIndexed(commandBuffer, model.indexCount, 1, 0, 0, 0);
    }


    ~Skybox()
    {
        context().destroy(pipeline);
        context().destroy(pipelineLayout);
    }

private:
    mutable Camera<false, true> camera;
    const Buffer                uniformBuffer;
    const Texture               texture;
    const Model                 model;
    const Descriptor            descriptor;

    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;
};

}  // namespace surge
