#pragma once

#include "surge/core/utils/Tree.hpp"
#include "surge/asset/Node.hpp"

namespace surge::asset {

class Animation {
public:
    struct Sampler {
        enum class Interpolation { linear, step, cubicspline };
        Interpolation                      interpolation;
        std::vector<float>                 inputs;
        std::vector<core::math::Vector<4>> outputs;
    };

    struct Channel {
        enum class Path { translation, rotation, scale, weights };
        Path                       path;
        std::optional<core::Index> nodeIndex;
        core::Index                samplerIndex;

        void update(Node& node, const Sampler& sampler, const float progress) const {
            const auto lowerBound = std::lower_bound(sampler.inputs.begin(), sampler.inputs.end(), progress);
            const auto index      = std::distance(sampler.inputs.begin(), lowerBound - 1);
            if (index < 0 || index >= static_cast<std::ptrdiff_t>(sampler.inputs.size() - 1)) {
                return;
            }
            assert(sampler.inputs.at(index) <= progress && progress <= sampler.inputs.at(index + 1));
            const auto a =
                (progress - sampler.inputs.at(index)) / (sampler.inputs.at(index + 1) - sampler.inputs.at(index));
            const auto& x4 { sampler.outputs.at(index) };
            const auto& y4 { sampler.outputs.at(index + 1) };
            switch (path) {
            case Channel::Path::translation: {
                const core::math::Vector<3> x { x4.at(0), x4.at(1), x4.at(2) };
                const core::math::Vector<3> y { y4.at(0), y4.at(1), y4.at(2) };
                node.state.translation = core::math::lerp(x, y, a);
                break;
            }
            case Channel::Path::rotation: {
                const core::math::Quaternion x { x4 };
                const core::math::Quaternion y { y4 };
                node.state.rotation = core::math::normalize(core::math::slerp(x, y, a));
                break;
            }
            case Channel::Path::scale: {
                const core::math::Vector<3> x { x4.at(0), x4.at(1), x4.at(2) };
                const core::math::Vector<3> y { y4.at(0), y4.at(1), y4.at(2) };
                node.state.scale = core::math::lerp(x, y, a);
                break;
            }
            case Channel::Path::weights: {
                throw std::runtime_error("Unsupported");
                break;
            }
            }
        }
    };

    std::string          name;
    float                start = std::numeric_limits<float>::max();
    float                end   = std::numeric_limits<float>::min();
    std::vector<Sampler> samplers;
    std::vector<Channel> channels;

    struct State {
        bool  active { true };
        float progress { 0 };
    };
    mutable State state;

    void update(core::utils::Tree<Node>& nodes, const float progress) const {
        for (const auto& channel : channels) {
            channel.update(nodes.get(channel.nodeIndex.value()), samplers.at(channel.samplerIndex), progress);
        }
    }
};
}  // namespace surge::asset