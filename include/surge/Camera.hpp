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

template<bool flipY, bool fixed>
class Camera
{
public:
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
        core::math::Perspective<flipY> perspective;
        core::math::View<>             view;
    } mats;

    void update(const Input& input, const core::Window::Resolution& resolution)
    {
        aspect           = static_cast<float>(resolution.width) / resolution.height;
        mats.perspective = core::math::Perspective<flipY> { core::math::deg2rad(45.0f), aspect, 0.1f, 100.0f };

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

    const core::math::Matrix<4, 4> viewProjection() const
    {
        return mats.view * mats.perspective;
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
};

}  // namespace surge
