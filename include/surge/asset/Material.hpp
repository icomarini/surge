#pragma once

#include "surge/Texture.hpp"

namespace surge::asset
{

struct Material
{
    enum class AlphaMode
    {
        blend,
        mask,
        opaque,
    };

    static std::string to_string(const AlphaMode alphaMode)
    {
        switch (alphaMode)
        {
        case AlphaMode::blend:
            return "blend";
        case AlphaMode::mask:
            return "mask";
        case AlphaMode::opaque:
            return "opaque";
        }
        throw;
    }

    struct TextureData
    {
        const Texture* texture;
        uint8_t        texCoord;
    };

    std::string name;

    bool doubleSided;
    bool unlit;

    AlphaMode alphaMode;
    float     alphaCutoff;

    TextureData     baseColorTexture;
    math::Vector<4> baseColorFactor;


    TextureData metallicRoughnessTexture;
    float       metallicFactor;
    float       roughnessFactor;

    TextureData     emissiveTexture;
    math::Vector<4> emissiveFactor;
    float           emissiveStrength;

    TextureData normalTexture;
    float       normalScale;

    TextureData occlusionTexture;
    float       occlusionStrength;

    VkDescriptorSet descriptorSet;

    struct Extension
    {
        Texture*        specularGlossinessTexture;
        Texture*        diffuseTexture;
        math::Vector<4> diffuseFactor;
        math::Vector<3> specularFactor;
    } /*extension*/;

    struct PbrWorkflows
    {
        bool metallicRoughness  = true;
        bool specularGlossiness = false;
    } /*pbrWorkflows*/;

    int index = 0;
};
}  // namespace surge::asset