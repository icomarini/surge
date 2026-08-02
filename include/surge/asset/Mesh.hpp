#pragma once

#include "surge/asset/Material.hpp"
#include "surge/core/geometry/BoundingBox.hpp"

namespace surge::asset {
struct Mesh {
    struct Primitive {
        uint32_t        firstIndex;
        uint32_t        indexCount;
        uint32_t        vertexCount;
        const Material& material;

        // bool color;
        // bool normal;
        // bool texCoord;

        using Attributes = std::map<core::geometry::Attribute, bool>;
        const Attributes attributes;

        core::geometry::BoundingBox boundingBox;

        struct State {
            bool boundingBox;
        };
        mutable State state;

        // void setBoundingBox(glm::vec3 min, glm::vec3 max)
        // {
        //     bb.min   = min;
        //     bb.max   = max;
        //     bb.valid = true;
        // }
    };

    Mesh(const std::string& name)
        : name { name }
        , primitives {} {
    }

    std::string            name;
    std::vector<Primitive> primitives;
};


struct Mesh2 {
    struct Primitive {
        uint32_t                    firstIndex;
        uint32_t                    indexCount;
        uint32_t                    vertexCount;
        MaterialID                  materialId;
        core::geometry::BoundingBox boundingBox;
    };

    Mesh2(std::vector<Primitive>&& primitives)
        : primitives { primitives } {
    }

    std::vector<Primitive> primitives;
};
}  // namespace surge::asset