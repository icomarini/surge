#pragma once

#include "surge/colors.hpp"
#include "surge/Context.hpp"
#include "surge/Command.hpp"
#include "surge/Defaults.hpp"
#include "surge/overlay/Overlay.hpp"
#include "surge/Presenter.hpp"
#include "surge/Renderer.hpp"
#include "surge/Skybox.hpp"

#include "surge/asset/Asset.hpp"
#include "surge/asset/Line.hpp"

#include "surge/physics/Physics.hpp"

#include "surge/entity/Entity.hpp"

namespace surge
{

void createRope(physics::Physics& physics, const physics::Position& first, const physics::Position& second,
                const int size)
{
    auto& firstAnchor  = physics.addAnchor(first);
    auto& secondAnchor = physics.addAnchor(second);

    const auto trajectory = second - first;
    const auto distance   = math::norm(trajectory);
    const auto direction  = math::normalize(trajectory);

    std::vector<physics::Particle*> particles;
    particles.reserve(size - 2);
    for (int index = 1; index < size - 1; ++index)
    {
        const auto step     = index * distance / size;
        const auto position = first + step * direction;
        particles.emplace_back(&physics.addParticle(physics::Mass { 0.01 }, position));
    };
    constexpr physics::Scalar springConstant = 0.1;
    constexpr physics::Scalar restLength     = 0.0;
    // const physics::Scalar     restLength     = distance / size;
    physics.addAnchoredSpring(firstAnchor, *particles.front(), springConstant, restLength);
    physics.addAnchoredSpring(secondAnchor, *particles.back(), springConstant, restLength);
    for (int index = 0; index < size - 3; ++index)
    {
        physics.addSpring(*particles.at(index), *particles.at(index + 1), springConstant, restLength);
    };
}


class Application
{
public:
    const uint32_t    WIDTH      = 1600;
    const uint32_t    HEIGHT     = 900;
    const std::string appName    = "surge-app";
    const std::string engineName = "surge";

    Application(const std::map<std::string, std::filesystem::path>& resources)
        : userInteraction { WIDTH, HEIGHT }
        , ctx { createContext(appName, engineName, WIDTH, HEIGHT, &userInteraction) }
        , command {}
        , presenter { command }
        , defaults { command, resources }
        , skybox { command, resources.at("skyboxTexture") }
        , physics { physics::earthGravity }
        , assets { createAssets(command, resources) }
        , entities {}
        , renderer { assets, physics /*, lines, points*/ }
        , overlay { command, userInteraction, assets }
        , forceRegistry {}
    {
        // preallocation
        // lines.reserve(256);
        // points.reserve(256);
        resetPhysics();

        std::cout << "\033[1;37m[surge of INFO]\033[0m The surge of urge to purge started" << std::endl;
    }

    void resetPhysics()
    {
        using namespace physics;
        physics.clear();
        {
            auto& anchor1   = physics.addAnchor(Position { 0, 0.5, 0 });
            auto& anchor2   = physics.addAnchor(Position { 0, 0.5, 1 });
            auto& anchor3   = physics.addAnchor(Position { 1, 0.5, 0 });
            auto& anchor4   = physics.addAnchor(Position { 1, 0.5, 1 });
            auto& particle1 = physics.addParticle(Mass { 1 }, Position { 0.4, 1.0, 0.5 });

            constexpr Scalar springConstant = 0.5;
            constexpr Scalar restLength     = 1;
            physics.addAnchoredSpring(anchor1, particle1, springConstant, restLength);
            physics.addAnchoredSpring(anchor2, particle1, springConstant, restLength);
            physics.addAnchoredSpring(anchor3, particle1, springConstant, restLength);
            physics.addAnchoredSpring(anchor4, particle1, springConstant, restLength);

            auto& particle2 = physics.addParticle(Mass { 0.1 }, Position { -1.4, 1.0, 0.5 });
            physics.addSpring(particle1, particle2, springConstant, restLength);
        }

        {
            createRope(physics, Position { -10, 0, 0 }, Position { 0, 0, -10 }, 64);
            createRope(physics, Position { -9, 0, 0 }, Position { 0, 0, -9 }, 64);
        }
    }

    ~Application()
    {
        std::cout << "\033[1;37m[surge of INFO]\033[0m The surge of urge to purge "
                     "terminated"
                  << std::endl;
    }

