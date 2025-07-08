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


}  // namespace surge::geometry
