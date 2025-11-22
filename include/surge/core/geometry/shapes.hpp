#pragma once

#include "surge/core/geometry/Shape.hpp"
#include "surge/core/geometry/Vertex.hpp"

namespace surge::core::geometry
{
// using PositionAndColor = Vertex<Attribute::position, Attribute::color>;
using Position = Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>>;

using PositionAndNormal = Vertex<AttributeSlot<Attribute::position, math::Vector<3>, 3, Format::sfloat>,
                                 AttributeSlot<Attribute::normal, core::math::Vector<3>, 3, Format::sfloat>>;

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

static constexpr Shape cube { "cube",
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
                              } };

static constexpr math::Vector<3> basisI { 1, 0, 0 };
static constexpr math::Vector<3> basisJ { 0, 1, 0 };
static constexpr math::Vector<3> basisK { 0, 0, 1 };

static constexpr math::Vector<3> vertex000 { -0.5, -0.5, -0.5 };
static constexpr math::Vector<3> vertex001 { -0.5, -0.5, +0.5 };
static constexpr math::Vector<3> vertex010 { -0.5, +0.5, -0.5 };
static constexpr math::Vector<3> vertex011 { -0.5, +0.5, +0.5 };
static constexpr math::Vector<3> vertex100 { +0.5, -0.5, -0.5 };
static constexpr math::Vector<3> vertex101 { +0.5, -0.5, +0.5 };
static constexpr math::Vector<3> vertex110 { +0.5, +0.5, -0.5 };
static constexpr math::Vector<3> vertex111 { +0.5, +0.5, +0.5 };

static constexpr Shape cube2 { "cube2",
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
                                   PositionAndNormal { vertex000, -basisI },  //  0: x = -0.5
                                   PositionAndNormal { vertex001, -basisI },  //  1: x = -0.5
                                   PositionAndNormal { vertex010, -basisI },  //  2: x = -0.5
                                   PositionAndNormal { vertex011, -basisI },  //  3: x = -0.5
                                   PositionAndNormal { vertex100, +basisI },  //  4: x = +0.5
                                   PositionAndNormal { vertex101, +basisI },  //  5: x = +0.5
                                   PositionAndNormal { vertex110, +basisI },  //  6: x = +0.5
                                   PositionAndNormal { vertex111, +basisI },  //  7: x = +0.5
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
                                   PositionAndNormal { vertex000, -basisJ },  //  8: y = -0.5
                                   PositionAndNormal { vertex001, -basisJ },  //  9: y = -0.5
                                   PositionAndNormal { vertex100, -basisJ },  // 10: y = -0.5
                                   PositionAndNormal { vertex101, -basisJ },  // 11: y = -0.5
                                   PositionAndNormal { vertex010, +basisJ },  // 12: y = +0.5
                                   PositionAndNormal { vertex011, +basisJ },  // 13: y = +0.5
                                   PositionAndNormal { vertex110, +basisJ },  // 14: y = +0.5
                                   PositionAndNormal { vertex111, +basisJ },  // 15: y = +0.5
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
                                   PositionAndNormal { vertex000, -basisK },  // 16: z = -0.5
                                   PositionAndNormal { vertex010, -basisK },  // 17: z = -0.5
                                   PositionAndNormal { vertex100, -basisK },  // 18: z = -0.5
                                   PositionAndNormal { vertex110, -basisK },  // 19: z = -0.5
                                   PositionAndNormal { vertex001, +basisK },  // 20: z = +0.5
                                   PositionAndNormal { vertex011, +basisK },  // 21: z = +0.5
                                   PositionAndNormal { vertex101, +basisK },  // 22: z = +0.5
                                   PositionAndNormal { vertex111, +basisK },  // 23: z = +0.5
                                                                              /**/
                               },
                               std::array {
                                   0,  1,  2,  1,  3,  2,   // x = -0.5
                                   4,  6,  5,  5,  6,  7,   // x = +0.5
                                   8,  10, 9,  9,  10, 11,  // y = -0.5
                                   12, 13, 14, 13, 15, 14,  // y = +0.5
                                   16, 17, 18, 17, 19, 18,  // z = -0.5
                                   20, 22, 21, 21, 22, 23,  // z = +0.5
                               } };

static constexpr Shape coordinateSystem { "coord",
                                          std::array { PositionAndColor { { -0.9, -0.9, -0.9 }, { 1, 0, 0, 1 } },
                                                       PositionAndColor { { 0.1, -0.9, -0.9 }, { 1, 0, 0, 1 } },
                                                       PositionAndColor { { -0.9, -0.9, -0.9 }, { 0, 1, 0, 1 } },
                                                       PositionAndColor { { -0.9, 0.1, -0.9 }, { 0, 1, 0, 1 } },
                                                       PositionAndColor { { -0.9, -0.9, -0.9 }, { 0, 0, 1, 1 } },
                                                       PositionAndColor { { -0.9, -0.9, 0.1 }, { 0, 0, 1, 1 } } },
                                          std::array { 0, 1, 2, 3, 4, 5 } };
}  // namespace surge::core::geometry