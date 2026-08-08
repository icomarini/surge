#pragma once

#include "surge/core/geometry/Shape.hpp"
#include "surge/core/geometry/Vertex.hpp"
#include "surge/core/colors.hpp"

namespace surge::core::geometry {
// using PositionAndColor = Vertex<Attribute::position, Attribute::color>;
using Position = Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>>;

using PositionTexture = Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>,
                               AttributeSlot<Attribute::texCoord, core::math::Vector<2>, 2, Format::sfloat>>;

using PositionNormal = Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>,
                              AttributeSlot<Attribute::normal, core::math::Vector<3>, 3, Format::sfloat>>;

using PositionNormalTexture = Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>,
                                     AttributeSlot<Attribute::normal, core::math::Vector<3>, 3, Format::sfloat>,
                                     AttributeSlot<Attribute::texCoord, core::math::Vector<2>, 2, Format::sfloat>>;

using PositionNormalTextureJoint =
    Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>,
           AttributeSlot<Attribute::normal, core::math::Vector<3>, 3, Format::sfloat>,
           AttributeSlot<Attribute::texCoord, core::math::Vector<2>, 2, Format::sfloat>,
           AttributeSlot<Attribute::jointIndex, core::math::Vector<4>, 4, Format::sfloat>,
           AttributeSlot<Attribute::jointWeight, core::math::Vector<4>, 4, Format::sfloat>>;

using PositionNormalTangentTexture =
    Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>,
           AttributeSlot<Attribute::normal, core::math::Vector<3>, 3, Format::sfloat>,
           AttributeSlot<Attribute::tangent, core::math::Vector<4>, 4, Format::sfloat>,
           AttributeSlot<Attribute::texCoord, core::math::Vector<2>, 2, Format::sfloat>>;

using PositionAndColor = geometry::Vertex<
    geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>,
    geometry::AttributeSlot<geometry::Attribute::color, math::Vector<4>, 4, geometry::Format::sfloat>>;


// static constexpr Shape cubeLine { "cubeline",
//                                   std::array {
//                                       PositionAndColor { { 1, 1, 1 }, { 1, 1, 1, 1 } },
//                                       PositionAndColor { { 1, -1, 1 }, { 1, 1, 1, 1 } },
//                                       PositionAndColor { { -1, -1, 1 }, { 1, 1, 1, 1 } },
//                                       PositionAndColor { { -1, 1, 1 }, { 1, 1, 1, 1 } },
//                                       PositionAndColor { { 1, 1, -1 }, { 1, 1, 1, 1 } },
//                                       PositionAndColor { { 1, -1, -1 }, { 1, 1, 1, 1 } },
//                                       PositionAndColor { { -1, -1, -1 }, { 1, 1, 1, 1 } },
//                                       PositionAndColor { { -1, 1, -1 }, { 1, 1, 1, 1 } },
//                                   },
//                                   std::array {
//                                       0, 1, 1, 2, 2, 3, 3, 0,  // up
//                                       4, 5, 5, 6, 6, 7, 7, 4,  // down
//                                       0, 4, 1, 5, 2, 6, 3, 7,  // side
//                                   } };


//    [-1, 1, 1] 5----------7 [ 1, 1, 1]
//              /|         /|
//             / |        / |
// [-1,-1, 1] 4----------6 [ 1,-1, 1]
//            |  |       |  |
//    [-1, 1,-1] 1-------|--3 [ 1, 1,-1]
//            | /        | /
//            |/         |/
// [-1,-1,-1] O----------2 [ 1,-1,-1]

static constexpr Shape cube {
    "cube",
    std::array {
                Position { { -1, -1, -1 } },
                Position { { -1, 1, -1 } },
                Position { { 1, -1, -1 } },
                Position { { 1, 1, -1 } },
                Position { { -1, -1, 1 } },
                Position { { -1, 1, 1 } },
                Position { { 1, -1, 1 } },
                Position { { 1, 1, 1 } },
                },
    std::array {
                1, 4, 0, 5, 4, 1,  // x = -1
        6, 3, 2, 6, 7, 3,  // x = +1
        4, 2, 0, 4, 6, 2,  // y = -1
        3, 5, 1, 7, 5, 3,  // y = +1
        2, 1, 0, 2, 3, 1,  // z = -1
        5, 6, 4, 7, 6, 5,  // z = +1
    }
};

static constexpr math::Vector<3> origin { 0, 0, 0 };

static constexpr math::Vector<3> normalI { 1, 0, 0 };
static constexpr math::Vector<3> normalJ { 0, 1, 0 };
static constexpr math::Vector<3> normalK { 0, 0, 1 };

static constexpr math::Vector<4> tangentI { 1, 0, 0, 1 };
static constexpr math::Vector<4> tangentJ { 0, 1, 0, 1 };
static constexpr math::Vector<4> tangentK { 0, 0, 1, 1 };

static constexpr math::Vector<3> vertex000 { -0.5, -0.5, -0.5 };
static constexpr math::Vector<3> vertex001 { -0.5, -0.5, +0.5 };
static constexpr math::Vector<3> vertex010 { -0.5, +0.5, -0.5 };
static constexpr math::Vector<3> vertex011 { -0.5, +0.5, +0.5 };
static constexpr math::Vector<3> vertex100 { +0.5, -0.5, -0.5 };
static constexpr math::Vector<3> vertex101 { +0.5, -0.5, +0.5 };
static constexpr math::Vector<3> vertex110 { +0.5, +0.5, -0.5 };
static constexpr math::Vector<3> vertex111 { +0.5, +0.5, +0.5 };

static constexpr math::Vector<2> texture00 { 0, 0 };
static constexpr math::Vector<2> texture01 { 0, 1 };
static constexpr math::Vector<2> texture10 { 1, 0 };
static constexpr math::Vector<2> texture11 { 1, 1 };

static constexpr Shape coordinates {
    "coordinates",
    std::array { PositionAndColor { -normalI, RGBA::darkRed }, PositionAndColor { normalI, RGBA::red },
                PositionAndColor { -normalJ, RGBA::darkGreen },                                            PositionAndColor { normalJ, RGBA::green },
                PositionAndColor { -normalK, RGBA::darkBlue },                                                                                           PositionAndColor { normalK, RGBA::blue } },
    std::array { 0,                                            1,                                       2, 3,                                         4, 5                                        }
};

constexpr math::Vector<3> sw { -0.5, -0.5, 0 };
constexpr math::Vector<3> se { 0.5, -0.5, 0 };
constexpr math::Vector<3> nw { -0.5, 0.5, 0 };
constexpr math::Vector<3> ne { 0.5, 0.5, 0 };


//
//  z
//  | y          2----------3
//  |/          /          /
//  O---x      /          /
//            0----------1
//
static constexpr Shape plane {
    "plane",
    std::array {
                Position { sw },
                Position { se },
                Position { nw },
                Position { ne },
                },
    std::array { 0, 1, 2, 1, 3, 2 }
};

static constexpr Shape planeTextured {
    "planeTextured",
    std::array {
                PositionTexture { sw, texture00 },
                PositionTexture { se, texture01 },
                PositionTexture { nw, texture10 },
                PositionTexture { ne, texture11 },
                },
    std::array { 0, 1, 2, 1, 3, 2 }
};

static constexpr Shape planeTexturedNormals {
    "planeTextNor",
    std::array {
                PositionNormalTexture { sw, normalK, texture00 },
                PositionNormalTexture { se, normalK, texture01 },
                PositionNormalTexture { nw, normalK, texture10 },
                PositionNormalTexture { ne, normalK, texture11 },
                },
    std::array { 0, 1, 2, 1, 3, 2 }
};

static constexpr Shape planeNormalTangentTexture {
    "planeTextNorTan",
    std::array {
                PositionNormalTangentTexture { sw, normalK, math::Vector<4> { 1, 0, 0, 1 }, texture00 },
                PositionNormalTangentTexture { se, normalK, math::Vector<4> { 1, 0, 0, 1 }, texture01 },
                PositionNormalTangentTexture { nw, normalK, math::Vector<4> { 1, 0, 0, 1 }, texture10 },
                PositionNormalTangentTexture { ne, normalK, math::Vector<4> { 1, 0, 0, 1 }, texture11 },
                },
    std::array { 0, 1, 2, 1, 3, 2 }
};

}  // namespace surge::core::geometry