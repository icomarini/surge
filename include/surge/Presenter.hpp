#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Cycle.hpp"
#include "surge/Swapchain.hpp"
#include "surge/Image.hpp"

namespace surge
{

class Presenter
{
public:
    using DepthImageInfo =
        ImageInfo<VkImageCreateFlags {}, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D>;

    Presenter(const Command& command)
        : renderPass { command.renderPass }
        , swapchain { std::in_place, renderPass, DepthImageInfo {} }
        , semaphores { createSemaphores(swapchain->imageCount()) }
        , frames { createFrames(command) }
        , imageIndex {}
    {
    }

    std::tuple<VkExtent2D, VkFramebuffer, VkCommandBuffer> acquire(const VkRenderPass renderPass)
    {
        const auto [fence, commandBuffer] = frames.current();
        vkWaitForFences(context().device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(context().device, 1, &fence);

        const auto presented = semaphores.current().presented;
        const auto result    = vkAcquireNextImageKHR(context().device, swapchain->swapchain, UINT64_MAX, presented,
                                                     VK_NULL_HANDLE, &imageIndex);
        switch (result)
        {
        case VkResult::VK_SUCCESS:
        case VkResult::VK_SUBOPTIMAL_KHR:
            break;
        case VkResult::VK_ERROR_OUT_OF_DATE_KHR:
        {
            recreateSwapchain(renderPass);
            break;
        }
        default:
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        return { swapchain->extent, swapchain->frameBuffer(imageIndex), commandBuffer };
    }

    void present(const Command& command, const bool framebufferResized)
    {
        const auto [fence, commandBuffer] = frames.current();
        const auto [presented, rendered]  = semaphores.current();

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
            throw std::runtime_error("failed to submit to queue!");
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
            result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            recreateSwapchain(command.renderPass);
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        frames.rotate();
        semaphores.rotate();
    }

    ~Presenter()
    {
        for (uint32_t i = 0; i < frames.size(); ++i)
        {
            context().destroy(frames.current().fence);
            frames.rotate();
        }
        for (uint32_t i = 0; i < semaphores.size(); ++i)
        {
            context().destroy(semaphores.current().rendered);
            context().destroy(semaphores.current().presented);
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

    VkRenderPass             renderPass;
    std::optional<Swapchain> swapchain;
    Cycle<Semaphores>        semaphores;
    Cycle<Frame>             frames;
    uint32_t                 imageIndex;

private:
    static Cycle<Semaphores> createSemaphores(const uint32_t count)
    {
        Cycle<Semaphores> semaphores { count };
        for (uint32_t i = 0; i < count; ++i)
        {
            constexpr VkSemaphoreCreateInfo semaphoreInfo {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = {},
            };
            semaphores.set(i, Semaphores { .presented = context().create(semaphoreInfo),
                                           .rendered  = context().create(semaphoreInfo) });
        }
        return semaphores;
    }

    static Cycle<Frame> createFrames(const Command& command)
    {
        const auto frameCount = context().frameBufferCount();

        Cycle<Frame> frames { frameCount };
        for (uint32_t i = 0; i < frameCount; ++i)
        {
            frames.set(
                i, Frame { .fence = context().create(VkFenceCreateInfo { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                                                         .pNext = nullptr,
                                                                         .flags = VK_FENCE_CREATE_SIGNALED_BIT }),
                           .commandBuffer = command.createCommandBuffer() });
        }
        return frames;
    }

    void recreateSwapchain(const VkRenderPass renderPass)
    {
        vkDeviceWaitIdle(context().device);
        swapchain.emplace(renderPass, DepthImageInfo {});
    }
};

}  // namespace surge
