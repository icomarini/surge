#pragma once

#include "surge/core/Command.hpp"
#include "surge/core/geometry/shapes.hpp"
#include "surge/core/math/matrices.hpp"
#include "surge/core/Pipeline.hpp"
#include "surge/asset/Texture.hpp"
#include "surge/asset/Model.hpp"
#include "surge/load/LoadedTexture.hpp"

namespace surge::load {

enum class This {
    animation,
    material,
    mesh,
    node,
    scene,
    skin,
    texture,
};

const std::map<This, std::string> toString {
    { This::animation, "animation" },
    { This::material,  "material"  },
    { This::mesh,      "mesh"      },
    { This::node,      "node"      },
    { This::scene,     "scene"     },
    { This::skin,      "skin"      },
    { This::texture,   "texture"   }
};

template<This t>
std::string baptize() {
    return "<default " + toString.at(t) + ">";
}

template<This t>
std::string baptize(const uint32_t id) {
    return "<unnamed " + toString.at(t) + " " + std::to_string(id) + ">";
}

template<This t, typename String>
std::string baptize(const String& name, const uint32_t id) {
    return name.size() > 0 ? std::string { name } : baptize<t>(id);
}

constexpr auto toUint8(const core::Colors<core::Type::rgba>::Format& color) {
    using SrcValue = core::math::ValueType<core::Colors<core::Type::rgba>::Format>;
    using DstValue = unsigned char;

    const auto max  = std::numeric_limits<DstValue>::max();
    const auto zero = core::math::zero<SrcValue>;
    const auto one  = core::math::one<SrcValue>;

    return std::array {
        static_cast<DstValue>(max * std::clamp(core::math::get<0>(color), zero, one)),
        static_cast<DstValue>(max * std::clamp(core::math::get<1>(color), zero, one)),
        static_cast<DstValue>(max * std::clamp(core::math::get<2>(color), zero, one)),
        static_cast<DstValue>(max * std::clamp(core::math::get<3>(color), zero, one)),
    };
};

template<uint32_t w, uint32_t h>
struct TextureData {
    static constexpr auto                                    width  = w;
    static constexpr auto                                    height = h;
    std::array<std::array<unsigned char, 4>, width * height> texture;

    const unsigned char* data() const {
        return texture.front().data();
    }
};

static consteval auto createDefaultTextureData(const core::Colors<core::Type::rgba>::Format& background,
                                               const core::Colors<core::Type::rgba>::Format& square) {
    const auto g = toUint8(background);
    const auto w = toUint8(square);
    return TextureData<16, 16> {
        {
         //
            g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g,  //
            g, w, w, w, w, w, w, w, w, w, w, w, w, w, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, g, g, g, g, g, g, g, g, g, g, g, g, w, g,  //
            g, w, w, w, w, w, w, w, w, w, w, w, w, w, w, g,  //
            g, g, g, g, g, g, g, g, g, g, g, g, g, g, g, g,  //
        }
    };
}

static consteval auto createTextureDataX(const core::Colors<core::Type::rgba>::Format& background,
                                         const core::Colors<core::Type::rgba>::Format& text) {
    const auto b = toUint8(background);
    const auto t = toUint8(text);
    return TextureData<16, 16> {
        {
         //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, b, t, t, t, b, b, t, t, t, b, b, b, b,  //
            b, b, b, b, b, t, t, t, t, t, t, b, b, b, b, b,  //
            b, b, b, b, b, b, t, t, t, t, b, b, b, b, b, b,  //
            b, b, b, b, b, t, t, t, t, t, t, b, b, b, b, b,  //
            b, b, b, b, t, t, t, b, b, t, t, t, b, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
        }
    };
}

static consteval auto createTextureDataY(const core::Colors<core::Type::rgba>::Format& background,
                                         const core::Colors<core::Type::rgba>::Format& text) {
    const auto b = toUint8(background);
    const auto t = toUint8(text);
    return TextureData<16, 16> {
        {
         //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, t, t, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, b, t, t, t, b, b, t, t, t, b, b, b, b,  //
            b, b, b, b, b, t, t, t, t, t, t, b, b, b, b, b,  //
            b, b, b, b, b, b, t, t, t, t, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
        }
    };
}

static consteval auto createTextureDataZ(const core::Colors<core::Type::rgba>::Format& background,
                                         const core::Colors<core::Type::rgba>::Format& text) {
    const auto b = toUint8(background);
    const auto t = toUint8(text);
    return TextureData<16, 16> {
        {
         //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, t, t, t, b, b, b, b,  //
            b, b, b, b, b, b, b, b, t, t, t, b, b, b, b, b,  //
            b, b, b, b, b, b, b, t, t, t, b, b, b, b, b, b,  //
            b, b, b, b, b, b, t, t, t, b, b, b, b, b, b, b,  //
            b, b, b, b, b, t, t, t, b, b, b, b, b, b, b, b,  //
            b, b, b, b, t, t, t, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
            b, b, b, t, t, t, t, t, t, t, t, t, t, b, b, b,  //
            b, b, b, b, b, b, b, b, b, b, b, b, b, b, b, b,  //
        }
    };
}

// static constexpr auto textureData2() {
//     const auto w = toUint8(core::Colors<core::Type::rgba>::white);
//     return std::array<decltype(w), 1> { w };
// }

static consteval auto createFlatTextureData(const core::Colors<core::Type::rgba>::Format& color) {
    // const auto c = toUint8(color);
    return TextureData<1, 1> { std::array { toUint8(color) } };
}

static consteval asset::Texture::Sampler defaultSampler() {
    return asset::Texture::Sampler {
        .magFilter    = VK_FILTER_LINEAR,
        .minFilter    = VK_FILTER_LINEAR,
        .mipmapMode   = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    };
}
}  // namespace surge::load