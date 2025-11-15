
#include "surge/Engine.hpp"

void createRope(surge::physics::Physics& physics, const surge::physics::Position& first,
                const surge::physics::Position& second, const int size)
{
    auto& firstAnchor  = physics.addAnchor(first);
    auto& secondAnchor = physics.addAnchor(second);

    const auto trajectory = second - first;
    const auto distance   = surge::core::math::norm(trajectory);
    const auto direction  = surge::core::math::normalize(trajectory);

    std::vector<surge::physics::Particle*> particles;
    particles.reserve(size - 2);
    for (int index = 1; index < size - 1; ++index)
    {
        const auto step     = index * distance / size;
        const auto position = first + step * direction;
        particles.emplace_back(&physics.addParticle(surge::physics::Mass { 0.01 }, position));
    };
    constexpr surge::physics::Scalar springConstant = 0.1;
    constexpr surge::physics::Scalar restLength     = 0.0;
    // const physics::Scalar     restLength     = distance / size;
    physics.addAnchoredSpring(firstAnchor, *particles.front(), springConstant, restLength);
    physics.addAnchoredSpring(secondAnchor, *particles.back(), springConstant, restLength);
    for (int index = 0; index < size - 3; ++index)
    {
        physics.addSpring(*particles.at(index), *particles.at(index + 1), springConstant, restLength);
    };
}

void resetPhysics(surge::physics::Physics& physics)
{
    using namespace surge::physics;
    physics.clear();

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

    createRope(physics, Position { -10, 0, 0 }, Position { 0, 0, -10 }, 32);
    createRope(physics, Position { -9, 0, 0 }, Position { 0, 0, -9 }, 32);
}

void physicsPlayground()
{
    // using Action = core::input::Action;
    // if (!physicsActive && input.mouse.left == Action::press)
    // {
    //     physicsActive = true;
    // }

    // if (physicsActive)
    // {
    //     const auto duration = input.elapsedTime;

    //     if (input.mouse.right == Action::press)
    //     {
    //         resetPhysics();
    //         physicsActive = false;
    //     }

    //     physics.update(duration);
    // }
}

int main()
{
    try
    {
        const std::filesystem::path home { "/home/ico/projects/" };

        const std::string                         windowName = "A Surge Of Engine";
        const std::string                         appName    = "aSurgeOfEngine";
        constexpr surge::core::Window::Resolution resolution { .width = 1600, .height = 900 };
        surge::Engine                             engine(windowName, appName, resolution);

        using Gltf   = surge::load::Gltf::Handle;
        using Obj    = surge::load::Obj::Handle;
        using Skybox = surge::load::LoadedSkybox::Handle;

        engine.loadAsset("skybox", Skybox { home / "surge/textures/skybox.ktx" });
        // engine.loadAsset("oaktree", Gltf { home / "surge/models/oaktree.gltf" });
        // engine.loadAsset("man", Gltf { home / "extern/Vulkan/assets/models/CesiumMan/glTF/CesiumMan.gltf" });
        engine.loadAsset("dragon", Gltf { home / "extern/Vulkan/assets/models/chinesedragon.gltf" });
        // engine.loadAsset("viking",
        //  Obj { home / "surge/models/viking_room.obj", home / "surge/textures/viking_room.png" });
        // engine.loadAsset("robot", Gltf { home / "uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" });
        // engine.loadAsset("cerberus", Gltf { home / "extern/Vulkan/assets/models/cerberus/cerberus.gltf",
        // {
        // { surge::load::Gltf::TextureType::baseColorTexture,
        //   home / "extern/Vulkan/assets/models/cerberus/albedo.ktx" },
        //  { surge::asset::GltfAsset::TextureType::metallicRoughnessTexture,
        //    home / "extern/Vulkan/assets/models/cerberus/roughness.ktx" },
        //  { surge::asset::GltfAsset::TextureType::emissiveTexture,
        //    home / "extern/Vulkan/assets/models/cerberus/metallic.ktx" },
        //  { surge::asset::GltfAsset::TextureType::normalTexture,
        //    home / "extern/Vulkan/assets/models/cerberus/normal.ktx" },
        //  { surge::asset::GltfAsset::TextureType::occlusionTexture,
        //    home / "extern/Vulkan/assets/models/cerberus/ao.ktx" },
        // } });

        engine.run();
    }
    catch (const std::exception& e)
    {
        surge::log::error(e.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "\033[1;31m[surge of ERROR]\033[0m" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
