#pragma once

#include "surge/Storage.hpp"
#include "surge/load/Gltf.hpp"

namespace surge {

class Loader {
public:
    Loader(Storage& storage)
        : storage { storage } {
    }

    template<typename Vertex>
    Entity load(const core::shader::Type shaderType, const load::Gltf::Handle& handle,
                const std::map<load::Gltf::TextureType, TextureID>& externalTextureIds) {
        const load::Gltf gltf { handle, storage.defaults };
        const auto       textureIds        = gltf.createTextures(storage);
        const auto       materialIds       = gltf.createMaterials(storage, textureIds, externalTextureIds);
        const auto       meshIds           = gltf.createMeshes(storage, materialIds);
        const auto       newModelId        = gltf.createModel<Vertex>(storage, meshIds);
        const auto       newSkinIds        = gltf.createSkins(storage);
        const auto       newNodeTreeId     = gltf.createNodeTree(storage, meshIds, newSkinIds);
        const auto       newAnimationSetId = gltf.createAnimationsSet(storage);
        return Entity {
            .modelId        = newModelId,
            .nodeTreeId     = newNodeTreeId,
            .pipelineId     = storage.getPipeline(shaderType),
            .animationSetId = newAnimationSetId,
        };
    }

    template<typename Vertex>
    Entity load(const core::shader::Type shaderType, const load::Gltf::Handle& handle) {
        return load<Vertex>(shaderType, handle, {});
    }

    Entity load(const core::shader::Type shaderType, const ModelID modelId) {
        return load(shaderType, modelId, MaterialID {});
    }

    Entity load(const core::shader::Type shaderType, const ModelID modelId, const MaterialID materialId) {
        const auto meshId = storage.createMesh({
            asset::Mesh2::Primitive { .firstIndex  = 0,
                                     .indexCount  = storage.models.get(modelId).indexCount,
                                     .vertexCount = storage.models.get(modelId).vertexCount,
                                     .materialId  = materialId,
                                     .boundingBox = {} }
        });
        return Entity {
            .modelId        = modelId,
            .nodeTreeId     = storage.createNodeTree(storage, meshId),
            .pipelineId     = storage.getPipeline(shaderType),
            .animationSetId = {},
        };
    }

private:
    Storage& storage;
};
}  // namespace surge