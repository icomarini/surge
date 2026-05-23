#pragma once

#include <array>
#include <string_view>

namespace surge::core::shader {

enum class Stage {
    geometry,
    vertex,
    fragment,
};

enum class Type {
    gltfAnimated,
    gltfStatic,
    bbox,
    line,
    point,
    shader,
    skybox,
    ui,
    normal,
    base,
    coordinates,
    primitive,
    primitiveTextured,
};


struct Entry {
    Type                 type;
    Stage                stage;
    const unsigned char* data = nullptr;
    std::size_t          size;
};

template<typename EmbeddedData>
constexpr Entry entry(const Type type, const Stage stage, const EmbeddedData& data) {
    return Entry { type, stage, data, sizeof(EmbeddedData) };
}

// clang-format off
constexpr unsigned char bboxVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/bbox.vert.spv"
};

constexpr unsigned char bboxFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/bbox.frag.spv"
};

constexpr unsigned char gltfAnimatedVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/gltf_animated.vert.spv"
};

constexpr unsigned char gltfAnimatedFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/gltf_animated.frag.spv"
};

constexpr unsigned char gltfStaticVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/gltf_static.vert.spv"
};

constexpr unsigned char gltfStaticFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/gltf_static.frag.spv"
};

constexpr unsigned char lineVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/line.vert.spv"
};

constexpr unsigned char lineFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/line.frag.spv"
};

constexpr unsigned char pointVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/point.vert.spv"
};

constexpr unsigned char pointFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/point.frag.spv"
};

constexpr unsigned char shaderVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/shader.vert.spv"
};

constexpr unsigned char shaderFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/shader.frag.spv"
};

constexpr unsigned char skyboxVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/skybox.vert.spv"
};

constexpr unsigned char skyboxFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/skybox.frag.spv"
};

constexpr unsigned char uiVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/ui.vert.spv"
};

constexpr unsigned char uiFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/ui.frag.spv"
};

constexpr unsigned char normalGeom[] {
#embed "/home/ico/projects/surge/build/debug/shaders/normal.geom.spv"
};

constexpr unsigned char baseVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/base.vert.spv"
};

constexpr unsigned char baseFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/base.frag.spv"
};

constexpr unsigned char coordinatesVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/coordinates.vert.spv"
};

constexpr unsigned char coordinatesFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/coordinates.frag.spv"
};

constexpr unsigned char primitiveVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitive.vert.spv"
};

constexpr unsigned char primitiveFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitive.frag.spv"
};

constexpr unsigned char primitiveTexturedVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTextured.vert.spv"
};

constexpr unsigned char primitiveTexturedFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTextured.frag.spv"
};
// clang-format on


constexpr std::array library {
    entry(Type::bbox, Stage::vertex, bboxVert),
    entry(Type::bbox, Stage::fragment, bboxFrag),
    entry(Type::gltfAnimated, Stage::vertex, gltfAnimatedVert),
    entry(Type::gltfAnimated, Stage::fragment, gltfAnimatedFrag),
    entry(Type::gltfStatic, Stage::vertex, gltfStaticVert),
    entry(Type::gltfStatic, Stage::fragment, gltfStaticFrag),
    entry(Type::line, Stage::vertex, lineVert),
    entry(Type::line, Stage::fragment, lineFrag),
    entry(Type::point, Stage::vertex, pointVert),
    entry(Type::point, Stage::fragment, pointFrag),
    entry(Type::shader, Stage::vertex, shaderVert),
    entry(Type::shader, Stage::fragment, shaderFrag),
    entry(Type::skybox, Stage::vertex, skyboxVert),
    entry(Type::skybox, Stage::fragment, skyboxFrag),
    entry(Type::ui, Stage::vertex, uiVert),
    entry(Type::ui, Stage::fragment, uiFrag),
    entry(Type::normal, Stage::geometry, normalGeom),
    entry(Type::base, Stage::vertex, baseVert),
    entry(Type::base, Stage::fragment, baseFrag),
    entry(Type::coordinates, Stage::vertex, coordinatesVert),
    entry(Type::coordinates, Stage::fragment, coordinatesFrag),
    entry(Type::primitive, Stage::vertex, primitiveVert),
    entry(Type::primitive, Stage::fragment, primitiveFrag),
    entry(Type::primitiveTextured, Stage::vertex, primitiveTexturedVert),
    entry(Type::primitiveTextured, Stage::fragment, primitiveTexturedFrag),
};

constexpr Entry get(const Type type, const Stage stage) {
    static constexpr unsigned char* invalid = nullptr;
    Entry                           requested { entry(type, stage, invalid) };
    for (const auto& entry : library) {
        if (entry.type == type && entry.stage == stage) {
            requested = entry;
        }
    }
    return requested;
}

}  // namespace surge::core::shader