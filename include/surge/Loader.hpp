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
    std::pair<ModelID, NodeID> load(const load::Gltf::Handle&                           handle,
                                    const std::map<load::Gltf::TextureType, TextureID>& externalTextureIds) {
        const load::Gltf gltf { handle, storage.defaults };
        const auto       newTextures  = gltf.createTextures3(storage);
        const auto       newMaterials = gltf.createMaterials3(storage, newTextures, externalTextureIds);
        const auto       newMeshes    = gltf.createMeshes3(storage, newMaterials);
        const auto       newModel     = gltf.createModel3<Vertex>(storage, newMeshes);
        const auto       newNodes     = gltf.createNodes(storage, newMeshes);
        return { newModel, newNodes };
    }

    template<typename Vertex>
    std::pair<ModelID, NodeID> load(const load::Gltf::Handle& handle) {
        return load<Vertex>(handle, {});
    }

private:
    Storage& storage;
};
}  // namespace surge