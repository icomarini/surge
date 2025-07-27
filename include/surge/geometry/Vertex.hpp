#pragma once

namespace surge::geometry
{
using Index = UInt32;

enum class Format
{
    sfloat,
    unorm,
};

enum class Attribute
{
    position,
    color,
    normal,
    texCoord,
    jointIndex,
    jointWeight,
    // Tangent,
    // Joint0,
    // Weight0
};


template<Attribute _attribute, typename _Value, UInt32 _size, Format _format>
    requires((sizeof(_Value) % _size == 0))
class AttributeSlot
{
public:
    using Value                     = _Value;
    static constexpr auto attribute = _attribute;
    static constexpr auto size      = _size;
    static constexpr auto format    = _format;

    Value value;
};


template<typename... Attributes>
class Vertex : public Attributes...
{
public:
    using Self = Vertex<Attributes...>;

    // template<geometry::Attribute requested>
    // using AttributeType = std::tuple_element_t<attributeIndex<requested>(), std::tuple<Attributes...>>;

    template<UInt32 index>
    using Attribute = std::tuple_element_t<index, std::tuple<Attributes...>>;


    static constexpr auto attributeCount = sizeof...(Attributes);

    template<typename Requested>
    static constexpr bool hasAttribute = std::is_base_of_v<Requested, Self>;

    // template<typename Requested>
    //     requires(hasAttribute<Requested>)
    // static constexpr auto attributeIndex = computeAttributeIndex<Requested>();

    // template<typename Requested>
    //     requires(hasAttribute<Requested>)
    // static constexpr UInt32 byteOffset = offsetof(Self, Requested::value);

    template<geometry::Attribute requested>
    auto& get()
    {
        return Attribute<Vertex::attributeIndex<requested>()>::value;
    }

    template<geometry::Attribute requested>
    // requires(hasAttribute<Requested>)
    static constexpr UInt32 attributeIndex()
    {
        UInt32 index {};
        forEach<0, attributeCount>(
            [&]<int i>()
            {
                if constexpr (requested == Attribute<i>::attribute)
                {
                    index = i;
                }
            });
        return index;
    }


    template<geometry::Attribute requested>
    // requires(hasAttribute<Requested>)
    static constexpr UInt32 computeByteOffset()
    {
        UInt32 offset {};
        forEach<0, attributeIndex<requested>()>([&]<int index>() { offset += sizeof(Attribute<index>); });
        return offset;
    }

    constexpr Vertex()
        : Attributes {}...
    {
    }

    constexpr Vertex(const typename Attributes::Value&... values)
        : Attributes { values }...
    {
    }

    // template<typename Requested>
    //     requires(hasAttribute<Requested>)
    // constexpr const auto& get() const
    // {
    //     return Requested::value;
    // }

    // template<typename Requested>
    //     requires(hasAttribute<Requested>)
    // constexpr void set(const typename Requested::Value& value)
    // {
    //     Requested::value = value;
    // }

    // static_assert((byteOffset<Attributes> == computeByteOffset<Attributes>() && ...));
private:
    template<typename Requested>
        requires(hasAttribute<Requested>)
    static constexpr UInt32 computeAttributeIndex()
    {
        UInt32 index {};
        forEach<0, attributeCount>(
            [&]<int i>()
            {
                if constexpr (std::is_same_v<Requested, Attribute<i>>)
                {
                    index = i;
                }
            });
        return index;
    }
};

template<Size size, geometry::Format format>
constexpr VkFormat extractFormat()
{
    constexpr std::array lut {
        std::pair { std::pair { 1, geometry::Format::sfloat }, VK_FORMAT_R32_SFLOAT },
        std::pair { std::pair { 2, geometry::Format::sfloat }, VK_FORMAT_R32G32_SFLOAT },
        std::pair { std::pair { 3, geometry::Format::sfloat }, VK_FORMAT_R32G32B32_SFLOAT },
        std::pair { std::pair { 4, geometry::Format::sfloat }, VK_FORMAT_R32G32B32A32_SFLOAT },
        std::pair { std::pair { 1, geometry::Format::unorm }, VK_FORMAT_R8_UNORM },
        std::pair { std::pair { 2, geometry::Format::unorm }, VK_FORMAT_R8G8_UNORM },
        std::pair { std::pair { 3, geometry::Format::unorm }, VK_FORMAT_R8G8B8_UNORM },
        std::pair { std::pair { 4, geometry::Format::unorm }, VK_FORMAT_R8G8B8A8_UNORM },
    };

    VkFormat result;
    forEach<0, lut.size()>(
        [&]<int i>()
        {
            constexpr auto t = lut.at(i);
            if constexpr (t.first.first == size && t.first.second == format)
            {
                result = t.second;
            }
        });
    return result;
};

template<typename... Attributes>
static constexpr auto createAttributeDescriptions(geometry::Vertex<Attributes...>)
{
    using Vertex = geometry::Vertex<Attributes...>;
    std::array<VkVertexInputAttributeDescription, Vertex::attributeCount> attributeDescriptions;
    forEach<0, Vertex::attributeCount>(
        [&]<int index>()
        {
            using Attribute              = typename Vertex::Attribute<index>;
            attributeDescriptions[index] = {
                .location = index,
                .binding  = 0,
                .format   = extractFormat<Attribute::size, Attribute::format>(),
                .offset   = Vertex::template computeByteOffset<Attribute::attribute>(),
            };
        });
    return attributeDescriptions;
}

template<typename Vertex>
VkPipelineVertexInputStateCreateInfo createVertexInputState()
{
    static constexpr VkVertexInputBindingDescription bindingDescription {
        .binding   = 0,
        .stride    = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,

    };
    static constexpr auto attributeDescriptions = createAttributeDescriptions(Vertex {});

    static constexpr VkPipelineVertexInputStateCreateInfo vertexInputState {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = {},
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions    = attributeDescriptions.data(),
    };
    return vertexInputState;
}
}  // namespace surge::geometry
