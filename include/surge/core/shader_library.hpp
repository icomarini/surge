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
    line,
    point,
    skybox,
    ui,
    normal,
    coordinates,
    primitive,
    primitiveNormal,
    primitiveTextured,
    primitiveTexturedNormal,
    primitiveTexturedNormalAnimated,
    phongModel,
    phongModelNormal,
    phongModelAnimated,
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

constexpr unsigned char primitiveNormalVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveNormal.vert.spv"
};

constexpr unsigned char primitiveNormalFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveNormal.frag.spv"
};

constexpr unsigned char primitiveTexturedVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTextured.vert.spv"
};

constexpr unsigned char primitiveTexturedFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTextured.frag.spv"
};

constexpr unsigned char primitiveTexturedNormalVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTexturedNormal.vert.spv"
};

constexpr unsigned char primitiveTexturedNormalFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTexturedNormal.frag.spv"
};

constexpr unsigned char primitiveTexturedNormalAnimatedVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTexturedNormalAnimated.vert.spv"
};

constexpr unsigned char primitiveTexturedNormalAnimatedFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/primitiveTexturedNormalAnimated.frag.spv"
};

constexpr unsigned char phongModelVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/phongModel.vert.spv"
};

constexpr unsigned char phongModelFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/phongModel.frag.spv"
};

constexpr unsigned char phongModelNormalVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/phongModelNormal.vert.spv"
};

constexpr unsigned char phongModelNormalFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/phongModelNormal.frag.spv"
};

constexpr unsigned char phongModelAnimatedVert[] {
#embed "/home/ico/projects/surge/build/debug/shaders/phongModelAnimated.vert.spv"
};

constexpr unsigned char phongModelAnimatedFrag[] {
#embed "/home/ico/projects/surge/build/debug/shaders/phongModelAnimated.frag.spv"
};
// clang-format on


constexpr std::array library {
    entry(Type::line, Stage::vertex, lineVert),
    entry(Type::line, Stage::fragment, lineFrag),
    entry(Type::point, Stage::vertex, pointVert),
    entry(Type::point, Stage::fragment, pointFrag),
    entry(Type::skybox, Stage::vertex, skyboxVert),
    entry(Type::skybox, Stage::fragment, skyboxFrag),
    entry(Type::ui, Stage::vertex, uiVert),
    entry(Type::ui, Stage::fragment, uiFrag),
    entry(Type::normal, Stage::geometry, normalGeom),
    entry(Type::coordinates, Stage::vertex, coordinatesVert),
    entry(Type::coordinates, Stage::fragment, coordinatesFrag),
    entry(Type::primitive, Stage::vertex, primitiveVert),
    entry(Type::primitive, Stage::fragment, primitiveFrag),
    entry(Type::primitiveNormal, Stage::vertex, primitiveNormalVert),
    entry(Type::primitiveNormal, Stage::fragment, primitiveNormalFrag),
    entry(Type::primitiveTextured, Stage::vertex, primitiveTexturedVert),
    entry(Type::primitiveTextured, Stage::fragment, primitiveTexturedFrag),
    entry(Type::primitiveTexturedNormal, Stage::vertex, primitiveTexturedNormalVert),
    entry(Type::primitiveTexturedNormal, Stage::fragment, primitiveTexturedNormalFrag),
    entry(Type::primitiveTexturedNormalAnimated, Stage::vertex, primitiveTexturedNormalAnimatedVert),
    entry(Type::primitiveTexturedNormalAnimated, Stage::fragment, primitiveTexturedNormalAnimatedFrag),
    entry(Type::phongModel, Stage::vertex, phongModelVert),
    entry(Type::phongModel, Stage::fragment, phongModelFrag),
    entry(Type::phongModelNormal, Stage::vertex, phongModelNormalVert),
    entry(Type::phongModelNormal, Stage::fragment, phongModelNormalFrag),
    entry(Type::phongModelAnimated, Stage::vertex, phongModelAnimatedVert),
    entry(Type::phongModelAnimated, Stage::fragment, phongModelAnimatedFrag),
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