    void run()
    {
        double elapsedTime   = {};
        bool   physicsActive = false;

        auto     start = std::chrono::high_resolution_clock::now();
        uint32_t ticCount {};
        auto     entity1 = renderer.createEntity("robot", 0);
        auto     entity2 = renderer.createEntity("robot", 0);
        auto     entity3 = renderer.createEntity("robot", 0);
        auto     entity4 = renderer.createEntity("robot", 0);

        while (!context().exit())
        {
            if (elapsedTime > 1.0 / 144.0)
            {
                userInteraction.reset();
                context().pollEvents();

                // === physics playground ===
                using KeyState = UserInteraction::KeyState;
                if (!physicsActive && userInteraction.mouse.left == KeyState::press)
                {
                    physicsActive = true;
                }

                if (physicsActive)
                {
                    const auto duration = userInteraction.elapsedTime;

                    physics.update(duration);

                    if (userInteraction.mouse.right == KeyState::press)
                    {
                        resetPhysics();
                        physicsActive = false;
                    }
                }
                // === physics playground ===

                // === entity playground ===
                entity1.update(0, elapsedTime);
                entity2.update(0, elapsedTime);
                entity3.update(0, elapsedTime);
                entity4.update(0, elapsedTime);
                // === entity playground ===

                render(presenter, userInteraction, skybox, renderer, overlay);

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop = std::chrono::high_resolution_clock::now();
            elapsedTime     = 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
            ++ticCount;
        }
        vkDeviceWaitIdle(context().device);
    }

private:
    template<typename... Pipelines>
    void render(Presenter& presenter, const UserInteraction& ui, Pipelines&... pipelines)
    {
        const auto [extent, image, imageView, depthImageView, commandBuffer] = presenter.acquire();

        (pipelines.update(extent, ui), ...);

        presenter.record(image, imageView, depthImageView, extent, commandBuffer, pipelines...);
        presenter.present(command, ui.framebufferResized);
    }

private:
    mutable UserInteraction             userInteraction;
    const Context&                      ctx;
    const Command                       command;
    Presenter                           presenter;
    const Defaults                      defaults;
    Skybox                              skybox;
    physics::Physics                    physics;
    std::map<std::string, asset::Asset> assets;
    std::vector<entity::Entity>         entities;
    Renderer                            renderer;
    overlay::Overlay                    overlay;
    physics::ForceRegistry              forceRegistry;

    std::map<std::string, asset::Asset> createAssets(const Command&                                      command,
                                                     const std::map<std::string, std::filesystem::path>& resources)
    {
        // constexpr std::array names { "oaktree", "helmet", "dragon", "buggy" };
        // constexpr std::array names { "buggy" };
        // constexpr std::array names { "simple" };

        // constexpr std::array names { "nope" };
        // constexpr std::array names { "man" };
        // constexpr std::array names { "gun" };

        // // std::vector<asset::Asset> assets;
        // assets.reserve(names.size() + 2);
        // for (const auto& name : names)
        // {
        //     if (name == std::string { "nope" })
        //     {
        //         continue;
        //     }
        //     assets.emplace_back(command, defaults, asset::GltfAsset { name, resources.at(name) });
        //     assets.emplace_back(
        //         command, defaults,
        //         asset::GltfAsset { "robot",
        //                            "/home/ico/projects/uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" });
        // }

        std::map<std::string, asset::Asset> assets;

        assets.emplace(std::piecewise_construct, std::forward_as_tuple("man"),
                       std::forward_as_tuple(command, defaults, asset::GltfAsset { "man", resources.at("man") }));
        assets.emplace(std::piecewise_construct, std::forward_as_tuple("robot"),
                       std::forward_as_tuple(
                           command, defaults,
                           asset::GltfAsset {
                               "robot", "/home/ico/projects/uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" }));

        // assets.emplace_back(command, defaults,
        //                     asset::ObjAsset { "viking room", resources.at("vikingRoomModel"),
        //                                              resources.at("vikingRoomTexture") });

        // const math::Vector<3> translation { -1.0f, 1.0f, 0.0f };
        // const math::Vector<3> scaling { 0.01f, 0.01f, 0.01f };

        // assets.emplace_back(command, defaults,
        //                     asset::ObjAsset { "container",
        //                                       "/home/ico/projects/Container_v1_L1/12279_Container_v1_l1.obj",
        //                                       "/home/ico/projects/Container_v1_L1/Container_diffuse.jpg" },
        //                     math::Translation { translation } * math::Scaling { scaling });

        // using Type                       = asset::GltfAsset::TextureType;
        // const std::filesystem::path base = "/home/ico/projects/extern/Vulkan/assets/models/cerberus";
        // assets.emplace_back(command, defaults,
        //                     asset::GltfAsset { "gun",
        //                                               base / "cerberus.gltf",
        //                                               {
        //                                                   { Type::baseColorTexture, base / "albedo.ktx" },
        //                                                   { Type::metallicRoughnessTexture, base / "roughness.ktx" },
        //                                                   { Type::emissiveTexture, base / "metallic.ktx" },
        //                                                   { Type::normalTexture, base / "normal.ktx" },
        //                                                   { Type::occlusionTexture, base / "ao.ktx" },
        //                                               } });

        for (auto& [name, asset] : assets)
        {
            asset.state.active = true;
            for (auto& mesh : asset.meshes)
            {
                for (auto& primitive : mesh.primitives)
                {
                    primitive.state.boundingBox = true;
                }
            }
            for (auto& node : asset.mainScene().nodes)
            {
                node.state.active = true;
                for (auto& child : node.children)
                {
                    child.state.active = true;
                }
            }
            // asset.entity.update(asset.skins, asset.animations.front(), 0);
            // int  nodeIndex { 0 };
            // for (const auto& node : asset.entity.nodes.nodes)
            // {
            //     if (node.value.skinIndex)
            //     {
            //         std::cout << "node " << nodeIndex << ": "
            //                   << (node.value.skinIndex ? std::to_string(node.value.skinIndex.value()) :
            //                                              std::string("nope"))
            //                   << std::endl;
            //         // for (const auto)
            //     }
            //     nodeIndex++;
            // }
            // std::cout << "roots:";
            // for (const auto root : asset.entity.nodes.roots)
            // {
            //     std::cout << " " << root;
            // }
            // std::cout << std::endl;
        }
        // std::vector<asset::Asset> assets;
        return assets;
    }
};
}  // namespace surge