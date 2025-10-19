#pragma once

#include "surge/core/Command.hpp"
#include "surge/core/Image.hpp"
#include "surge/core/Descriptor.hpp"

namespace surge::asset
{


struct Sampler
{
    VkFilter             magFilter;
    VkFilter             minFilter;
    VkSamplerMipmapMode  mipmapMode;
    VkSamplerAddressMode addressModeU;
    VkSamplerAddressMode addressModeV;
    VkSamplerAddressMode addressModeW;
};

class Texture
{
public:
    enum class Type
    {
        texture2d = 0,
        cube,
    };

    template<typename ImageData, VkImageLayout layout>
    struct Info
    {
        static constexpr auto image       = ImageData {};
        static constexpr auto imageLayout = layout;
    };

    static constexpr auto texture2d = Info<decltype(core::Image::texture2d),  //
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL> {};

    static constexpr auto cube = Info<decltype(core::Image::cube),  //
                                      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL> {};

    template<typename LoadedTexture, typename I>
    Texture(const core::Command& command, const LoadedTexture& loadedTexture, const Sampler& sampler, I)
        : name { loadedTexture.name }
        , image { loadedTexture, I::image }
        , sampler { createSampler(sampler) }
        , info { .sampler = this->sampler, .imageView = image.view, .imageLayout = I::imageLayout }
    {
        command.transferImage(image.image, loadedTexture);
    }

    const VkDescriptorImageInfo* imageInfo() const
    {
        return &info;
    }

    const VkDescriptorBufferInfo* bufferInfo() const
    {
        return nullptr;
    }

    ~Texture()
    {
        core::context().destroy(sampler);
    }

public:
    const std::string           name;
    const core::Image           image;
    const VkSampler             sampler;
    const VkDescriptorImageInfo info;

private:
    static VkSampler createSampler()
    {
        return core::context().create(VkSamplerCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext                   = nullptr,
            .flags                   = {},
            .magFilter               = VK_FILTER_LINEAR,
            .minFilter               = VK_FILTER_LINEAR,
            .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .mipLodBias              = 0.0f,
            .anisotropyEnable        = VK_TRUE,
            .maxAnisotropy           = core::context().physicalDevice.maxSamplerAnisotropy,
            .compareEnable           = false,
            .compareOp               = {},
            .minLod                  = 0.0f,
            .maxLod                  = 0.0f,
            .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        });
    }

    static VkSampler createSampler(const Sampler& sampler)
    {
        return core::context().create(VkSamplerCreateInfo {
            .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .pNext                   = nullptr,
            .flags                   = {},
            .magFilter               = sampler.magFilter,
            .minFilter               = sampler.minFilter,
            .mipmapMode              = sampler.mipmapMode,
            .addressModeU            = sampler.addressModeU,
            .addressModeV            = sampler.addressModeV,
            .addressModeW            = sampler.addressModeW,
            .mipLodBias              = 0.0f,
            .anisotropyEnable        = VK_TRUE,
            .maxAnisotropy           = core::context().physicalDevice.maxSamplerAnisotropy,
            .compareEnable           = false,
            .compareOp               = {},
            .minLod                  = 0.0f,
            .maxLod                  = 0.0f,
            .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        });
    }
};

template<VkShaderStageFlags stageFlags>
using TextureDescription = core::Description<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, Texture>;

}  // namespace surge::asset
