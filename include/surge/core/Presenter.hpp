#pragma once

#include "surge/core/Command.hpp"
#include "surge/core/Swapchain.hpp"
#include "surge/core/utils/Cycle.hpp"

namespace surge::core
{

class Presenter : public Contextualized
{
public:
    Presenter(const Command& command)
        : Contextualized { command.context }
        , swapchain { std::in_place, context }
        , semaphores { createSemaphores(context, swapchain->imageCount()) }
        , frames { createFrames(command) }
        , imageIndex {}
    {
    }

    static void prepareCommandBuffer(VkCommandBuffer commandBuffer)
    {
        vkResetCommandBuffer(commandBuffer, 0);

        constexpr VkCommandBufferBeginInfo beginInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            .pInheritanceInfo = nullptr,
        };
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }
    }

    void beginRendering()
    {
        const auto& frame                 = swapchain->frames.at(imageIndex);
        const auto [fence, commandBuffer] = frames.current();

        const VkRenderingAttachmentInfo colorAttachmentInfo {
            .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext              = nullptr,
            .imageView          = frame.imageView,
            .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode        = {},
            .resolveImageView   = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue         = VkClearValue { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },
        };
        const VkRenderingAttachmentInfo depthAttachmentInfo {
            .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext              = nullptr,
            .imageView          = swapchain->depthImage.view,
            .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
            .resolveMode        = {},
            .resolveImageView   = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue         = VkClearValue { .depthStencil = { 1.0, 0 } },
        };

        const VkRenderingInfoKHR renderInfo {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .renderArea =
                VkRect2D {
                    .offset = { 0, 0 },
                    .extent = swapchain->extent,
                },
            .layerCount           = 1,
            .viewMask             = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &colorAttachmentInfo,
            .pDepthAttachment     = &depthAttachmentInfo,
            .pStencilAttachment   = VK_NULL_HANDLE,
        };

        // viewport and scissors
        const VkViewport viewport {
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = static_cast<float>(swapchain->extent.width),
            .height   = static_cast<float>(swapchain->extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        const VkRect2D scissor {
            .offset = { 0, 0 },
            .extent = swapchain->extent,
        };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        Extern::beginRendering(commandBuffer, &renderInfo);
    }

    void endRendering()
    {
        const auto& frame                 = swapchain->frames.at(imageIndex);
        const auto [fence, commandBuffer] = frames.current();

        Extern::endRendering(commandBuffer);
    }

    VkCommandBuffer acquire()
    {
        const auto [fence, commandBuffer] = frames.current();
        vkWaitForFences(context.device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(context.device, 1, &fence);

        // acquire next image
        const auto presented = semaphores.current().presented;
        const auto result    = vkAcquireNextImageKHR(context.device, swapchain->swapchain, UINT64_MAX, presented,
                                                     VK_NULL_HANDLE, &imageIndex);
        switch (result)
        {
        case VkResult::VK_SUCCESS:
        case VkResult::VK_SUBOPTIMAL_KHR:
            break;
        case VkResult::VK_ERROR_OUT_OF_DATE_KHR:
        {
            recreateSwapchain();
            break;
        }
        default:
            throw std::runtime_error("Failed to acquire swap chain image");
        }
        const auto& frame = swapchain->frames.at(imageIndex);

        // reset command buffer
        vkResetCommandBuffer(commandBuffer, 0);

        constexpr VkCommandBufferBeginInfo beginInfo {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            .pInheritanceInfo = nullptr,
        };
        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        // pipeline barrier
        const VkImageMemoryBarrier imageMemoryBarrierBegin {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = 0,
            .dstAccessMask       = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = frame.image,
            .subresourceRange =
                VkImageSubresourceRange {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
        };
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &imageMemoryBarrierBegin);

        // begin rendering
        // const VkRenderingAttachmentInfo colorAttachmentInfo {
        //     .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        //     .pNext              = nullptr,
        //     .imageView          = frame.imageView,
        //     .imageLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        //     .resolveMode        = {},
        //     .resolveImageView   = VK_NULL_HANDLE,
        //     .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        //     .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
        //     .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
        //     .clearValue         = VkClearValue { .color = { { 0.0f, 0.0f, 0.0f, 1.0f } } },
        // };
        // const VkRenderingAttachmentInfo depthAttachmentInfo {
        //     .sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        //     .pNext              = nullptr,
        //     .imageView          = swapchain->depthImage.view,
        //     .imageLayout        = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
        //     .resolveMode        = {},
        //     .resolveImageView   = VK_NULL_HANDLE,
        //     .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        //     .loadOp             = VK_ATTACHMENT_LOAD_OP_CLEAR,
        //     .storeOp            = VK_ATTACHMENT_STORE_OP_STORE,
        //     .clearValue         = VkClearValue { .depthStencil = { 1.0, 0 } },
        // };

        // const VkRenderingInfoKHR renderInfo {
        //     .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        //     .pNext = nullptr,
        //     .flags = 0,
        //     .renderArea =
        //         VkRect2D {
        //             .offset = { 0, 0 },
        //             .extent = swapchain->extent,
        //         },
        //     .layerCount           = 1,
        //     .viewMask             = 0,
        //     .colorAttachmentCount = 1,
        //     .pColorAttachments    = &colorAttachmentInfo,
        //     .pDepthAttachment     = &depthAttachmentInfo,
        //     .pStencilAttachment   = VK_NULL_HANDLE,
        // };

        // Extern::beginRendering(commandBuffer, &renderInfo);

        // // viewport and scissors
        // const VkViewport viewport {
        //     .x        = 0.0f,
        //     .y        = 0.0f,
        //     .width    = static_cast<float>(swapchain->extent.width),
        //     .height   = static_cast<float>(swapchain->extent.height),
        //     .minDepth = 0.0f,
        //     .maxDepth = 1.0f,
        // };
        // vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        // const VkRect2D scissor {
        //     .offset = { 0, 0 },
        //     .extent = swapchain->extent,
        // };
        // vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        return commandBuffer;
    }

    void present(const Command& command)
    {
        const auto& frame                 = swapchain->frames.at(imageIndex);
        const auto [fence, commandBuffer] = frames.current();

        // Extern::endRendering(commandBuffer);

        const VkImageMemoryBarrier imageMemoryBarrierEnd {
            .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext               = nullptr,
            .srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask       = 0,
            .oldLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image               = frame.image,
            .subresourceRange =
                VkImageSubresourceRange {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1,
                },
        };

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                             VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &imageMemoryBarrierEnd);


        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }

        const auto [presented, rendered] = semaphores.current();

        constexpr std::array<VkPipelineStageFlags, 1> waitStages { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        const VkSubmitInfo                            submitInfo { .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                                                   .pNext              = nullptr,
                                                                   .waitSemaphoreCount = 1,
                                                                   .pWaitSemaphores    = &presented,
                                                                   .pWaitDstStageMask  = waitStages.data(),
                                                                   .commandBufferCount = 1,
                                                                   .pCommandBuffers    = &commandBuffer,
                                                                   .signalSemaphoreCount = 1,
                                                                   .pSignalSemaphores    = &rendered };
        if (vkQueueSubmit(command.graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit to queue");
        }

        const VkPresentInfoKHR presentInfo {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext              = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores    = &rendered,
            .swapchainCount     = 1,
            .pSwapchains        = &swapchain->swapchain,
            .pImageIndices      = &imageIndex,
            .pResults           = nullptr,
        };
        if (const auto result = vkQueuePresentKHR(command.presentQueue, &presentInfo);
            result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            recreateSwapchain();
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swap chain image");
        }

        frames.rotate();
        semaphores.rotate();
    }

    ~Presenter()
    {
        for (uint32_t i = 0; i < frames.size(); ++i)
        {
            context.destroy(frames.current().fence);
            frames.rotate();
        }
        for (uint32_t i = 0; i < semaphores.size(); ++i)
        {
            context.destroy(semaphores.current().rendered);
            context.destroy(semaphores.current().presented);
            semaphores.rotate();
        }
    }

private:
    struct Semaphores
    {
        VkSemaphore presented;
        VkSemaphore rendered;
    };

    struct Frame
    {
        VkFence         fence;
        VkCommandBuffer commandBuffer;
    };

    std::optional<Swapchain> swapchain;
    utils::Cycle<Semaphores> semaphores;
    utils::Cycle<Frame>      frames;
    uint32_t                 imageIndex;

private:
    static utils::Cycle<Semaphores> createSemaphores(const Context& context, const uint32_t count)
    {
        utils::Cycle<Semaphores> semaphores { count };
        for (uint32_t i = 0; i < count; ++i)
        {
            constexpr VkSemaphoreCreateInfo semaphoreInfo {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
            };
            semaphores.set(i, Semaphores { .presented = context.create(semaphoreInfo),
                                           .rendered  = context.create(semaphoreInfo) });
        }
        return semaphores;
    }

    static utils::Cycle<Frame> createFrames(const Command& command)
    {
        const auto frameCount = command.context.frameBufferCount();

        utils::Cycle<Frame> frames { frameCount };
        for (uint32_t i = 0; i < frameCount; ++i)
        {
            frames.set(i, Frame { .fence         = command.context.create(VkFenceCreateInfo {
                                              .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                              .pNext = nullptr,
                                              .flags = VK_FENCE_CREATE_SIGNALED_BIT,
                                  }),
                                  .commandBuffer = command.createCommandBuffer() });
        }
        return frames;
    }

    void recreateSwapchain()
    {
        vkDeviceWaitIdle(context.device);
        swapchain.emplace(context);
    }
};

}  // namespace surge::core
