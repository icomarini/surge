#pragma once

#include "surge/asset/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ktx.h>

#include <filesystem>
#include <variant>

namespace surge::load
{

class LoadedTexture
{
public:
    struct Handle
    {
        asset::Texture::Type  type;
        std::filesystem::path path;
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

    LoadedTexture(const Handle& handle)
        : width {}
        , height {}
        , mipLevels { 1 }
        , arrayLayers { 1 }
        , name { handle.path.filename() }
        , type { handle.type }  // , textureData { }
        , vOffsets { {} }
    {
        const auto fileExtension = handle.path.extension();

        if (fileExtension == ".ktx")
        {
            ktxTexture* data = nullptr;
            if (ktxTexture_CreateFromNamedFile(handle.path.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                               &data) != KTX_SUCCESS)
            {
                throw std::runtime_error("failed to load texture " + handle.path.string());
            }

            width       = data->baseWidth;
            height      = data->baseHeight;
            mipLevels   = data->numLevels;
            arrayLayers = data->numFaces;

            for (uint32_t arrayLayer = 0; arrayLayer < arrayLayers; ++arrayLayer)
            {
                for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
                {
                    ktx_size_t offset;
                    if (ktxTexture_GetImageOffset(data, mipLevel, 0, arrayLayer, &offset) != KTX_SUCCESS)
                    {
                        throw std::runtime_error("failed to get offset");
                    }
                    vOffsets.emplace_back(mipLevel, arrayLayer, offset);
                }
            }
            pData = data;
            // throw std::runtime_error("Unsupported formnat image 'ktx' for file " + path.string());
        }
        else
        {
            int        texWidth, texHeight, texChannels;
            const auto data =
                stbi_load(handle.path.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            if (data == nullptr)
            {
                throw std::runtime_error("failed to load texture " + handle.path.string());
            }
            pData    = data;
            width    = static_cast<uint32_t>(texWidth);
            height   = static_cast<uint32_t>(texHeight);
            vOffsets = { { 0, 0, 0 } };
        }
    }

    LoadedTexture(const std::filesystem::path& path)
        : LoadedTexture(Handle { asset::Texture::Type::scene, path })
    {
    }

    LoadedTexture(const std::string& name, const uint8_t* const buffer, const uint64_t size)
        : width {}
        , height {}
        , mipLevels { 1 }
        , arrayLayers { 1 }
        , name { name }
        , pData {}
        , vOffsets { {} }
    {
        int        texWidth, texHeight, texChannels;
        const auto data = stbi_load_from_memory(buffer, size, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (data == nullptr)
        {
            throw std::runtime_error("failed to load texture '" + name + "' from buffer");
        }
        pData    = data;
        width    = static_cast<uint32_t>(texWidth);
        height   = static_cast<uint32_t>(texHeight);
        vOffsets = { { 0, 0, 0 } };
    }

    ~LoadedTexture()
    {
        std::visit(
            core::overload {
                [](stbi_uc* data) { stbi_image_free(data); },
                [](ktxTexture* data) { ktxTexture_Destroy(data); },
            },
            pData);
    }

    const void* data() const
    {
        // return static_cast<const void* const>(pData);
        return std::visit(
            core::overload {
                [](stbi_uc* data) { return static_cast<const void*>(data); },
                [](ktxTexture* data) { return static_cast<const void*>(ktxTexture_GetData(data)); },
            },
            pData);
    }

    const std::vector<std::tuple<uint32_t, uint32_t, uint64_t>>& offsets() const
    {
        return vOffsets;
    }

    uint64_t memorySize() const
    {
        return std::visit(
            core::overload {
                [&](stbi_uc*) { return static_cast<uint64_t>(width * height * 4); },
                [](ktxTexture* data) { return static_cast<uint64_t>(ktxTexture_GetDataSize(data)); },
            },
            pData);
    }

    uint32_t             width;
    uint32_t             height;
    uint32_t             mipLevels;
    uint32_t             arrayLayers;
    std::string          name;
    asset::Texture::Type type;

private:
    std::variant<stbi_uc*, ktxTexture*>                   pData;
    std::vector<std::tuple<uint32_t, uint32_t, uint64_t>> vOffsets;
};

}  // namespace surge::load
