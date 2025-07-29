#pragma once

#include "surge/geometry/Vertex.hpp"

// #define TINYOBJLOADER_IMPLEMENTATION
// #include <tiny_obj_loader.h>

#include <filesystem>
#include <vector>

namespace surge
{

class LoadedModel
{
public:
    using Index  = geometry::Index;
    using Vertex = geometry::Vertex<
        geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::color, math::Vector<4>, 4, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::normal, math::Vector<3>, 3, geometry::Format::sfloat>,
        geometry::AttributeSlot<geometry::Attribute::texCoord, math::Vector<2>, 2, geometry::Format::sfloat>>;

    LoadedModel(const std::string& name, const std::filesystem::path&)
        : name { name }
    {
        // tinyobj::attrib_t                attrib;
        // std::vector<tinyobj::shape_t>    shapes;
        // std::vector<tinyobj::material_t> materials;
        // std::string                      warn, err;

        // if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str()))
        // {
        //     throw std::runtime_error(warn + err);
        // }

        // for (const auto& shape : shapes)
        // {
        //     for (const auto& index : shape.mesh.indices)
        //     {
        //         const Vertex vertex {
        //             {
        //                 attrib.vertices[3 * index.vertex_index + 0],
        //                 attrib.vertices[3 * index.vertex_index + 1],
        //                 attrib.vertices[3 * index.vertex_index + 2],
        //             },
        //             { 1.0f, 1.0f, 1.0f },
        //             {
        //                 attrib.normals[3 * index.vertex_index + 0],
        //                 attrib.normals[3 * index.vertex_index + 1],
        //                 attrib.normals[3 * index.vertex_index + 2],
        //             },
        //             {
        //                 attrib.texcoords[2 * index.texcoord_index + 0],
        //                 1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
        //             },
        //         };
        //         vertices.push_back(vertex);
        //         indices.push_back(indices.size());
        //     }
        // }
    }

    LoadedModel(const std::filesystem::path& path)
        : LoadedModel { path.filename(), path }
    {
    }

    LoadedModel(const std::string& name, std::vector<Vertex>&& vertices, std::vector<Index>&& indices)
        : name { name }
        , vertices { vertices }
        , indices { indices }
    {
    }

    // LoadedModel(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    //     : name { name }
    //     , vertices { vertices }
    //     , indices { indices }
    // {
    // }

    // LoadedModel(std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices)
    //     : LoadedModel { path.filename(), vertices, indices }
    // {
    // }

    static constexpr auto copyVertex = [](void* const mapped, const Vertex* const vertex, const Size size)
    { std::memcpy(mapped, vertex, sizeof(Vertex) * size); };

    static constexpr auto copyIndex = [](void* const mapped, const Index* const index, const Size size)
    { std::memcpy(mapped, index, sizeof(Index) * size); };

    Size vertexSize() const
    {
        return vertices.size();
    }
    Size vertexBufferSize() const
    {
        return sizeof(Vertex) * vertexSize();
    }
    const Vertex* vertexData() const
    {
        return vertices.data();
    }


    Size indexSize() const
    {
        return indices.size();
    }
    Size indexBufferSize() const
    {
        return sizeof(Index) * indexSize();
    }
    const Index* indexData() const
    {
        return indices.data();
    }

    std::string         name;
    std::vector<Vertex> vertices;
    std::vector<Index>  indices;
};

}  // namespace surge
