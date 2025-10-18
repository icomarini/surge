#pragma once

#include "surge/core/Context.hpp"

namespace surge::core
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

class Image
{
public:
    struct Data
    {
        VkImageCreateFlags    imageCreateFlags;
        VkFormat              format;
        VkImageUsageFlags     imageUsageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
        VkImageAspectFlags    imageAspectFlags;
        VkImageViewType       imageViewType;
    };

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

    Image(const VkExtent2D& extent, const Data& data)
        : extent { extent }
        , image { createImage(extent, 1, 1, data) }
        , memory { createImageMemory(image, data) }
        , view { createImageView(image, 1, 1, data) }
    {
    }

    template<typename LoadedTexture>
    Image(const LoadedTexture& loadedTexture, const Data& data)
        : extent { loadedTexture.width, loadedTexture.height }
        , image { createImage(extent, loadedTexture.mipLevels, loadedTexture.arrayLayers, data) }
        , memory { createImageMemory(image, data) }
        , view { createImageView(image, loadedTexture.mipLevels, loadedTexture.arrayLayers, data) }
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
    static VkImage createImage(const VkExtent2D& extent, const uint32_t mipLevels, const uint32_t arrayLayers,
                               const Data& data)
    {
        return context().create(VkImageCreateInfo {
            .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext                 = nullptr,
            .flags                 = data.imageCreateFlags,
            .imageType             = VK_IMAGE_TYPE_2D,
            .format                = data.format,
            .extent                = { extent.width, extent.height, 1 },
            .mipLevels             = mipLevels,
            .arrayLayers           = arrayLayers,
            .samples               = VK_SAMPLE_COUNT_1_BIT,
            .tiling                = VK_IMAGE_TILING_OPTIMAL,
            .usage                 = data.imageUsageFlags,
            .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices   = nullptr,
            .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
        });
    }

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
            throw std::runtime_error("Failed to bind image memory");
        }
        return memory;
    }

    static VkDeviceMemory createImageMemory(const VkImage image, const Data& data)
    {
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(context().device, image, &memRequirements);
        const auto memory = context().create(VkMemoryAllocateInfo {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext           = nullptr,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = context().findMemoryType(data.memoryPropertyFlags, memRequirements.memoryTypeBits),
        });
        if (vkBindImageMemory(context().device, image, memory, 0) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to bind image memory");
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

    static VkImageView createImageView(const VkImage image, const uint32_t mipLevels, const uint32_t arrayLayers,
                                       const Data& data)
    {
        const VkImageSubresourceRange subresourceRange {
            .aspectMask     = data.imageAspectFlags,
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
            .viewType         = data.imageViewType,
            .format           = data.format,
            .components       = {},
            .subresourceRange = subresourceRange,
        });
    }
};

}  // namespace surge::core
