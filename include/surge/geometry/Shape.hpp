#pragma once

#include <string>
#include <vector>

namespace surge::geometry
{
template<typename Vertices, typename Indices>
class Shape
{
public:
    using Vertex = typename Vertices::value_type;
    using Index  = typename Indices::value_type;

    constexpr Shape(const std::string& name, const Vertices& vertices, const Indices& indices)
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

    std::string name;
    Vertices    vertices;
    Indices     indices;
};
}  // namespace surge::geometry