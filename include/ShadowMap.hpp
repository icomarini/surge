#pragma once

#include "surge/Context.hpp"
#include "surge/Buffer.hpp"
#include "surge/Descriptor.hpp"
#include "surge/math/matrices.hpp"

#include <vector>

namespace surge
{

class ShadowMap : Contextualized
{
public:
    // uint32_t pcfOff;
    // uint32_t pcfOn;

    mutable bool                shadowMap;
    mutable Camera<true, false> camera;

    Texture sceneTexture;
    Model   sceneModel;

    VkRenderPass offscreenRenderPass;

    Buffer        uniformBufferScene;
    Buffer        uniformBufferOffscreen;
    Texture       offscreenPassDepthTexture;
    VkFramebuffer offscreenFrameBuffer;

    VkDescriptorPool      descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet       debugDescriptorSet;
    VkDescriptorSet       offscreenDescriptorSet;
    VkDescriptorSet       sceneDescriptorSet;

    VkPipelineCache  pipelineCache;
    VkPipelineLayout pipelineLayout;
    VkPipeline       debugPipeline;
    VkPipeline       scenePipeline;
    VkPipeline       scenePCFPipeline;
    VkPipeline       offscreenPipeline;


    bool displayShadowMap = false;
    bool filterPCF        = true;

    // Keep depth range as small as possible
    // for better shadow map precision
    float zNear = 1.0f;
    float zFar  = 96.0f;

    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    float depthBiasConstant = 1.25f;
    // Slope depth bias factor, applied depending on polygon's slope
    float depthBiasSlope = 1.75f;

    // Vec<3> lightPos {};
    float lightFOV = 45.0f;

    struct UniformDataScene
    {
        math::Matrix<4, 4> sceneVP;
        math::Matrix<4, 4> depthMVP;
        math::Matrix<4, 4> model;
        math::Vector<4>    lightPos;
        float              zNear;
        float              zFar;
    } uniformDataScene;

    // struct UniformDataOffscreen
    // {
    //     Mat<4, 4> depth;
    // } uniformDataOffscreen;

    // 16 bits of depth is enough for such a small scene
    static constexpr VkFormat offscreenDepthFormat { VK_FORMAT_D16_UNORM };
    static constexpr uint32_t shadowMapSize { 2048 };

    // static const uint32_t pcfOff = 0;
    // static const uint32_t pcfOn  = 1;

    using UBDescription = UniformBufferDescription<VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT>;
    using TDescription  = TextureDescription<VK_SHADER_STAGE_FRAGMENT_BIT>;

    using DepthImageInfo =
        ImageInfo<VkImageCreateFlags {}, offscreenDepthFormat,
                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D>;
    using DepthTextureInfo = TextureInfo<DepthImageInfo, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL>;
    using DepthTextureDescription =
        Description<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, Texture>;


