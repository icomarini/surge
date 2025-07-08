#pragma once

#include "surge/asset/LoadedTexture.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <filesystem>
#include <optional>
#include <vector>

namespace surge::asset
{

class ObjAsset
{
public:
    ObjAsset(const std::string& name, const std::filesystem::path& modelPath,
             const std::optional<std::filesystem::path>& texturePath)
        : name { name }
        , path { modelPath }
        , texture {}
    {
        if (texturePath)
        {
            texture.emplace(texturePath.value());
        }

        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.string().c_str()))
        {
            throw std::runtime_error(warn + err);
        }
    }

    std::string                      name;
    std::filesystem::path            path;
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::optional<LoadedTexture>     texture;
};

}  // namespace surge::asset
