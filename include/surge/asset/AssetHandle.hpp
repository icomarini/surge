#pragma once

#include "surge/asset/GltfAsset.hpp"

#include <filesystem>
#include <string>
#include <variant>

namespace surge::asset
{


struct TextureHandle
{
    std::filesystem::path path;
};

struct GltfHandle
{
    std::filesystem::path                                          path;
    std::map<asset::GltfAsset::TextureType, std::filesystem::path> externalPaths {};
};

struct ObjHandle
{
    std::filesystem::path meshPath;
    std::filesystem::path texturePath;
};

using AssetHandle = std::variant<TextureHandle, GltfHandle, ObjHandle>;

}  // namespace surge::asset