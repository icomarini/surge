#pragma once

#include <array>
#include <string_view>

namespace surge::shader
{

enum class Stage
{
    vertex,
    fragment,
};

enum class Type
{
    gltfAnimated,
    gltfStatic,
    bbox,
    line,
    point,
    shader,
    skybox,
    ui,
};

constexpr VkShaderStageFlagBits translate(const Stage stage)
{
    switch (stage)
    {
    case Stage::vertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case Stage::fragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    default:
        throw;
    }
}

constexpr Stage translate(const VkShaderStageFlagBits stage)
{
    switch (stage)
    {
    case VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT:
        return Stage::vertex;
    case VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT:
        return Stage::fragment;
    default:
        throw;
    }
}

struct Entry
{
    Type                 type;
    Stage                stage;
    const unsigned char* data = nullptr;
    std::size_t          size;
};

template<typename EmbeddedData>
constexpr Entry entry(const Type type, const Stage stage, const EmbeddedData& data)
{
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
};

constexpr Entry get(const Type type, const Stage stage)
{
    static constexpr unsigned char* invalid = nullptr;
    Entry                           requested { entry(type, stage, invalid) };
    for (const auto& entry : library)
    {
        if (entry.type == type && entry.stage == stage)
        {
            requested = entry;
        }
    }
    return requested;
}

}  // namespace surge::shader