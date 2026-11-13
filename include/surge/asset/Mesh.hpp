#pragma once

#include "surge/core/geometry/BoundingBox.hpp"

namespace surge::asset {
struct Mesh {
    struct Primitive {
        uint32_t                    firstIndex;
        uint32_t                    indexCount;
        uint32_t                    vertexCount;
        MaterialID                  materialId;
        core::geometry::BoundingBox boundingBox;
    };

    Mesh(std::vector<Primitive>&& primitives)
        : primitives { primitives } {
    }

    std::vector<Primitive> primitives;
};
}  // namespace surge::asset