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
    Entity2 load(const core::shader::Type shaderType, const load::Gltf::Handle& handle,
                 const std::map<load::Gltf::TextureType, TextureID>& externalTextureIds) {
        const load::Gltf gltf { handle, storage.defaults };
        const auto       textureIds        = gltf.createTextures3(storage);
        const auto       materialIds       = gltf.createMaterials3(storage, textureIds, externalTextureIds);
        const auto       meshIds           = gltf.createMeshes3(storage, materialIds);
        const auto       newModelId        = gltf.createModel3<Vertex>(storage, meshIds);
        const auto       newSkinIds        = gltf.createSkins2(storage);
        const auto       newNodeTreeId     = gltf.createNodeTree(storage, meshIds, newSkinIds);
        const auto       newAnimationSetId = gltf.createAnimationsSet(storage);
        return Entity2 {
            .modelId        = newModelId,
            .nodeTreeId     = newNodeTreeId,
            .pipelineId     = storage.getPipeline(shaderType),
            .animationSetId = newAnimationSetId,
        };
    }

    template<typename Vertex>
    Entity2 load(const core::shader::Type shaderType, const load::Gltf::Handle& handle) {
        return load<Vertex>(shaderType, handle, {});
    }

    template<typename LoadedModel>
    Entity2 load(const core::shader::Type shaderType, const LoadedModel& loadedModel) {
        return load(shaderType, storage.createModel(loadedModel), MaterialID {});
    }

    Entity2 load(const core::shader::Type shaderType, const ModelID modelId) {
        return load(shaderType, modelId, MaterialID {});
    }

    Entity2 load(const core::shader::Type shaderType, const ModelID modelId, const MaterialID materialId) {
        const auto meshId = storage.createMesh({
            asset::Mesh2::Primitive { .firstIndex  = 0,
                                     .indexCount  = storage.models.get(modelId).indexCount,
                                     .vertexCount = storage.models.get(modelId).vertexCount,
                                     .materialId  = materialId,
                                     .boundingBox = {} }
        });
        return Entity2 {
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