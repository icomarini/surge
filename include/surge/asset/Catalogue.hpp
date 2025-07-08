#pragma once

// #include "surge/asset/GltfAsset.hpp"
// #include "surge/asset/ObjAsset.hpp"

#include <filesystem>
#include <optional>
#include <variant>

namespace surge::asset
{

struct Gltf
{
    std::filesystem::path path;
};

struct Obj
{
    std::filesystem::path                modelPath;
    std::optional<std::filesystem::path> texturePath;
};

struct Skybox
{
    std::filesystem::path texturePath;
};

class ResourceManager
{
    using Asset = std::variant<Gltf, Obj, Skybox>;
    ResourceManager(const std::filesystem::path& assetsPath, const std::filesystem::path& shadersPath)
        : assetsPath { assetsPath }
        , shadersPath { shadersPath }
    {
    }

private:
    std::filesystem::path assetsPath;
    std::filesystem::path shadersPath;
};

}  // namespace surge::asset