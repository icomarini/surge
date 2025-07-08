#pragma once

#include "fastgltf/core.hpp"
#include "fastgltf/tools.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/util.hpp"
#include "fastgltf/glm_element_traits.hpp"

#include <filesystem>

namespace surge::asset
{

class GltfAsset
{
public:
    GltfAsset(const std::string& name, const std::filesystem::path& path)
        : name { name }
        , path { path }
        , asset { createAsset(path) }
    {
    }

    const fastgltf::Node& node(const Size nodeId) const
    {
        return asset.nodes.at(nodeId);
    }

    std::string           name;
    std::filesystem::path path;
    fastgltf::Asset       asset;

private:
    static fastgltf::Asset createAsset(const std::filesystem::path& path)
    {
        const auto errorMessage = [&](const fastgltf::Error error)
        {
            return "failed to load asset at path '" + path.string() +
                   "': " + std::string { fastgltf::getErrorName(error) };
        };

        auto data = fastgltf::GltfDataBuffer::FromPath(path);
        if (!data)
        {
            throw std::runtime_error(errorMessage(data.error()));
        }

        [[maybe_unused]] const auto type = fastgltf::determineGltfFileType(data.get());
        assert(type == fastgltf::GltfType::glTF);

        constexpr auto options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
                                 fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::LoadExternalBuffers;
        auto load = fastgltf::Parser().loadGltf(data.get(), path.parent_path(), options);
        if (!load)
        {
            throw std::runtime_error(errorMessage(load.error()));
        }

        return std::move(load.get());
    }
};


}  // namespace surge::asset
