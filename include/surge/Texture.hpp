#pragma once

#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Descriptor.hpp"

namespace surge
{

template<typename _ImageInfo, VkImageLayout _imageLayout>
struct TextureInfo
{
    using ImageInfo                   = _ImageInfo;
    static constexpr auto imageLayout = _imageLayout;
};

using SceneTextureInfo = TextureInfo<SceneImageInfo, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL>;

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
    template<typename LoadedTexture, typename Info>
    Texture(const Command& command, const LoadedTexture& loadedTexture, const Sampler& sampler, Info)
        : name { loadedTexture.name }
        , image { loadedTexture, typename Info::ImageInfo {} }
        , sampler { createSampler(sampler) }
        , info { .sampler = this->sampler, .imageView = image.view, .imageLayout = Info::imageLayout }
    {
        command.transferImage(image.image, loadedTexture);
    }

    template<typename LoadedTexture, typename Info>
    Texture(const Command& command, const LoadedTexture& loadedTexture, Info)
        : name { loadedTexture.name }
        , image { loadedTexture, typename Info::ImageInfo {} }
        , sampler { createSampler() }
        , info { .sampler = sampler, .imageView = image.view, .imageLayout = Info::imageLayout }
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
        context().destroy(sampler);
    }

public:
    const std::string           name;
    const Image                 image;
    const VkSampler             sampler;
    const VkDescriptorImageInfo info;

private:
    static VkSampler createSampler()
    {
        return context().create(VkSamplerCreateInfo {
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
            .maxAnisotropy           = context().properties.maxSamplerAnisotropy,
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
        return context().create(VkSamplerCreateInfo {
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
            .maxAnisotropy           = context().properties.maxSamplerAnisotropy,
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
using TextureDescription = Description<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stageFlags, Texture>;

}  // namespace surge
