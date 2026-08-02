#pragma once

#include "surge/Engine.hpp"

namespace surge {

using Resolution = core::Window::Resolution;

using RGBA       = core::Colors<core::Type::rgba>;
using ShaderType = core::shader::Type;

using asset::Texture;

// utils
using core::createArray;
using core::forEach;

using load::createDefaultTextureData;
using load::createTextureDataX;
using load::createTextureDataY;
using load::createTextureDataZ;

using asset::Line;

using namespace core::math;


// using SkyboxHandle = load::LoadedSkybox::Handle;
using GltfHandle = load::Gltf::Handle;

namespace geom {
using namespace core::geometry;
using GltfVertex = load::Gltf::Vertex;
}  // namespace geom
}  // namespace surge