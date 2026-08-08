#include "surge/surge.hpp"

template<int radius>
constexpr auto generateTranslations() {
    constexpr auto                     length = 2 * radius + 1;
    constexpr auto                     size   = length * length;
    std::array<surge::Vector<3>, size> translations;
    surge::forEach<0, length, 0, length>([&]<int i, int j>() {
        constexpr auto index = i * length + j;
        translations[index]  = surge::Vector<3> { 4 * (i - radius), -3, 4 * (j - radius) };
    });
    return translations;
}

int main() {
    try {
        const std::filesystem::path surgeTextureFolder { "/home/ico/projects/surge/textures" };
        const std::filesystem::path vulkanAssetFolder { "/home/ico/projects/extern/Vulkan/assets" };

        // create engine
        const std::string           windowName = "A Surge Of Engine";
        const std::string           appName    = "aSurgeOfEngine";
        constexpr surge::Resolution resolution { .width = 1600, .height = 900 };
        surge::Engine               engine(windowName, appName, resolution);

        // create cameras
        surge::Camera<false> playerCamera {
            16.0 / 9.0,
            { 0.0f, 3.0f,  4.0f  },
            { 0.0f, -0.5f, -1.0f },
        };
        surge::Camera<true> skyboxCamera {
            16.0 / 9.0,
            { 0.0f, 0.0f, 0.0f  },
            { 0.0f, 0.0f, -1.0f },
        };
        surge::Camera<false> lightCamera {
            16.0 / 9.0,
            { -1.0f, 1.0f,  3.0f  },
            { -1.0f, -1.0f, -1.0f },
        };

        // create scene
        const auto mainSceneId = engine.storage.createScene();
        auto&      mainScene   = engine.storage.scenes.at(mainSceneId);

        // load
        const auto skybox = engine.loader.load(surge::ShaderType::skybox, engine.storage.createModel(surge::geom::cube),
                                               engine.storage.createSimpleMaterial(engine.storage.createTexture(
                                                   surgeTextureFolder / "skybox.ktx", surge::Texture::cube)));
        const auto coordinates =
            engine.loader.load(surge::ShaderType::coordinates, engine.storage.createModel(surge::geom::coordinates));

        surge::Vector<3> lightPosition { -2, 2, 1 };
        constexpr auto   lightColor = surge::RGBA::white;

        constexpr auto x = surge::Coordinate::x;
        constexpr auto y = surge::Coordinate::y;
        constexpr auto z = surge::Coordinate::z;
        enum { xBack = 0, xFront, yBack, yFront, zBack, zFront };

        constexpr std::array cubeFaces {
            surge::translate<x>(-0.5) * surge::rotate<y>(+90),  //
            surge::translate<x>(+0.5) * surge::rotate<y>(-90),  //
            surge::translate<y>(-0.5) * surge::rotate<x>(-90),  //
            surge::translate<y>(+0.5) * surge::rotate<x>(+90),  //
            surge::translate<z>(-0.5) * surge::flip<x>(),       //
            surge::fullMatrix(surge::translate<z>(+0.5)),       //
        };

        const auto planeTexturedModel = engine.storage.createModel(surge::geom::planeTextured);

        const auto lightCube = surge::createArray<surge::Entity, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face = engine.loader.load(surge::ShaderType::primitiveTextured, planeTexturedModel,
                                      engine.storage.defaultMaterialId);
        });

        const std::array cubeDiffuseTextures {
            engine.storage.createTexture(surge::createTextureDataX(surge::RGBA::darkRed, surge::RGBA::black)),    //
            engine.storage.createTexture(surge::createTextureDataX(surge::RGBA::red, surge::RGBA::black)),        //
            engine.storage.createTexture(surge::createTextureDataY(surge::RGBA::darkGreen, surge::RGBA::black)),  //
            engine.storage.createTexture(surge::createTextureDataY(surge::RGBA::green, surge::RGBA::black)),      //
            engine.storage.createTexture(surge::createTextureDataZ(surge::RGBA::darkBlue, surge::RGBA::black)),   //
            engine.storage.createTexture(surge::createTextureDataZ(surge::RGBA::blue, surge::RGBA::black)),       //
        };

        const std::array cubeSpecularTextures {
            engine.storage.createTexture(surge::createTextureDataX(surge::RGBA::black, surge::RGBA::white)),  //
            engine.storage.createTexture(surge::createTextureDataY(surge::RGBA::black, surge::RGBA::white)),  //
            engine.storage.createTexture(surge::createTextureDataZ(surge::RGBA::black, surge::RGBA::white)),  //
        };

        const std::array cubeNormalTextures {
            engine.storage.createTexture(
                surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::Format { 0, 0.15, 0.87, 1 })),  //
            engine.storage.createTexture(
                surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::Format { 0, 0.15, 0.87, 1 })),  //
            engine.storage.createTexture(
                surge::createDefaultTextureData(surge::RGBA::blue, surge::RGBA::Format { 0, 0.15, 0.87, 1 })),  //
        };

        const std::array cubeSimpleMaterials {
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(xBack)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(xFront)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(yBack)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(yFront)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(zBack)),
            engine.storage.createSimpleMaterial(cubeDiffuseTextures.at(zFront)),
        };

        const std::array cubePhongMaterials {
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(xBack), cubeSpecularTextures.at(x),
                                               cubeNormalTextures.at(x)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(xFront), cubeSpecularTextures.at(x),
                                               cubeNormalTextures.at(x)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(yBack), cubeSpecularTextures.at(y),
                                               cubeNormalTextures.at(y)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(yFront), cubeSpecularTextures.at(y),
                                               cubeNormalTextures.at(y)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(zBack), cubeSpecularTextures.at(z),
                                               cubeNormalTextures.at(z)),
            engine.storage.createPhongMaterial(cubeDiffuseTextures.at(zFront), cubeSpecularTextures.at(z),
                                               cubeNormalTextures.at(z)),
        };

        const auto texturedCube = surge::createArray<surge::Entity, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face = engine.loader.load(surge::ShaderType::primitiveTextured, planeTexturedModel,
                                      cubeSimpleMaterials.at(faceId));
            engine.update(face, surge::translate<x>(2.0) * cubeFaces.at(faceId));
        });

        const auto planeTexturedNormalsModel = engine.storage.createModel(surge::geom::planeTexturedNormals);
        const auto texturedNormalCube =
            surge::createArray<surge::Entity, cubeFaces.size()>([&]<int faceId>(auto& face) {
                face = engine.loader.load(surge::ShaderType::primitiveTexturedNormal, planeTexturedNormalsModel,
                                          cubeSimpleMaterials.at(faceId));
            });

        const auto phongCube = surge::createArray<surge::Entity, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face = engine.loader.load(surge::ShaderType::phongModel, planeTexturedNormalsModel,
                                      cubePhongMaterials.at(faceId));
        });

        const auto planeTexturedNormalTangentModel = engine.storage.createModel(surge::geom::planeNormalTangentTexture);

        const auto phongNormalCube = surge::createArray<surge::Entity, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face = engine.loader.load(surge::ShaderType::phongModelNormal, planeTexturedNormalTangentModel,
                                      cubePhongMaterials.at(faceId));
        });

        std::vector<surge::Entity> brickwalls;
        const auto                 brickwallMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(surgeTextureFolder / "brickwall_diffuse.jpg", surge::Texture::texture2d),
            engine.storage.blackTextureId,
            engine.storage.createTexture(surgeTextureFolder / "brickwall_normal.jpg", surge::Texture::texture2dNorm));

        for (const auto& translation : generateTranslations<10>()) {
            const auto brickwall = engine.loader.load(surge::ShaderType::phongModelNormal,
                                                      planeTexturedNormalTangentModel, brickwallMaterial);
            engine.update(brickwall, surge::Translation { translation } * surge::scale(4.0) * surge::rotate<x>(90));
            brickwalls.emplace_back(brickwall);
        }

        const auto crateMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(surgeTextureFolder / "container_diffuse.png", surge::Texture::texture2d),
            engine.storage.createTexture(surgeTextureFolder / "container_specular.png", surge::Texture::texture2d),
            engine.storage.whiteTextureId);
        const auto crate = surge::createArray<surge::Entity, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face =
                engine.loader.load(surge::ShaderType::phongModelNormal, planeTexturedNormalTangentModel, crateMaterial);
        });

        const auto floor =
            engine.loader.load(surge::ShaderType::phongModelNormal, planeTexturedNormalTangentModel, brickwallMaterial);


        // ===========================================================================================
        const auto dragon = engine.loader.load<surge::geom::PositionNormal>(
            surge::ShaderType::primitiveNormal,
            surge::load::Gltf::Handle { vulkanAssetFolder / "models/chinesedragon.gltf" });

        const std::filesystem::path cerberusFolder { vulkanAssetFolder / "models/cerberus" };
        const surge::GltfHandle     cerberusGltfHandle { cerberusFolder / "cerberus.gltf" };
        using TextureType = surge::load::Gltf::TextureType;
        const std::map<TextureType, surge::TextureID> cerberusTextures {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(cerberusFolder / "albedo.ktx",   surge::Texture::texture2d) },
            { TextureType::metallicRoughnessTexture,
             engine.storage.createTexture(cerberusFolder / "metallic.ktx", surge::Texture::metallic)  },
            { TextureType::normalTexture,
             engine.storage.createTexture(cerberusFolder / "normal.ktx",   surge::Texture::texture2d) },
        };
        const auto cerberus = engine.loader.load<surge::geom::PositionNormalTangentTexture>(
            surge::ShaderType::phongModelNormal, cerberusGltfHandle, cerberusTextures);

        const auto cesiumMan = engine.loader.load<surge::geom::PositionNormalTexture>(
            surge::ShaderType::primitiveTexturedNormal,
            surge::GltfHandle { vulkanAssetFolder / "models/CesiumMan/glTF-Embedded/CesiumMan.gltf" });

        const auto buggy = engine.loader.load<surge::geom::PositionNormal>(
            surge::ShaderType::primitiveNormal,
            surge::GltfHandle { vulkanAssetFolder / "models/gltf/glTF-Embedded/Buggy.gltf" });

        const std::filesystem::path                   armorFolder { vulkanAssetFolder / "models/armor" };
        const std::map<TextureType, surge::TextureID> armorTextures1 {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(armorFolder / "colormap_rgba.ktx", surge::Texture::texture2d) },
            { TextureType::metallicRoughnessTexture, engine.storage.whiteTextureId },
            { TextureType::normalTexture,
             engine.storage.createTexture(armorFolder / "normalmap_rgba.ktx", surge::Texture::texture2d) },
        };
        const auto armor1 = engine.loader.load<surge::geom::PositionNormalTangentTexture>(
            surge::ShaderType::phongModelNormal, surge::load::Gltf::Handle { armorFolder / "armor.gltf" },
            armorTextures1);

        const std::map<TextureType, surge::TextureID> armorTextures2 {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(armorFolder / "colormap_rgba.ktx", surge::Texture::texture2d) },
        };
        const auto armor2 = engine.loader.load<surge::geom::PositionNormalTexture>(
            surge::ShaderType::primitiveTexturedNormal, surge::load::Gltf::Handle { armorFolder / "armor.gltf" },
            armorTextures2);

        const auto oaktree = engine.loader.load<surge::geom::PositionNormalTexture>(
            surge::ShaderType::primitiveTexturedNormal,
            surge::GltfHandle { vulkanAssetFolder / "models/oaktree.gltf" });

        const auto pathfinder = engine.loader.load<surge::geom::PositionNormalTangentTexture>(
            surge::ShaderType::phongModelNormal,
            surge::GltfHandle { "/home/ico/projects/uploads_files_2619136_Pathfinder_2k/Pathfinder_2k.glb" });


        double elapsedTime = {};
        auto   start       = std::chrono::high_resolution_clock::now();
        while (engine.input.proceed) {
            if (elapsedTime > 1.0 / 144.0) {
                engine.input.reset();
                engine.context.pollEvents();

                // === update ===
                mainScene.entities.clear();

                playerCamera.update(engine.input, engine.context.window.resolution);
                skyboxCamera.update(engine.input, engine.context.window.resolution);

                engine.updateBuffer(mainScene.bufferId,
                                    surge::Storage::SceneBuffer { surge::fullMatrix(playerCamera.mats.perspective),
                                                                  surge::fullMatrix(playerCamera.mats.view), lightColor,
                                                                  lightPosition });

                // channels
                const surge::Rotation rotationY { surge::toQuaternion(0.0f, 1.0f * engine.input.timer, 0.0f) };
                const surge::Rotation rotationX { surge::toQuaternion(1.0f * engine.input.timer, 0.0f, 0.0f) };

                // move light
                lightPosition[1] = std::sin(engine.input.timer);

                // rotate
                engine.update(skybox, skyboxCamera.mats.perspective * skyboxCamera.mats.view);
                surge::forEach<0, cubeFaces.size()>([&]<int face>() {
                    engine.update(lightCube.at(face), surge::Translation<> { lightPosition } *
                                                          surge::Scaling<> { 0.1f, 0.1f, 0.1f } * cubeFaces.at(face));
                    engine.update(texturedNormalCube.at(face),
                                  surge::translate<x>(4.0) * rotationY * cubeFaces.at(face));
                    engine.update(phongCube.at(face), surge::translate<x>(10.0) * rotationY * cubeFaces.at(face));
                    engine.update(phongNormalCube.at(face), surge::translate<x>(10.0) * surge::translate<y>(2.0) *
                                                                rotationY * cubeFaces.at(face));
                    engine.update(crate.at(face), surge::translate<x>(12.0) * rotationY * cubeFaces.at(face));
                });
                engine.storage.reset();
                engine.update(floor, surge::translate<x>(-2.0) * rotationY * surge::rotate<x>(90));
                engine.update(dragon, surge::translate<x>(6.0) * surge::scale(0.5) * rotationY);
                engine.update(cerberus, surge::translate<x>(8.0) * surge::scale(0.5) * rotationY);
                engine.update(cesiumMan, surge::translate<x>(-4.0) * rotationY);
                engine.update(buggy, surge::translate<x>(-6.0) * surge::scale(0.01) * rotationY);
                engine.update(armor1, surge::translate<x>(-8.0) * surge::scale(0.3) * rotationY);
                engine.update(armor2,
                              surge::translate<x>(-8.0) * surge::translate<z>(2.0) * surge::scale(0.3) * rotationY);
                engine.update(oaktree, surge::translate<x>(-10.0) * rotationY);
                engine.update(pathfinder, surge::translate<z>(4.0) * rotationY);

                // create scene
                mainScene.entities.push_back(skybox);
                mainScene.entities.push_back(coordinates);
                mainScene.entities.push_back(floor);
                mainScene.entities.insert(mainScene.entities.end(), lightCube.begin(), lightCube.end());
                mainScene.entities.insert(mainScene.entities.end(), texturedCube.begin(), texturedCube.end());
                mainScene.entities.insert(mainScene.entities.end(), texturedNormalCube.begin(),
                                          texturedNormalCube.end());
                mainScene.entities.insert(mainScene.entities.end(), phongCube.begin(), phongCube.end());
                mainScene.entities.insert(mainScene.entities.end(), phongNormalCube.begin(), phongNormalCube.end());
                mainScene.entities.insert(mainScene.entities.end(), brickwalls.begin(), brickwalls.end());
                mainScene.entities.insert(mainScene.entities.end(), crate.begin(), crate.end());
                mainScene.entities.push_back(dragon);
                mainScene.entities.push_back(cerberus);
                mainScene.entities.push_back(cesiumMan);
                mainScene.entities.push_back(buggy);
                mainScene.entities.push_back(armor1);
                mainScene.entities.push_back(armor2);
                mainScene.entities.push_back(oaktree);
                mainScene.entities.push_back(pathfinder);


                // render
                const auto commandBuffer = engine.presenter.acquire();
                engine.presenter.beginRendering();

                engine.renderer.draw(commandBuffer, mainScene);

                surge::forEach<0, texturedNormalCube.size(), 0, 2>([&]<int face, int triangle>() {
                    using namespace surge::geom;
                    constexpr auto& vertices = planeTexturedNormals.vertices;
                    constexpr auto& indices  = planeTexturedNormals.indices;
                    constexpr auto  offset   = triangle * 3;

                    constexpr auto a = (vertices.at(indices.at(offset + 0)).get<Attribute::position>() +
                                        vertices.at(indices.at(offset + 1)).get<Attribute::position>() +
                                        vertices.at(indices.at(offset + 2)).get<Attribute::position>()) /
                                       3.0f;
                    constexpr auto b = a + (vertices.at(indices.at(offset + 0)).get<Attribute::normal>() +
                                            vertices.at(indices.at(offset + 1)).get<Attribute::normal>() +
                                            vertices.at(indices.at(offset + 2)).get<Attribute::normal>()) /
                                               3.0f;

                    const auto  nodeTreeId     = texturedNormalCube.at(face).nodeTreeId;
                    const auto& transformation = engine.storage.nodeTrees.at(nodeTreeId).get(0).transformation;
                    engine.renderer.draw(commandBuffer, mainScene,
                                         surge::Line {
                                             .a     = transform(a, transformation),
                                             .b     = transform(b, transformation),
                                             .color = surge::RGBA::white,
                                         });
                });

                engine.presenter.endRendering();
                engine.presenter.present(engine.command);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop     = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration<double, std::milli>(stop - start).count();
            elapsedTime         = 1e-3 * duration;
        }

        vkDeviceWaitIdle(engine.context.device);
    } catch (const std::exception& e) {
        surge::log::error(e.what());
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "\033[1;31m[surge of ERROR]\033[0m" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
