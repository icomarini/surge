
#include "surge/Application.hpp"

int main()
{
    try
    {
        const std::filesystem::path                     home { "/home/ico/projects/" };
        std::map<std::string, surge::load::AssetHandle> assetHandles {
            { "default", surge::load::LoadedTexture::Handle { surge::asset::Texture::Type::scene,
                                                              home / "surge/textures/default.png" } },
        };

        const std::string                                windowName = "A Surge Of Engine";
        const std::string                                appName    = "aSurgeOfEngine";
        static constexpr surge::core::Window::Resolution resolution { .width = 1600, .height = 900 };
        surge::Application                               engine(windowName, appName, resolution, assetHandles);

        using Gltf   = surge::load::Gltf::Handle;
        using Obj    = surge::load::Obj::Handle;
        using Skybox = surge::load::LoadedSkybox::Handle;

        engine.loadAsset("skyboxasset", Skybox { home / "surge/textures/skybox.ktx" });
        engine.loadAsset("oaktree", Gltf { home / "surge/models/oaktree.gltf" });
        engine.loadAsset("man", Gltf { home / "extern/Vulkan/assets/models/CesiumMan/glTF/CesiumMan.gltf" });
        engine.loadAsset("dragon", Gltf { home / "extern/Vulkan/assets/models/chinesedragon.gltf" });
        engine.loadAsset("viking",
                         Obj { home / "surge/models/viking_room.obj", home / "surge/textures/viking_room.png" });
        engine.loadAsset("robot", Gltf { home / "uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" });
        engine.loadAsset("cerberus", Gltf { home / "extern/Vulkan/assets/models/cerberus/cerberus.gltf",
                                            {
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
                                            } });

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
