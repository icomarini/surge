#pragma once

#include "surge/Input.hpp"
#include "surge/core/math/matrices.hpp"
#include "surge/core/Window.hpp"

#include <algorithm>
#include <iomanip>

#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/euler_angles.hpp"
// #include "glm/glm.hpp"

namespace surge
{

template<bool fixed>
class Camera
{
public:
    static constexpr auto flipY { true };
    Camera(const float aspect, const core::math::Vector<3>& position, const core::math::Vector<3>& front)
        : sensitivity { 0.1f }
        , speed { 2.5f }
        , aspect { aspect }
        , yaw { -90.0f }
        , pitch { 0.0f }
        , vecs { .position = position, .front = front, .up = { 0, 1, 0 }, }
        , mats {
            .perspective = { core::math::deg2rad(45.0f), aspect, 0.1f, 1024.0f },
            .view        = { vecs.position, vecs.position + vecs.front, vecs.up },
        }
    {
    }

    float sensitivity;
    float speed;
    float aspect;
    float yaw;
    float pitch;

    struct
    {
        core::math::Vector<3> position;
        core::math::Vector<3> front;
        core::math::Vector<3> up;
    } vecs;

    struct
    {
        core::math::Perspective<> perspective;
        core::math::View<>        view;
    } mats;

    void update(const Input& input, const core::Window::Resolution& resolution)
    {
        aspect           = static_cast<float>(resolution.width) / resolution.height;
        mats.perspective = core::math::Perspective<> { core::math::deg2rad(45.0f), aspect, 0.1f, 100.0f };

        if (!input.mouseActive)
        {
            if constexpr (flipY)
            {
                rotate(input.mouse.offset[0], -input.mouse.offset[1]);
            }
            else
            {
                rotate(input.mouse.offset[0], input.mouse.offset[1]);
            }

            if constexpr (!fixed)
            {
                using State        = core::input::Action;
                const auto forward = input.keyboard.w == State::press || input.keyboard.w == State::repeat;
                const auto back    = input.keyboard.s == State::press || input.keyboard.s == State::repeat;
                const auto left    = input.keyboard.a == State::press || input.keyboard.a == State::repeat;
                const auto right   = input.keyboard.d == State::press || input.keyboard.d == State::repeat;
                translate(input.elapsedTime, forward, back, left, right);
            }
            mats.view = core::math::View { vecs.position, vecs.position + vecs.front, vecs.up };
        }
    }

    void update(const float elapsedTime, const core::Window::Resolution& resolution)
    {
        aspect           = static_cast<float>(resolution.width) / resolution.height;
        mats.perspective = core::math::Perspective { core::math::deg2rad(45.0f), aspect, 0.1f, 100.0f };
        // vecs.position    = {
        //     std::cos(core::math::deg2rad(elapsedTime * 360.0f)) * 4.0f,
        //     -5.0f + std::sin(core::math::deg2rad(elapsedTime * 360.0f)) * 2.0f,
        //     2.50f + std::sin(core::math::deg2rad(elapsedTime * 360.0f)) * 0.5f,
        // };
        vecs.position[0] += 0.06 * std::sin(core::math::deg2rad(elapsedTime * 100.0f));
        vecs.position[1] += 0.006 * std::sin(core::math::deg2rad(elapsedTime * 100.0f));
        vecs.position[2] += 0.06 * std::cos(core::math::deg2rad(elapsedTime * 100.0f));

        mats.view = core::math::View { vecs.position, vecs.position + vecs.front, vecs.up };
    }

    void rotate(const float offsetx, const float offsety)
    {
        yaw += sensitivity * offsetx;
        pitch = std::clamp(pitch + sensitivity * offsety, -89.0f, 89.0f);

        const auto y = core::math::deg2rad(yaw);
        const auto p = core::math::deg2rad(pitch);
        vecs.front   = {
            std::cos(y) * std::cos(p),
            std::sin(p),
            std::sin(y) * std::cos(p),
        };
        vecs.up = core::math::cross(core::math::cross(vecs.front, { 0, 1, 0 }), vecs.front);
    }

    void translate(const float elapsedTime, const bool forward, const bool back, const bool left, const bool right)
    {
        const auto                      s = speed * elapsedTime;
        constexpr core::math::Vector<3> z {};
        vecs.position = vecs.position                                                                      //
                        + (forward ? s * vecs.front : z)                                                   //
                        + (back ? -s * vecs.front : z)                                                     //
                        + (left ? -core::math::normalize(core::math::cross(vecs.front, vecs.up)) * s : z)  //
                        + (right ? core::math::normalize(core::math::cross(vecs.front, vecs.up)) * s : z);
    }

    // template<typename T, qualifier Q>
    // GLM_FUNC_QUALIFIER mat<4, 4, T, Q> lookAtRH(vec<3, T, Q> const& eye, vec<3, T, Q> const& center,
    //                                             vec<3, T, Q> const& up)
    // {
    //     vec<3, T, Q> const f(normalize(center - eye));
    //     vec<3, T, Q> const s(normalize(cross(f, up)));
    //     vec<3, T, Q> const u(cross(s, f));

    //     mat<4, 4, T, Q> Result(1);
    //     Result[0][0] = s.x;
    //     Result[1][0] = s.y;
    //     Result[2][0] = s.z;
    //     Result[0][1] = u.x;
    //     Result[1][1] = u.y;
    //     Result[2][1] = u.z;
    //     Result[0][2] = -f.x;
    //     Result[1][2] = -f.y;
    //     Result[2][2] = -f.z;
    //     Result[3][0] = -dot(s, eye);
    //     Result[3][1] = -dot(u, eye);
    //     Result[3][2] = dot(f, eye);
    //     return Result;
    // }

    // void lookAt()
};

}  // namespace surge
