#pragma once

#include "surge/Context.hpp"
#include "surge/Image.hpp"

namespace surge
{

class Swapchain
{
public:
    template<typename DepthImageInfo>
    Swapchain(DepthImageInfo)
        : extent { computeExtent() }
        , swapchain { createSwapChain(extent) }
        , depthImage { extent, DepthImageInfo {} }
        , frames { createFrames(swapchain) }
    {
    }

    uint32_t imageCount() const
    {
        return frames.size();
    }

    ~Swapchain()
    {
        for (const auto [_, imageView] : frames)
        {
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
        VkImage     image;
        VkImageView imageView;
    };

public:
    Image depthImage;

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
        const std::array indices { context().physicalDevice.graphicsFamilyIndex,
                                   context().physicalDevice.presentFamilyIndex };
        const auto       sameQueueFamilies = indices.at(0) == indices.at(1);

        constexpr VkSwapchainPresentScalingCreateInfoEXT presentScaling {
            .sType           = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_SCALING_CREATE_INFO_EXT,
            .pNext           = nullptr,
            .scalingBehavior = 0,
            .presentGravityX = 0,
            .presentGravityY = 0,
        };

        return context().create(VkSwapchainCreateInfoKHR {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext                 = &presentScaling,
            .flags                 = {},
            .surface               = context().surface,
            .minImageCount         = context().frameBufferCount(),
            .imageFormat           = context().physicalDevice.surfaceFormat.format,
            .imageColorSpace       = context().physicalDevice.surfaceFormat.colorSpace,
            .imageExtent           = extent,
            .imageArrayLayers      = 1,
            .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode      = sameQueueFamilies ? VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT,
            .queueFamilyIndexCount = sameQueueFamilies ? 0U : static_cast<uint32_t>(indices.size()),
            .pQueueFamilyIndices   = sameQueueFamilies ? nullptr : indices.data(),
            .preTransform          = context().getSurfaceCapabilities().currentTransform,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = context().physicalDevice.presentMode,
            .clipped               = VK_TRUE,
            .oldSwapchain          = VK_NULL_HANDLE,
        });
    }

    static std::vector<Frame> createFrames(const VkSwapchainKHR swapchain)
    {
        uint32_t count {};
        vkGetSwapchainImagesKHR(context().device, swapchain, &count, VK_NULL_HANDLE);
        std::vector<VkImage> images(count);
        vkGetSwapchainImagesKHR(context().device, swapchain, &count, images.data());

        std::vector<Frame> frames;
        frames.reserve(count);
        for (const auto image : images)
        {
            // constexpr VkImageSubresourceRange subresourceRange {
            //     .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            //     .baseMipLevel   = 0,
            //     .levelCount     = 1,
            //     .baseArrayLayer = 0,
            //     .layerCount     = 1,
            // };
            const auto imageView = context().create(VkImageViewCreateInfo {
                .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext      = nullptr,
                .flags      = {},
                .image      = image,
                .viewType   = VK_IMAGE_VIEW_TYPE_2D,
                .format     = context().physicalDevice.surfaceFormat.format,
                .components = {},
                .subresourceRange =
                    VkImageSubresourceRange {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
            });
            frames.emplace_back(image, imageView);
        }
        return frames;
    }
};

}  // namespace surge
