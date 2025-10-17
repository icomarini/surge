#pragma once

#include "surge/load/Gltf.hpp"
#include "surge/load/Obj.hpp"
#include "surge/load/LoadedTexture.hpp"

#include <variant>

namespace surge::load
{
using AssetHandle = std::variant<LoadedTexture::Handle, Gltf::Handle, Obj::Handle>;
}  // namespace surge::load