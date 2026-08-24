#pragma once

#include "surge/Storage.hpp"
#include "surge/Pipelines.hpp"
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
        const auto       textureIds  = gltf.createTextures(storage);
        const auto       materialIds = gltf.createMaterials(storage, textureIds, externalTextureIds);
        const auto       meshIds     = gltf.createMeshes(storage, materialIds);
        const auto       modelId     = gltf.createModel<Pipelines::Vertex<shaderType>>(storage, meshIds);
        const auto       skinIds     = gltf.createSkins(storage);
        const auto       nodeTreeId  = gltf.createNodeTree(storage, meshIds, skinIds);

        int nodeIndex = 0;
        storage.nodeTrees.at(nodeTreeId).traverse<core::utils::Traversal::linear>([&](asset::Node2& node) {
            log::info("In file '" + std::string(handle.path) + "': loaded node " + std::to_string(nodeIndex) +
                      " | mesh " + std::to_string(node.meshId.get()) + " | skin " + std::to_string(node.skinId.get()));
            ++nodeIndex;
        });

        const auto animationSetId = gltf.createAnimationSet(storage);

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