#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Presenter.hpp"
#include "surge/Pipeline.hpp"
#include "surge/Camera.hpp"
#include "surge/math/matrices.hpp"

#include "surge/asset/LoadedTexture.hpp"
// #include "surge/asset/LoadedSkybox.hpp"
#include "surge/Texture.hpp"

#include "surge/asset/LoadedModel.hpp"
#include "surge/Model.hpp"
#include "surge/Descriptor.hpp"

namespace surge
{

class VikingRoom : Contextualized
{
public:
    VikingRoom(const Context& context, const Command& command, const std::filesystem::path& loadedTexture,
               const std::filesystem::path& loadedModel, const VkRenderPass renderPass,
               const std::filesystem::path& shaders)
        : Contextualized { context }
        , camera { 16.0 / 9.0, { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , uniformBuffer { ctx, sizeof(math::Matrix<4, 4>), UniformBufferInfo {} }
        , texture { ctx, command, LoadedTexture { loadedTexture }, SceneTextureInfo {} }
        , model { ctx, command, LoadedModel { loadedModel }, true, SceneModelInfo {} }
        , descriptor { ctx, 1, UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT> { uniformBuffer },
                       TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT> { texture } }
        , pipelineCache { createPipelineCache(ctx) }
        , pipelineLayout { createPipelineLayout(ctx, descriptor.setLayout) }
        , pipeline { createGraphicPipeline<LoadedModel::Vertex>(
              ctx, renderPass, pipelineCache, pipelineLayout,
              Shader { ctx, ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "shader.vert.spv" },
                       ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "shader.frag.spv" } }) }
    {
    }

    void update(const VkExtent2D extent, const UserInteraction& ui) const
    {
        camera.update(ui);

        // const math::Rotation    model { -math::deg2rad(90.0f), { 1, 0, 0 } };
        const math::Identity<4> model;
        const auto              mvp = model * camera.viewProjection();
        memcpy(uniformBuffer.mapped, &mvp, sizeof(math::Matrix<4, 4>));
    }

    void drawOffscreen(const VkCommandBuffer commandBuffer) const
    {
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

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor.set,
                                0, nullptr);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

        const std::array<VkBuffer, 1>     vertexBuffers { model.vertexBuffer.buffer };
        const std::array<VkDeviceSize, 1> offsets { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, model.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(commandBuffer, model.indexCount, 1, 0, 0, 0);
    }

    ~VikingRoom()
    {
        vkDestroyPipeline(ctx.device, pipeline, nullptr);
        ctx.destroy(pipelineLayout);
        ctx.destroy(pipelineCache);
    }

private:
    mutable Camera<true, false> camera;
    const Buffer                uniformBuffer;
    const Texture               texture;
    const Model                 model;
    const Descriptor            descriptor;

public:
    VkPipelineCache  pipelineCache;
    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;
};

}  // namespace surge
