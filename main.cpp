
#include "surge/Application.hpp"
#include "surge/utils.hpp"

int main()
{
    try
    {
        const std::filesystem::path rootPath { "/home/ico/projects/surge" };

        [[maybe_unused]] const std::map<std::string, std::filesystem::path> resources {
            { "root", rootPath / "textures" },
            { "vikingRoomTexture", rootPath / "textures/viking_room.png" },
            { "vikingRoomModel", rootPath / "models/viking_room.obj" },
            { "skyboxTexture", rootPath / "textures/skybox.ktx" },
            { "oaktree", rootPath / "models/oaktree.gltf" },
            // { "helmet",
            // "/home/ico/extern/Vulkan-glTF-PBR/data/models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf"
            // },
            { "dragon", "/home/ico/projects/extern/Vulkan/assets/models/chinesedragon.gltf" },
            { "buggy", "/home/ico/projects/extern/Vulkan/assets/models/gltf/"
                       "glTF-Embedded/Buggy.gltf" },
            { "man", "/home/ico/projects/extern/Vulkan/assets/models/CesiumMan/glTF/CesiumMan.gltf" },
            { "simple", "/home/ico/projects/surge/models/skinning_example.gltf" },
            { "gun", "/home/ico/projects/extern/Vulkan/assets/models/cerberus/cerberus.gltf" }
        };

        // assets.emplace_back(command, defaults,
        //                     asset::GltfAsset { "gun",
        //                                               home / "extern/Vulkan/assets/models/cerberus/cerberus.gltf",
        //                                               {
        //                                                   { Type::baseColorTexture, base / "albedo.ktx" },
        //                                                   { Type::metallicRoughnessTexture, base / "roughness.ktx" },
        //                                                   { Type::emissiveTexture, base / "metallic.ktx" },
        //                                                   { Type::normalTexture, base / "normal.ktx" },
        //                                                   { Type::occlusionTexture, base / "ao.ktx" },
        //                                               } });

        const std::filesystem::path                      home { "/home/ico/projects/" };
        std::map<std::string, surge::asset::AssetHandle> assetHandles {
            { "skybox", surge::asset::TextureHandle { home / "surge/textures/skybox.ktx" } },
            { "man", surge::asset::GltfHandle { home / "extern/Vulkan/assets/models/CesiumMan/glTF/CesiumMan.gltf" } },
            { "dragon", surge::asset::GltfHandle { home / "extern/Vulkan/assets/models/chinesedragon.gltf" } },
            { "robot", surge::asset::GltfHandle { home / "uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" } },
            { "viking", surge::asset::ObjHandle { home / "surge/models/viking_room.obj",
                                                  home / "surge/textures/viking_room.png" } },
            // { "cerberus",
            //   surge::asset::GltfHandle { home / "extern/Vulkan/assets/models/cerberus/cerberus.gltf",
            //                              {
            //                                  { surge::asset::GltfAsset::TextureType::baseColorTexture,
            //                                    home / "extern/Vulkan/assets/models/cerberus/albedo.ktx" },
            //                                  { surge::asset::GltfAsset::TextureType::metallicRoughnessTexture,
            //                                    home / "extern/Vulkan/assets/models/cerberus/roughness.ktx" },
            //                                  { surge::asset::GltfAsset::TextureType::emissiveTexture,
            //                                    home / "extern/Vulkan/assets/models/cerberus/metallic.ktx" },
            //                                  { surge::asset::GltfAsset::TextureType::normalTexture,
            //                                    home / "extern/Vulkan/assets/models/cerberus/normal.ktx" },
            //                                  { surge::asset::GltfAsset::TextureType::occlusionTexture,
            //                                    home / "extern/Vulkan/assets/models/cerberus/ao.ktx" },
            //                              } } },
        };

        surge::Application application(resources, assetHandles);
        application.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "\033[1;31m[surge of ERROR]\033[0m " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...)
    {
        std::cerr << "\033[1;31m[surge of ERROR]\033[0m" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
