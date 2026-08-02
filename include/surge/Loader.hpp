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
    std::pair<ModelID, NodeID> load(const load::Gltf::Handle& handle) {
        const load::Gltf gltf { handle, storage.defaults };

        const auto newTextures  = gltf.createTextures3(storage);
        const auto newMaterials = gltf.createMaterials3(storage, newTextures);
        const auto newMeshes    = gltf.createMeshes3(storage, newMaterials);
        const auto newModel     = gltf.createModel3<Vertex>(storage, newMeshes);
        const auto newNodes     = gltf.createNodes(storage, newMeshes);

        // const auto newMaterials = asset.createMaterials2<PhongMaterialLayout>(descriptorPool, newTextures,
        // materials2); const auto newMeshes    = asset.createMeshes2(newMaterials, meshes); return
        // asset.createModel2<Vertex>(command, newMeshes, models);
        return { newModel, newNodes };
    }

private:
    Storage& storage;
};
}  // namespace surge