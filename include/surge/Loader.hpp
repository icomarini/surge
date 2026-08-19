#pragma once

#include "surge/Storage.hpp"
#include "surge/load/Gltf.hpp"

namespace surge {

class Loader {
public:
    Loader(Storage& storage)
        : storage { storage } {
    }

    template<core::shader::Type shaderType>
    Asset loadAsset(const load::Gltf::Handle&                           handle,
                    const std::map<load::Gltf::TextureType, TextureID>& externalTextureIds) {
        const load::Gltf gltf { handle, storage.defaults };
        const auto       textureIds    = gltf.createTextures(storage);
        const auto       materialIds   = gltf.createMaterials(storage, textureIds, externalTextureIds);
        const auto       meshIds       = gltf.createMeshes(storage, materialIds);
        constexpr auto   pipelineIndex = Storage::getPipelineIndex<shaderType>();
        using Vertex                   = std::tuple_element_t<pipelineIndex, Storage::Pipelines>::Vertex;
        const auto modelId             = gltf.createModel<Vertex>(storage, meshIds);
        const auto skinIds             = gltf.createSkins(storage);
        const auto nodeTreeId          = gltf.createNodeTree(storage, meshIds, skinIds);
        const auto animationSetId      = gltf.createAnimationSet(storage);

        return Asset {
            .shaderType     = shaderType,
            .modelId        = modelId,
            .nodeTree       = storage.nodeTrees.at(nodeTreeId),
            .meshNodes      = {},
            .skinsUsed      = {},
            .animationSetId = animationSetId,
        };
    }

    template<core::shader::Type shaderType>
    Asset loadAsset(const load::Gltf::Handle& handle) {
        return loadAsset<shaderType>(handle, {});
    }

    template<core::shader::Type shaderType>
    Asset loadAsset(const ModelID modelId, const MaterialID materialId) {
        const auto meshId     = storage.createMesh({
            asset::Mesh2::Primitive { .firstIndex  = 0,
                                     .indexCount  = storage.models.get(modelId).indexCount,
                                     .vertexCount = storage.models.get(modelId).vertexCount,
                                     .materialId  = materialId,
                                     .boundingBox = {} }
        });
        const auto nodeTreeId = storage.createNodeTree(storage, meshId);
        return Asset {
            .shaderType     = shaderType,
            .modelId        = modelId,
            .nodeTree       = storage.nodeTrees.at(nodeTreeId),
            .meshNodes      = {},
            .skinsUsed      = {},
            .animationSetId = {},
        };
    }

    template<core::shader::Type shaderType>
    Asset loadAsset(const ModelID modelId) {
        return loadAsset<shaderType>(modelId, MaterialID {});
    }

private:
    Storage& storage;
};
}  // namespace surge