#pragma once

#include "surge/Context.hpp"

namespace surge
{

template<VkImageCreateFlags    _imageCreateFlags,     //
         VkFormat              _format,               //
         VkImageUsageFlags     _imageUsageFlags,      //
         VkMemoryPropertyFlags _memoryPropertyFlags,  //
         VkImageAspectFlags    _imageAspectFlags,     //
         VkImageViewType       _imageViewType         //
         >
struct ImageInfo
{
    static constexpr auto imageCreateFlags    = _imageCreateFlags;
    static constexpr auto format              = _format;
    static constexpr auto imageUsageFlags     = _imageUsageFlags;
    static constexpr auto memoryPropertyFlags = _memoryPropertyFlags;
    static constexpr auto imageAspectFlags    = _imageAspectFlags;
    static constexpr auto imageViewType       = _imageViewType;
};

using SceneImageInfo = ImageInfo<VkImageCreateFlags {}, VK_FORMAT_R8G8B8A8_SRGB,
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D>;

class Image
{
public:
    template<typename LoadedTexture, typename Info>
    Image(const LoadedTexture& loadedTexture, Info)
        : extent { loadedTexture.width, loadedTexture.height }
        , image { createImage<Info::imageCreateFlags, Info::format, Info::imageUsageFlags>(
              extent, loadedTexture.mipLevels, loadedTexture.arrayLayers) }
        , memory { createImageMemory<Info::memoryPropertyFlags>(image) }
        , view { createImageView<Info::imageAspectFlags, Info::imageViewType, Info::format>(
              image, loadedTexture.mipLevels, loadedTexture.arrayLayers) }
    {
    }

    template<typename Info>
    Image(const VkExtent2D& extent, Info)
        : extent { extent }
        , image { createImage<Info::imageCreateFlags, Info::format, Info::imageUsageFlags>(extent, 1, 1) }
        , memory { createImageMemory<Info::memoryPropertyFlags>(image) }
        , view { createImageView<Info::imageAspectFlags, Info::imageViewType, Info::format>(image, 1, 1) }
    {
    }

    ~Image()
    {
        context().destroy(view);
        context().destroy(memory);
        context().destroy(image);
    }

public:
    VkExtent2D     extent;
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;

private:
    template<VkImageCreateFlags imageCreateFlags, VkFormat format, VkImageUsageFlags imageUsageFlags>
    static VkImage createImage(const VkExtent2D& extent, const uint32_t mipLevels, const uint32_t arrayLayers)
    {
        return context().create(VkImageCreateInfo {
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
    static VkDeviceMemory createImageMemory(const VkImage image)
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(context().device, image, &memRequirements);
        const auto memory = context().create(VkMemoryAllocateInfo {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = context().findMemoryType<memoryPropertyFlags>(memRequirements.memoryTypeBits),
        });
        if (vkBindImageMemory(context().device, image, memory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to bind image memory!");
        }
        return memory;
    }

    template<VkImageAspectFlags imageAspectFlags, VkImageViewType imageViewType, VkFormat format>
    static VkImageView createImageView(const VkImage image, const uint32_t mipLevels, const uint32_t arrayLayers)
    {
        const VkImageSubresourceRange subresourceRange {
            .aspectMask     = imageAspectFlags,
            .baseMipLevel   = 0,
            .levelCount     = mipLevels,
            .baseArrayLayer = 0,
            .layerCount     = arrayLayers,
        };
        return context().create(VkImageViewCreateInfo {
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

}  // namespace surge
