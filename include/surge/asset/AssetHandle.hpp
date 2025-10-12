#pragma once

#include "surge/load/Gltf.hpp"
#include "surge/load/Obj.hpp"
#include "surge/load/LoadedTexture.hpp"

#include <variant>

namespace surge::asset
{


// struct TextureHandle
// {
//     std::filesystem::path path;
// };

// struct GltfHandle
// {
//     std::filesystem::path                                    path;
//     std::map<load::Gltf::TextureType, std::filesystem::path> externalPaths {};
// };

// struct ObjHandle
// {
//     std::filesystem::path                meshPath;
//     std::optional<std::filesystem::path> texturePath;
// };

using AssetHandle = std::variant<load::LoadedTexture::Handle, load::Gltf::Handle, load::Obj::Handle>;

}  // namespace surge::asset