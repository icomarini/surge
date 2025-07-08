#pragma once

#include "surge/geometry/Shape.hpp"
#include "surge/geometry/Vertex.hpp"

namespace surge::geometry
{
// using PositionAndColor = Vertex<Attribute::position, Attribute::color>;
using Position = geometry::Vertex<
    geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>>;

using PositionAndColor = geometry::Vertex<
    geometry::AttributeSlot<geometry::Attribute::position, math::Vector<3>, 3, geometry::Format::sfloat>,
    geometry::AttributeSlot<geometry::Attribute::color, math::Vector<4>, 4, geometry::Format::sfloat>>;

static constexpr Shape cubeLine { "cubeline",
                                  std::array {
                                      PositionAndColor { { 1, 1, 1 }, { 1, 1, 1, 1 } },
                                      PositionAndColor { { 1, -1, 1 }, { 1, 1, 1, 1 } },
                                      PositionAndColor { { -1, -1, 1 }, { 1, 1, 1, 1 } },
                                      PositionAndColor { { -1, 1, 1 }, { 1, 1, 1, 1 } },
                                      PositionAndColor { { 1, 1, -1 }, { 1, 1, 1, 1 } },
                                      PositionAndColor { { 1, -1, -1 }, { 1, 1, 1, 1 } },
                                      PositionAndColor { { -1, -1, -1 }, { 1, 1, 1, 1 } },
                                      PositionAndColor { { -1, 1, -1 }, { 1, 1, 1, 1 } },
                                  },
                                  std::array {
                                      0, 1, 1, 2, 2, 3, 3, 0,  // up
                                      4, 5, 5, 6, 6, 7, 7, 4,  // down
                                      0, 4, 1, 5, 2, 6, 3, 7,  // side
                                  } };


//    [-1, 1, 1] 5----------7 [ 1, 1, 1]
//              /|         /|
//             / |        / |
// [-1,-1, 1] 4----------6 [ 1,-1, 1]
//            |  |       |  |
//    [-1, 1,-1] 1-------|--3 [ 1, 1,-1]
//            | /        | /
//            |/         |/
// [-1,-1,-1] O----------2 [ 1,-1,-1]


static constexpr Shape cubeFill { "cubefill",
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
                                      6, 3, 2, 6, 7, 3,  // x = 1
                                      4, 2, 0, 4, 6, 2,  // y = -1
                                      3, 5, 1, 7, 5, 3,  // y = 1
                                      2, 1, 0, 2, 3, 1,  // z = -1
                                      5, 6, 4, 7, 6, 5,  // z = 1
                                  } };

static constexpr Shape coordinateSystem { "coord",
                                          std::array { PositionAndColor { { -0.9, -0.9, -0.9 }, { 1, 0, 0, 1 } },
                                                       PositionAndColor { { 0.1, -0.9, -0.9 }, { 1, 0, 0, 1 } },
                                                       PositionAndColor { { -0.9, -0.9, -0.9 }, { 0, 1, 0, 1 } },
                                                       PositionAndColor { { -0.9, 0.1, -0.9 }, { 0, 1, 0, 1 } },
                                                       PositionAndColor { { -0.9, -0.9, -0.9 }, { 0, 0, 1, 1 } },
                                                       PositionAndColor { { -0.9, -0.9, 0.1 }, { 0, 0, 1, 1 } } },
                                          std::array { 0, 1, 2, 3, 4, 5 } };
}  // namespace surge::geometry