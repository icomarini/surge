#pragma once

#include "surge/core/math/Vector.hpp"

#include <map>
#include <string>

namespace surge::core::input {
enum class Key {
    space        = 32,
    apostrophe   = 39, /* ' */
    comma        = 44, /* , */
    minus        = 45, /* - */
    period       = 46, /* . */
    slash        = 47, /* / */
    digit0       = 48,
    digit1       = 49,
    digit2       = 50,
    digit3       = 51,
    digit4       = 52,
    digit5       = 53,
    digit6       = 54,
    digit7       = 55,
    digit8       = 56,
    digit9       = 57,
    semicolon    = 59, /* ; */
    equal        = 61, /* = */
    a            = 65,
    b            = 66,
    c            = 67,
    d            = 68,
    e            = 69,
    f            = 70,
    g            = 71,
    h            = 72,
    i            = 73,
    j            = 74,
    k            = 75,
    l            = 76,
    m            = 77,
    n            = 78,
    o            = 79,
    p            = 80,
    q            = 81,
    r            = 82,
    s            = 83,
    t            = 84,
    u            = 85,
    v            = 86,
    w            = 87,
    x            = 88,
    y            = 89,
    z            = 90,
    leftBracket  = 91,  /* [ */
    backslash    = 92,  /* \ */
    rightBracket = 93,  /* ] */
    graveAccent  = 96,  /* ` */
    world1       = 161, /* non-us #1 */
    world2       = 162, /* non-us #2 */
    /* function keys */
    escape         = 256,
    enter          = 257,
    tab            = 258,
    backspace      = 259,
    insert         = 260,
    dlt            = 261, /* delete */
    right          = 262,
    left           = 263,
    down           = 264,
    up             = 265,
    pageUp         = 266,
    pageDown       = 267,
    home           = 268,
    end            = 269,
    capsLock       = 280,
    scrollLock     = 281,
    numLock        = 282,
    printScreen    = 283,
    pause          = 284,
    f1             = 290,
    f2             = 291,
    f3             = 292,
    f4             = 293,
    f5             = 294,
    f6             = 295,
    f7             = 296,
    f8             = 297,
    f9             = 298,
    f10            = 299,
    f11            = 300,
    f12            = 301,
    f13            = 302,
    f14            = 303,
    f15            = 304,
    f16            = 305,
    f17            = 306,
    f18            = 307,
    f19            = 308,
    f20            = 309,
    f21            = 310,
    f22            = 311,
    f23            = 312,
    f24            = 313,
    f25            = 314,
    keypad0        = 320,
    keypad1        = 321,
    keypad2        = 322,
    keypad3        = 323,
    keypad4        = 324,
    keypad5        = 325,
    keypad6        = 326,
    keypad7        = 327,
    keypad8        = 328,
    keypad9        = 329,
    keypadDecimal  = 330,
    keypadDivide   = 331,
    keypadMultiply = 332,
    keypadSubtract = 333,
    keypadAdd      = 334,
    keypadEnter    = 335,
    keypadEqual    = 336,
    leftShift      = 340,
    leftControl    = 341,
    leftAlt        = 342,
    leftSuper      = 343,
    rightShift     = 344,
    rightControl   = 345,
    rightAlt       = 346,
    rightSuper     = 347,
    menu           = 348
};

enum class Button {
    left = 0,
    middle,
    right,
};

enum class Action {
    release = 0,
    press,
    repeat,
    none,
};

// struct Position
// {
//     double x;
//     double y;
// };

// struct Offset
// {
//     double x;
//     double y;
// };
using Position = core::math::Vector<2, double>;
using Offset   = core::math::Vector<2, double>;

const std::map<Action, std::string> toString {
    { Action::none, "none" },
    { Action::release, "release" },
    { Action::press, "press" },
    { Action::repeat, "repeat" },
};
}  // namespace surge::core::input