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

using PositionNormalTextureTangent =
    Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>,
           AttributeSlot<Attribute::normal, core::math::Vector<3>, 3, Format::sfloat>,
           AttributeSlot<Attribute::texCoord, core::math::Vector<2>, 2, Format::sfloat>,
           AttributeSlot<Attribute::tangent, core::math::Vector<4>, 4, Format::sfloat>>;

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

// static constexpr math::Vector<3> tangentI { 1, 0, 0 };
// static constexpr math::Vector<3> tangentJ { 0, 1, 0 };
// static constexpr math::Vector<3> tangentK { 0, 0, 1 };

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

static constexpr Shape cube2 {
    "cube2",
    std::array {
                //               3----------7
        //              /|         /|
        //             / |        / |
        //            1----------5  |
        //  z         |  |       |  |
        //  | y       |  2-------|--6
        //  |/        | /        | /
        //  O---x     |/         |/
        //            O----------4
        PositionNormalTextureTangent { vertex000, -normalI, texture00, +tangentK },  //  0: x = -0.5
        PositionNormalTextureTangent { vertex001, -normalI, texture01, +tangentK },  //  1: x = -0.5
        PositionNormalTextureTangent { vertex010, -normalI, texture10, +tangentK },  //  2: x = -0.5
        PositionNormalTextureTangent { vertex011, -normalI, texture11, +tangentK },  //  3: x = -0.5
        PositionNormalTextureTangent { vertex100, +normalI, texture00, -tangentK },  //  4: x = +0.5
        PositionNormalTextureTangent { vertex101, +normalI, texture01, -tangentK },  //  5: x = +0.5
        PositionNormalTextureTangent { vertex110, +normalI, texture10, -tangentK },  //  6: x = +0.5
        PositionNormalTextureTangent { vertex111, +normalI, texture11, -tangentK },  //  7: x = +0.5
                                                                                     /**/
        //               13---------15
        //              /|         /|
        //             / |        / |
        //            9----------11 |
        //  z         |  |       |  |
        //  | y       |  12------|--14
        //  |/        | /        | /
        //  O---x     |/         |/
        //            8----------10
        PositionNormalTextureTangent { vertex000, -normalJ, texture00, +tangentI },  //  8: y = -0.5
        PositionNormalTextureTangent { vertex001, -normalJ, texture01, +tangentI },  //  9: y = -0.5
        PositionNormalTextureTangent { vertex100, -normalJ, texture10, +tangentI },  // 10: y = -0.5
        PositionNormalTextureTangent { vertex101, -normalJ, texture11, +tangentI },  // 11: y = -0.5
        PositionNormalTextureTangent { vertex010, +normalJ, texture00, -tangentI },  // 12: y = +0.5
        PositionNormalTextureTangent { vertex011, +normalJ, texture01, -tangentI },  // 13: y = +0.5
        PositionNormalTextureTangent { vertex110, +normalJ, texture10, -tangentI },  // 14: y = +0.5
        PositionNormalTextureTangent { vertex111, +normalJ, texture11, -tangentI },  // 15: y = +0.5
                                                                                     /**/
        //               21---------23
        //              /|         /|
        //             / |        / |
        //            20---------22 |
        //  z         |  |       |  |
        //  | y       |  17------|--19
        //  |/        | /        | /
        //  O---x     |/         |/
        //            16---------18
        PositionNormalTextureTangent { vertex000, -normalK, texture00, +tangentJ },  // 16: z = -0.5
        PositionNormalTextureTangent { vertex010, -normalK, texture01, +tangentJ },  // 17: z = -0.5
        PositionNormalTextureTangent { vertex100, -normalK, texture10, +tangentJ },  // 18: z = -0.5
        PositionNormalTextureTangent { vertex110, -normalK, texture11, +tangentJ },  // 19: z = -0.5
        PositionNormalTextureTangent { vertex001, +normalK, texture00, -tangentJ },  // 20: z = +0.5
        PositionNormalTextureTangent { vertex011, +normalK, texture01, -tangentJ },  // 21: z = +0.5
        PositionNormalTextureTangent { vertex101, +normalK, texture10, -tangentJ },  // 22: z = +0.5
        PositionNormalTextureTangent { vertex111, +normalK, texture11, -tangentJ },  // 23: z = +0.5
                                                                                     /**/
    },
    std::array {
                0,  1,  2,  1,  3,  2,   // x = -0.5
        4,  6,  5,  5,  6,  7,   // x = +0.5
        8,  10, 9,  9,  10, 11,  // y = -0.5
        12, 13, 14, 13, 15, 14,  // y = +0.5
        16, 17, 18, 17, 19, 18,  // z = -0.5
        20, 22, 21, 21, 22, 23,  // z = +0.5
    }
};

static constexpr Shape square {
    "square",
    std::array {
                //               3
        //              /|
        //             / |
        //            1  |
        //  z         |  |
        //  | y       |  2
        //  |/        | /
        //  O---x     |/
        //            O
        PositionNormalTextureTangent { vertex000, -normalI, texture00, -tangentJ },  //  0: x = -0.5
        PositionNormalTextureTangent { vertex001, -normalI, texture01, -tangentJ },  //  1: x = -0.5
        PositionNormalTextureTangent { vertex010, -normalI, texture10, -tangentJ },  //  2: x = -0.5
        PositionNormalTextureTangent { vertex011, -normalI, texture11, -tangentJ },  //  3: x = -0.5
    },
    std::array {
                0, 1, 2, 1, 3, 2,  // x = -0.5
    }
};

static constexpr Shape coordinateSystem {
    "coord",
    std::array { PositionAndColor { { -0.9, -0.9, -0.9 }, { 1, 0, 0, 1 } },
                PositionAndColor { { 0.1, -0.9, -0.9 }, { 1, 0, 0, 1 } },
                PositionAndColor { { -0.9, -0.9, -0.9 }, { 0, 1, 0, 1 } },
                PositionAndColor { { -0.9, 0.1, -0.9 }, { 0, 1, 0, 1 } },
                PositionAndColor { { -0.9, -0.9, -0.9 }, { 0, 0, 1, 1 } },
                PositionAndColor { { -0.9, -0.9, 0.1 }, { 0, 0, 1, 1 } }                  },
    std::array { 0,                                                         1, 2, 3, 4, 5 }
};

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

}  // namespace surge::core::geometry