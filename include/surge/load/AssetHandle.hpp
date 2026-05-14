#pragma once

#include "surge/load/LoadedTexture.hpp"
#include "surge/load/LoadedSkybox.hpp"
#include "surge/load/Gltf.hpp"
#include "surge/load/Obj.hpp"

#include <variant>

namespace surge::load {
using AssetHandle = std::variant<LoadedTexture::Handle, LoadedSkybox::Handle, Gltf::Handle, Obj::Handle>;
}  // namespace surge::load