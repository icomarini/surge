#pragma once

#include "surge/core/Command.hpp"
#include "surge/core/Image.hpp"
#include "surge/core/Descriptor.hpp"

namespace surge::asset
{

template<typename _ImageInfo, VkImageLayout _imageLayout>
struct TextureInfo
{
    using ImageInfo                   = _ImageInfo;
    static constexpr auto imageLayout = _imageLayout;
};


using SceneImageInfo =
    core::ImageInfo<VkImageCreateFlags {}, VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D>;

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
    enum class Type
    {
        scene = 0,
        cube,
    };

    struct Data
    {
        core::Image::Data image;
        VkImageLayout     layout;
    };

    // const std::map<asset::Texture::Type, asset::TextureData> map {
    //     { asset::Texture::Type::scene,
    //       asset::Texture::Data {
    //           .imageData =
    //               core::Image::Data {
    //                   .imageCreateFlags    = {},
    //                   .format              = VK_FORMAT_R8G8B8A8_SRGB,
    //                   .imageUsageFlags     = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //                   .memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //                   .imageAspectFlags    = VK_IMAGE_ASPECT_COLOR_BIT,
    //                   .imageViewType       = VK_IMAGE_VIEW_TYPE_2D,
    //               },
    //           .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //       } },
    // };

    template<typename LoadedTexture, typename Info>
    Texture(const core::Command& command, const LoadedTexture& loadedTexture, const Sampler& sampler, Info)
        : name { loadedTexture.name }
        , image { loadedTexture, typename Info::ImageInfo {} }
        , sampler { createSampler(sampler) }
        , info { .sampler = this->sampler, .imageView = image.view, .imageLayout = Info::imageLayout }
    {
        command.transferImage(image.image, loadedTexture);
    }

    template<typename LoadedTexture>
    Texture(const core::Command& command, const LoadedTexture& loadedTexture, const Sampler& sampler)
        : name { loadedTexture.name }
        , image { loadedTexture, convert(loadedTexture.type).image }
        , sampler { createSampler(sampler) }
        , info { .sampler = this->sampler, .imageView = image.view, .imageLayout = convert(loadedTexture.type).layout }
    {
        command.transferImage(image.image, loadedTexture);
    }

    template<typename LoadedTexture>
    Texture(const core::Command& command, const LoadedTexture& loadedTexture)
        : name { loadedTexture.name }
        , image { loadedTexture, convert(loadedTexture.type).image }
        , sampler { createSampler(Sampler {
              .magFilter    = VK_FILTER_LINEAR,
              .minFilter    = VK_FILTER_LINEAR,
              .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
              .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
              .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
              .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
          }) }
        , info { .sampler = this->sampler, .imageView = image.view, .imageLayout = convert(loadedTexture.type).layout }
    {
        command.transferImage(image.image, loadedTexture);
    }

    // template<typename LoadedTexture>
    // Texture(const core::Command& command, const LoadedTexture& loadedTexture)
    //     : Texture { command, loadedTexture,
    //                 createSampler(Sampler {
    //                     .magFilter    = VK_FILTER_LINEAR,
    //                     .minFilter    = VK_FILTER_LINEAR,
    //                     .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    //                     .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    //                     .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    //                     .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    //                 }) }
    // {
    // }

    // template<typename LoadedTexture, typename Info>
    // Texture(const core::Command& command, const LoadedTexture& loadedTexture, Info)
    //     : name { loadedTexture.name }
    //     , image { loadedTexture, typename Info::ImageInfo {} }
    //     , sampler { createSampler() }
    //     , info { .sampler = sampler, .imageView = image.view, .imageLayout = Info::imageLayout }
    // {
    //     command.transferImage(image.image, loadedTexture);
    // }

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
    static Data convert(const Type type)
    {
        switch (type)
        {
        case asset::Texture::Type::scene:
            return Data { .image =
                              core::Image::Data {
                                  .imageCreateFlags    = {},
                                  .format              = VK_FORMAT_R8G8B8A8_SRGB,
                                  .imageUsageFlags     = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  .memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  .imageAspectFlags    = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .imageViewType       = VK_IMAGE_VIEW_TYPE_2D,
                              },
                          .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        case asset::Texture::Type::cube:
            return Data { .image =
                              core::Image::Data {
                                  .imageCreateFlags    = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
                                  .format              = VK_FORMAT_R8G8B8A8_SRGB,
                                  .imageUsageFlags     = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                  .memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  .imageAspectFlags    = VK_IMAGE_ASPECT_COLOR_BIT,
                                  .imageViewType       = VK_IMAGE_VIEW_TYPE_CUBE,
                              },
                          .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        }
    }

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
