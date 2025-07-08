#pragma once

#include "surge/asset/Node.hpp"

namespace surge::asset
{

class Animation
{
public:
    struct Channel
    {
        enum class Path
        {
            translation,
            rotation,
            scale,
            weights
        };
        Path     path;
        Node*    node;
        uint32_t samplerIndex;
    };

    struct Sampler
    {
        enum class Interpolation
        {
            linear,
            step,
            cubicspline
        };
        Interpolation                interpolation;
        std::vector<float>           inputs;
        std::vector<math::Vector<4>> outputs;
    };

    std::string          name;
    float                start = std::numeric_limits<float>::max();
    float                end   = std::numeric_limits<float>::min();
    std::vector<Sampler> samplers;
    std::vector<Channel> channels;

    struct State
    {
        bool  active { true };
        float progress { 0 };
    };
    mutable State state;

    void update(const double elapsedTime)
    {
        state.progress += elapsedTime;
        if (state.progress > end)
        {
            state.progress -= end;
        }
        for (const auto& channel : channels)
        {
            const auto& sampler    = samplers.at(channel.samplerIndex);
            const auto  lowerBound = std::lower_bound(sampler.inputs.begin(), sampler.inputs.end(), state.progress);
            const auto  index      = std::distance(sampler.inputs.begin(), lowerBound - 1);
            if (index < 0 || index >= static_cast<std::ptrdiff_t>(sampler.inputs.size() - 1))
            {
                continue;
            }

            assert(sampler.inputs.at(index) <= state.progress && state.progress < sampler.inputs.at(index + 1));
            const auto a =
                (state.progress - sampler.inputs.at(index)) / (sampler.inputs.at(index + 1) - sampler.inputs.at(index));
            switch (channel.path)
            {
            case Channel::Path::translation:
            {
                const auto&           x4 { sampler.outputs.at(index) };
                const auto&           y4 { sampler.outputs.at(index + 1) };
                const math::Vector<3> x { x4.at(0), x4.at(1), x4.at(2) };
                const math::Vector<3> y { y4.at(0), y4.at(1), y4.at(2) };
                channel.node->state.translation = math::lerp(x, y, a);
                break;
            }
            case Channel::Path::rotation:
            {
                const math::Quaternion x     = sampler.outputs.at(index);
                const math::Quaternion y     = sampler.outputs.at(index);
                channel.node->state.rotation = math::normalize(math::slerp(x, y, a));
                break;
            }
            case Channel::Path::scale:
            {
                const auto&           x4 { sampler.outputs.at(index) };
                const auto&           y4 { sampler.outputs.at(index + 1) };
                const math::Vector<3> x { x4.at(0), x4.at(1), x4.at(2) };
                const math::Vector<3> y { y4.at(0), y4.at(1), y4.at(2) };
                channel.node->state.scale = math::lerp(x, y, a);
                break;
            }
            case Channel::Path::weights:
            {
                throw std::runtime_error("unsupported");
                break;
            }
            }
        }
    }
};
}  // namespace surge::asset