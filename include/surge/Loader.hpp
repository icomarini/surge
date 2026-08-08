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
        const auto       textureIds     = gltf.createTextures(storage);
        const auto       materialIds    = gltf.createMaterials(storage, textureIds, externalTextureIds);
        const auto       meshIds        = gltf.createMeshes(storage, materialIds);
        const auto       modelId        = gltf.createModel<Vertex>(storage, meshIds);
        const auto       skinIds        = gltf.createSkins(storage);
        const auto       nodeTreeId     = gltf.createNodeTree(storage, meshIds, skinIds);
        const auto       animationSetId = gltf.createAnimationSet(storage);

        const auto animationChannelId =
            animationSetId ? storage.createAnimationChannel(storage.nodeTrees.at(nodeTreeId), animationSetId, 0) :
                             AnimationChannelID {};
        // const auto
        return Entity {
            .modelId            = modelId,
            .nodeTreeId         = nodeTreeId,
            .pipelineId         = storage.getPipeline(shaderType),
            .animationChannelId = animationChannelId,
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
            .modelId            = modelId,
            .nodeTreeId         = storage.createNodeTree(storage, meshId),
            .pipelineId         = storage.getPipeline(shaderType),
            .animationChannelId = {},
        };
    }

private:
    Storage& storage;
};
}  // namespace surge