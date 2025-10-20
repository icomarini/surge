#pragma once

#include "surge/core/Context.hpp"

namespace surge::core
{
class Image : public Contextualized
{
public:
    template<VkImageCreateFlags    create,     //
             VkFormat              fmt,        //
             VkImageUsageFlags     usage,      //
             VkMemoryPropertyFlags propertry,  //
             VkImageAspectFlags    aspect,     //
             VkImageViewType       view        //
             >
    struct Info
    {
        static constexpr auto imageCreateFlags    = create;
        static constexpr auto format              = fmt;
        static constexpr auto imageUsageFlags     = usage;
        static constexpr auto memoryPropertyFlags = propertry;
        static constexpr auto imageAspectFlags    = aspect;
        static constexpr auto imageViewType       = view;
    };

    static constexpr auto texture2d = Info<VkImageCreateFlags {},                                         //
                                           VK_FORMAT_R8G8B8A8_SRGB,                                       //
                                           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,  //
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                           //
                                           VK_IMAGE_ASPECT_COLOR_BIT,                                     //
                                           VK_IMAGE_VIEW_TYPE_2D> {};

    static constexpr auto cube = Info<VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,                           //
                                      VK_FORMAT_R8G8B8A8_SRGB,                                       //
                                      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,  //
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                           //
                                      VK_IMAGE_ASPECT_COLOR_BIT,                                     //
                                      VK_IMAGE_VIEW_TYPE_CUBE> {};

    static constexpr auto depth = Info<VkImageCreateFlags {},                        //
                                       VK_FORMAT_D32_SFLOAT,                         //
                                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,  //
                                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,          //
                                       VK_IMAGE_ASPECT_DEPTH_BIT,                    //
                                       VK_IMAGE_VIEW_TYPE_2D> {};

    template<typename LoadedTexture, typename I>
    Image(const Context& context, const LoadedTexture& loadedTexture, I)
        : Contextualized { context }
        , extent { loadedTexture.width, loadedTexture.height }
        , image { createImage<I::imageCreateFlags, I::format, I::imageUsageFlags>(
              context, extent, loadedTexture.mipLevels, loadedTexture.arrayLayers) }
        , memory { createImageMemory<I::memoryPropertyFlags>(context, image) }
        , view { createImageView<I::imageAspectFlags, I::imageViewType, I::format>(
              context, image, loadedTexture.mipLevels, loadedTexture.arrayLayers) }
    {
    }

    template<typename I>
    Image(const Context& context, const VkExtent2D& extent, I)
        : Contextualized { context }
        , extent { extent }
        , image { createImage<I::imageCreateFlags, I::format, I::imageUsageFlags>(context, extent, 1, 1) }
        , memory { createImageMemory<I::memoryPropertyFlags>(context, image) }
        , view { createImageView<I::imageAspectFlags, I::imageViewType, I::format>(context, image, 1, 1) }
    {
    }

    ~Image()
    {
        context.destroy(view);
        context.destroy(memory);
        context.destroy(image);
    }

public:
    VkExtent2D     extent;
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;

private:
    template<VkImageCreateFlags imageCreateFlags, VkFormat format, VkImageUsageFlags imageUsageFlags>
    static VkImage createImage(const Context& context, const VkExtent2D& extent, const uint32_t mipLevels,
                               const uint32_t arrayLayers)
    {
        return context.create(VkImageCreateInfo {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = imageCreateFlags,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = format,
            .extent                = { extent.width, extent.height, 1 },
            .mipLevels             = mipLevels,
            .arrayLayers           = arrayLayers,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = imageUsageFlags,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
        });
    }

    template<VkMemoryPropertyFlags memoryPropertyFlags>
    static VkDeviceMemory createImageMemory(const Context& context, const VkImage image)
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(context.device, image, &memRequirements);
        const auto memory = context.create(VkMemoryAllocateInfo {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = context.findMemoryType<memoryPropertyFlags>(memRequirements.memoryTypeBits),
        });
        if (vkBindImageMemory(context.device, image, memory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bind image memory");
        }
        return memory;
    }

    template<VkImageAspectFlags imageAspectFlags, VkImageViewType imageViewType, VkFormat format>
    static VkImageView createImageView(const Context& context, const VkImage image, const uint32_t mipLevels,
                                       const uint32_t arrayLayers)
    {
        const VkImageSubresourceRange subresourceRange {
            .aspectMask     = imageAspectFlags,
            .baseMipLevel   = 0,
            .levelCount     = mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = arrayLayers,
        };
        return context.create(VkImageViewCreateInfo {
            .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext            = nullptr,
            .flags            = {},
            .image            = image,
            .viewType         = imageViewType,
            .format           = format,
            .components       = {},
            .subresourceRange = subresourceRange,
        });
    }
};

}  // namespace surge::core