    ShadowMap(const Context& context, const Command& command, const VkRenderPass renderPass,
              const std::filesystem::path& loadedTexture, const std::filesystem::path& loadedModel,
              const std::filesystem::path& shaders)
        : Contextualized { context }
        , camera { 16.0 / 9.0, { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -1.0f } }
        , sceneTexture { ctx, command, LoadedTexture { loadedTexture }, SceneTextureInfo {} }
        , sceneModel { ctx, command, LoadedModel { loadedModel }, true, SceneModelInfo {} }
        , offscreenRenderPass { createRenderPass(ctx, offscreenDepthFormat) }
        , uniformBufferScene { ctx, sizeof(UniformDataScene), UniformBufferInfo {} }
        , uniformBufferOffscreen { ctx, sizeof(math::Matrix<4, 4>), UniformBufferInfo {} }
        , offscreenPassDepthTexture { ctx, VkExtent2D { shadowMapSize, shadowMapSize }, DepthTextureInfo {} }
        , offscreenFrameBuffer { prepareOffscreenFramebuffer(ctx, offscreenRenderPass, offscreenPassDepthTexture) }
        , descriptorPool { Descriptor::createDescriptorPool<UBDescription::type, DepthTextureDescription::type,
                                                            TDescription::type>(ctx, 3) }
        , descriptorSetLayout { Descriptor::createDescriptorSetLayout<UBDescription, DepthTextureDescription,
                                                                      TDescription>(ctx, 1) }
        , debugDescriptorSet { Descriptor::createDescriptorSet(ctx, descriptorSetLayout, descriptorPool,
                                                               UBDescription { uniformBufferScene },
                                                               DepthTextureDescription { offscreenPassDepthTexture }) }
        , offscreenDescriptorSet { Descriptor::createDescriptorSet(ctx, descriptorSetLayout, descriptorPool,
                                                                   UBDescription { uniformBufferOffscreen }) }
        , sceneDescriptorSet { Descriptor::createDescriptorSet(
              ctx, descriptorSetLayout, descriptorPool, UBDescription { uniformBufferScene },
              TDescription { sceneTexture }, DepthTextureDescription { offscreenPassDepthTexture }) }
        , pipelineCache { createPipelineCache(ctx) }
        , pipelineLayout { createPipelineLayout(ctx, descriptorSetLayout) }
        , debugPipeline { createGraphicPipeline<VoidVertex>(
              ctx, renderPass, pipelineCache, pipelineLayout,
              Shader { ctx, ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "quad.vert.spv" },
                       ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT> { shaders / "quad.frag.spv" } },
              VkPipelineRasterizationStateCreateInfo {
                  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                  .depthClampEnable        = VK_FALSE,
                  .rasterizerDiscardEnable = VK_FALSE,
                  .polygonMode             = VK_POLYGON_MODE_FILL,
                  .cullMode                = VK_CULL_MODE_NONE,
                  .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                  .depthBiasEnable         = VK_FALSE,
                  .depthBiasConstantFactor = 0.0f,
                  .depthBiasClamp          = 0.0f,
                  .depthBiasSlopeFactor    = 0.0f,
                  .lineWidth               = 1.0f,
              }) }
        , scenePipeline { createGraphicPipeline<LoadedModel::Vertex>(
              ctx, renderPass, pipelineCache, pipelineLayout,
              Shader { ctx, ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "scene.vert.spv" },
                       ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT, uint32_t> { shaders / "scene.frag.spv", 0 } }) }
        , scenePCFPipeline { createGraphicPipeline<LoadedModel::Vertex>(
              ctx, renderPass, pipelineCache, pipelineLayout,
              Shader { ctx, ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "scene.vert.spv" },
                       ShaderInfo<VK_SHADER_STAGE_FRAGMENT_BIT, uint32_t> { shaders / "scene.frag.spv", 1 } }) }
        , offscreenPipeline { createGraphicPipeline<LoadedModel::Vertex>(
              ctx, offscreenRenderPass, pipelineCache, pipelineLayout,
              Shader { ctx, ShaderInfo<VK_SHADER_STAGE_VERTEX_BIT> { shaders / "offscreen.vert.spv" } },
              VkPipelineRasterizationStateCreateInfo {
                  .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                  .depthClampEnable        = VK_FALSE,
                  .rasterizerDiscardEnable = VK_FALSE,
                  .polygonMode             = VK_POLYGON_MODE_FILL,
                  .cullMode                = VK_CULL_MODE_NONE,
                  .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                  .depthBiasEnable         = VK_TRUE,
                  .depthBiasConstantFactor = 0.0f,
                  .depthBiasClamp          = 0.0f,
                  .depthBiasSlopeFactor    = 0.0f,
                  .lineWidth               = 1.0f,
              },
              VkPipelineColorBlendStateCreateInfo {
                  .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                  .logicOpEnable   = VK_FALSE,
                  .logicOp         = VK_LOGIC_OP_COPY,
                  .attachmentCount = 0,
                  .pAttachments    = nullptr,
                  .blendConstants  = { 0.0f, 0.0f, 0.0f, 0.0f },
              },
              std::array<VkDynamicState, 3> { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
                                              VK_DYNAMIC_STATE_DEPTH_BIAS }) }
    {
    }

    void update(const VkExtent2D extent, const UserInteraction& ui) const
    {
        camera.update(ui);

        const math::Rotation          model { -math::deg2rad(90.0f), { 1, 0, 0 } };
        const math::View              depthV { ui.lightPos, { 0, 0, 0 }, { 0, 1, 0 } };
        const math::Perspective<true> depthP { math::deg2rad(lightFOV), 1.0f, zNear, zFar };
        const math::Matrix<4, 4>      depthMVP { model * depthV * depthP };
        memcpy(uniformBufferOffscreen.mapped, &depthMVP, sizeof(math::Matrix<4, 4>));

        const math::Vector<4>  light { ui.lightPos[0], ui.lightPos[1], ui.lightPos[2], 1.0f };
        const UniformDataScene uniformDataScene {
            .sceneVP  = camera.viewProjection(),
            .depthMVP = depthMVP,
            .model    = math::Identity<4> {} * model,
            .lightPos = light,
            .zNear    = zNear,
            .zFar     = zFar,
        };
        memcpy(uniformBufferScene.mapped, &uniformDataScene, sizeof(uniformDataScene));

        shadowMap = ui.shadowMap;
    }

    void drawOffscreen(const VkCommandBuffer commandBuffer) const
    {
        // if (!shadowMap)
        // {
        //     return;
        // }
        const VkRect2D renderArea {
            .offset = { 0, 0 },
            .extent = { shadowMapSize, shadowMapSize },
        };
        VkClearValue clearValue {};
        clearValue.color        = { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValue.depthStencil = { 1.0f, 0 };
        const VkRenderPassBeginInfo renderPassInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass      = offscreenRenderPass,
            .framebuffer     = offscreenFrameBuffer,
            .renderArea      = renderArea,
            .clearValueCount = 1,
            .pClearValues    = &clearValue,
        };
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        const VkViewport viewport {
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = static_cast<float>(shadowMapSize),
            .height   = static_cast<float>(shadowMapSize),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        const VkRect2D scissor {
            .offset = { 0, 0 },
            .extent = { shadowMapSize, shadowMapSize },
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdSetDepthBias(commandBuffer, depthBiasConstant, 0.0f, depthBiasSlope);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, offscreenPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &offscreenDescriptorSet, 0, nullptr);

        const std::array<VkBuffer, 1>     vertexBuffers { sceneModel.vertexBuffer.buffer };
        const std::array<VkDeviceSize, 1> offsets { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
        vkCmdBindIndexBuffer(commandBuffer, sceneModel.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(commandBuffer, sceneModel.indexCount, 1, 0, 0, 0);

#if 0
        clearValues[0].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo    = vks::initializers::renderPassBeginInfo();
        renderPassBeginInfo.renderPass               = offscreenPass.renderPass;
        renderPassBeginInfo.framebuffer              = offscreenPass.frameBuffer;
        renderPassBeginInfo.renderArea.extent.width  = offscreenPass.width;
        renderPassBeginInfo.renderArea.extent.height = offscreenPass.height;
        renderPassBeginInfo.clearValueCount          = 1;
        renderPassBeginInfo.pClearValues             = clearValues;

        vkCmdBeginRenderPass(drawCmdBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        viewport = vks::initializers::viewport((float)offscreenPass.width, (float)offscreenPass.height, 0.0f, 1.0f);
        vkCmdSetViewport(drawCmdBuffers[i], 0, 1, &viewport);

        scissor = vks::initializers::rect2D(offscreenPass.width, offscreenPass.height, 0, 0);
        vkCmdSetScissor(drawCmdBuffers[i], 0, 1, &scissor);

        // Set depth bias (aka "Polygon offset")
        // Required to avoid shadow mapping artifacts
        vkCmdSetDepthBias(drawCmdBuffers[i], depthBiasConstant, 0.0f, depthBiasSlope);

        vkCmdBindPipeline(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelines.offscreen);
        vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &descriptorSets.offscreen, 0, nullptr);
        scenes[sceneIndex].draw(drawCmdBuffers[i]);

#endif
        vkCmdEndRenderPass(commandBuffer);
    }

    void draw(const VkCommandBuffer commandBuffer, const VkExtent2D extent) const
    {
        // if (!shadowMap)
        // {
        //     return;
        // }

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

        if (!shadowMap)
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                    &sceneDescriptorSet, 0, nullptr);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, scenePipeline);

            const std::array<VkBuffer, 1>     vertexBuffers { sceneModel.vertexBuffer.buffer };
            const std::array<VkDeviceSize, 1> offsets { 0 };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());
            vkCmdBindIndexBuffer(commandBuffer, sceneModel.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdDrawIndexed(commandBuffer, sceneModel.indexCount, 1, 0, 0, 0);
        }
        else
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                    &debugDescriptorSet, 0, nullptr);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, debugPipeline);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }
    }

    ~ShadowMap()
    {
        ctx.destroy(offscreenPipeline);
        ctx.destroy(scenePCFPipeline);
        ctx.destroy(scenePipeline);
        ctx.destroy(debugPipeline);
        ctx.destroy(pipelineLayout);
        ctx.destroy(pipelineCache);
        ctx.destroy(offscreenRenderPass);
        ctx.destroy(offscreenFrameBuffer);
        ctx.destroy(descriptorPool);
        ctx.destroy(descriptorSetLayout);
    }

    static VkRenderPass createRenderPass(const Context& ctx, const VkFormat offscreenDepthFormat)
    {
        const VkAttachmentDescription colorAttachment {
            .format         = offscreenDepthFormat,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        };

        const VkAttachmentReference depthReference {
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        const VkSubpassDescription subpass {
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = 0,
            .pDepthStencilAttachment = &depthReference,
        };

        constexpr std::array dependencies {
            VkSubpassDependency {
                .srcSubpass      = VK_SUBPASS_EXTERNAL,
                .dstSubpass      = 0,
                .srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask   = VK_ACCESS_SHADER_READ_BIT,
                .dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            },
            VkSubpassDependency {
                .srcSubpass      = 0,
                .dstSubpass      = VK_SUBPASS_EXTERNAL,
                .srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                .dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstAccessMask   = VK_ACCESS_SHADER_READ_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            },
        };

        const VkRenderPassCreateInfo renderPassInfo {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &colorAttachment,
            .subpassCount    = 1,
            .pSubpasses      = &subpass,
            .dependencyCount = static_cast<uint32_t>(dependencies.size()),
            .pDependencies   = dependencies.data(),
        };
        return ctx.create<VkRenderPass>(&renderPassInfo);
    }

    static VkSampler createSampler(const Context& ctx, const VkFormat offscreenDepthFormat)
    {
        const VkFilter shadowmapFilter = ctx.formatIsFilterable(offscreenDepthFormat, VK_IMAGE_TILING_OPTIMAL) ?
                                             VK_FILTER_LINEAR :
                                             VK_FILTER_NEAREST;

        const VkSamplerCreateInfo samplerInfo {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter               = shadowmapFilter,
            .minFilter               = shadowmapFilter,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias              = 0.0f,
            .anisotropyEnable        = VK_TRUE,
            .maxAnisotropy           = ctx.physicalDeviceProperties.limits.maxSamplerAnisotropy,
            .minLod                  = 0.0f,
            .maxLod                  = 1.0f,
            .borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
            .unnormalizedCoordinates = VK_FALSE,
        };

        return ctx.create<VkSampler>(&samplerInfo);
    }

    static VkFramebuffer prepareOffscreenFramebuffer(const Context& ctx, const VkRenderPass offscreenPassrenderPass,
                                                     const Texture& offscreenDepthTexture)
    {
        const VkFramebufferCreateInfo frameBufferInfo {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = offscreenPassrenderPass,
            .attachmentCount = 1,
            .pAttachments    = &offscreenDepthTexture.image.view,
            .width           = offscreenDepthTexture.image.extent.width,
            .height          = offscreenDepthTexture.image.extent.height,
            .layers          = 1,
        };
        return ctx.create<VkFramebuffer>(&frameBufferInfo);
    }
};

}  // namespace surge
