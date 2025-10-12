
#include "surge/Application.hpp"

int main()
{
    try
    {
        // const std::filesystem::path rootPath { "/home/ico/projects/surge" };
        // [[maybe_unused]] const std::map<std::string, std::filesystem::path> resources {
        //     { "root", rootPath / "textures" },
        //     { "helmet", "/home/ico/extern/Vulkan-glTF-PBR/data/models/DamagedHelmet/glTF-Embedded/DamagedHelmet.gltf"
        //     }, { "buggy", "/home/ico/projects/extern/Vulkan/assets/models/gltf/"
        //                "glTF-Embedded/Buggy.gltf" },
        //     { "simple", "/home/ico/projects/surge/models/skinning_example.gltf" },
        // };

        const std::filesystem::path                      home { "/home/ico/projects/" };
        std::map<std::string, surge::asset::AssetHandle> assetHandles {
            { "default", surge::load::LoadedTexture::Handle { home / "surge/textures/default.png" } },
            { "skybox", surge::load::LoadedTexture::Handle { home / "surge/textures/skybox.ktx" } },
            { "oaktree", surge::load::Gltf::Handle { home / "surge/models/oaktree.gltf" } },
            { "man", surge::load::Gltf::Handle { home / "extern/Vulkan/assets/models/CesiumMan/glTF/CesiumMan.gltf" } },
            { "dragon", surge::load::Gltf::Handle { home / "extern/Vulkan/assets/models/chinesedragon.gltf" } },
            { "robot", surge::load::Gltf::Handle { home / "uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" } },
            { "viking", surge::load::Obj::Handle { home / "surge/models/viking_room.obj",
                                                   home / "surge/textures/viking_room.png" } },
            { "cerberus",
              surge::load::Gltf::Handle { home / "extern/Vulkan/assets/models/cerberus/cerberus.gltf",
                                          {
                                              //  { surge::asset::GltfAsset::TextureType::baseColorTexture,
                                              //    home / "extern/Vulkan/assets/models/cerberus/albedo.ktx" },
                                              //  { surge::asset::GltfAsset::TextureType::metallicRoughnessTexture,
                                              //    home / "extern/Vulkan/assets/models/cerberus/roughness.ktx" },
                                              //  { surge::asset::GltfAsset::TextureType::emissiveTexture,
                                              //    home / "extern/Vulkan/assets/models/cerberus/metallic.ktx" },
                                              //  { surge::asset::GltfAsset::TextureType::normalTexture,
                                              //    home / "extern/Vulkan/assets/models/cerberus/normal.ktx" },
                                              //  { surge::asset::GltfAsset::TextureType::occlusionTexture,
                                              //    home / "extern/Vulkan/assets/models/cerberus/ao.ktx" },
                                          } } },
            // { "crate", surge::asset::ObjHandle { home / "Container_v1_L1/12279_Container_v1_l1.obj",
            //                                      home / "Container_v1_L1/Container_diffuse.jpg" } },
        };

        surge::Application application(assetHandles);
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
