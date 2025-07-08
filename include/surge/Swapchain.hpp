#pragma once

#include "surge/Context.hpp"
#include "surge/Image.hpp"

namespace surge
{

class Swapchain
{
public:
    template<typename DepthImageInfo>
    Swapchain(const VkRenderPass renderPass, DepthImageInfo)
        : extent { computeExtent() }
        , swapchain { createSwapChain(extent) }
        , depthImage { extent, DepthImageInfo {} }
        , frames { createFrames(swapchain, extent, depthImage.view, renderPass) }
    {
    }

    uint32_t imageCount() const
    {
        return frames.size();
    }

    VkFramebuffer frameBuffer(const uint32_t imageIndex) const
    {
        return frames.at(imageIndex).framebuffer;
    }

    ~Swapchain()
    {
        for (const auto [imageView, frameBuffer] : frames)
        {
            context().destroy(frameBuffer);
            context().destroy(imageView);
        }
        context().destroy(swapchain);
    }

public:
    VkExtent2D     extent;
    VkSwapchainKHR swapchain;

private:
    struct Frame
    {
        VkImageView   imageView;
        VkFramebuffer framebuffer;
    };

    Image              depthImage;
    std::vector<Frame> frames;

private:
    static VkExtent2D computeExtent()
    {
        const auto surfaceCapabilities = context().getSurfaceCapabilities();
        const auto extent              = context().extent();
        return surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() ?
                   surfaceCapabilities.currentExtent :
                   VkExtent2D { std::clamp(extent.width, surfaceCapabilities.minImageExtent.width,
                                           surfaceCapabilities.maxImageExtent.width),
                                std::clamp(extent.height, surfaceCapabilities.minImageExtent.height,
                                           surfaceCapabilities.maxImageExtent.height) };
    }

    static VkSwapchainKHR createSwapChain(const VkExtent2D extent)
    {
        const std::array indices { context().properties.graphicsFamilyIndex, context().properties.presentFamilyIndex };
        const auto       sameQueueFamilies = indices.at(0) == indices.at(1);
        return context().create(VkSwapchainCreateInfoKHR {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext                 = nullptr,
            .flags                 = {},
            .surface               = context().surface,
            .minImageCount         = context().frameBufferCount(),
            .imageFormat           = context().properties.surfaceFormat.format,
            .imageColorSpace       = context().properties.surfaceFormat.colorSpace,
            .imageExtent           = extent,
            .imageArrayLayers      = 1,
            .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = sameQueueFamilies ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = sameQueueFamilies ? 0U : static_cast<uint32_t>(indices.size()),
            .pQueueFamilyIndices   = sameQueueFamilies ? nullptr : indices.data(),
            .preTransform          = context().getSurfaceCapabilities().currentTransform,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = context().properties.presentMode,
            .clipped               = VK_TRUE,
            .oldSwapchain          = VK_NULL_HANDLE,
        });
    }

    static std::vector<Frame> createFrames(const VkSwapchainKHR swapchain, const VkExtent2D extent,
                                           const VkImageView depthImageView, const VkRenderPass renderPass)
    {
        uint32_t count {};
        vkGetSwapchainImagesKHR(context().device, swapchain, &count, VK_NULL_HANDLE);
        std::vector<VkImage> images(count);
        vkGetSwapchainImagesKHR(context().device, swapchain, &count, images.data());

        std::vector<Frame> frames;
        frames.reserve(count);
        for (const auto image : images)
        {
            constexpr VkImageSubresourceRange subresourceRange {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            };
            const auto       imageView = context().create(VkImageViewCreateInfo {
                      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                      .pNext            = nullptr,
                      .flags            = {},
                      .image            = image,
                      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
                      .format           = context().properties.surfaceFormat.format,
                      .components       = {},
                      .subresourceRange = subresourceRange,
            });
            const std::array attachments { imageView, depthImageView };
            const auto       frameBuffer = context().create(VkFramebufferCreateInfo {
                      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                      .pNext           = nullptr,
                      .flags           = {},
                      .renderPass      = renderPass,
                      .attachmentCount = static_cast<uint32_t>(attachments.size()),
                      .pAttachments    = attachments.data(),
                      .width           = extent.width,
                      .height          = extent.height,
                      .layers          = 1,
            });
            frames.emplace_back(imageView, frameBuffer);
        }
        return frames;
    }
};

}  // namespace surge
