#pragma once

#include "surge/overlay/Overlay.hpp"
#include "surge/core/colors.hpp"
#include "surge/core/Presenter.hpp"
#include "surge/Renderer.hpp"
#include "surge/Storage.hpp"


// #include "surge/physics/Physics.hpp"
#include "surge/entity/Entity.hpp"
#include "surge/entity/Skybox.hpp"

#include "surge/Log.hpp"

#include <type_traits>

#define sqrt2 1.41421356237f
#define sqrt2o2 0.70710678118f

namespace surge {

using vec3 = core::math::Vector<3>;
struct triangle3 {
    vec3 a;
    vec3 b;
    vec3 c;
};
std::optional<vec3> ray_intersects_triangle(const vec3& ray_origin, const vec3& ray_vector, const triangle3& triangle) {
    constexpr float epsilon = std::numeric_limits<float>::epsilon();

    vec3 edge1 = triangle.b - triangle.a;
    vec3 edge2 = triangle.c - triangle.a;

    // Backface culling, assuming CCW-wound triangles.
    const vec3 normal = cross(edge1, edge2);  // No need to normalize
    if (dot(normal, ray_vector) > 0) {
        return std::nullopt;
    }

    vec3  ray_cross_e2 = cross(ray_vector, edge2);
    float det          = dot(edge1, ray_cross_e2);

    if (abs(det) < epsilon) {
        return std::nullopt;  // Ray is parallel to triangle
    }

    float inv_det = 1.0 / det;
    vec3  s       = ray_origin - triangle.a;
    float u       = inv_det * dot(s, ray_cross_e2);

    if (u < -epsilon || u - 1 > epsilon) {
        return std::nullopt;  // Ray passes outside edge2's bounds
    }

    vec3  s_cross_e1 = cross(s, edge1);
    float v          = inv_det * dot(ray_vector, s_cross_e1);

    if (v < -epsilon || u + v - 1 > epsilon) {
        return std::nullopt;  // Ray passes outside edge1's bounds
    }

    // The ray line intersects with the triangle.
    // We compute t to find where on the ray the intersection is.
    float t = inv_det * dot(edge2, s_cross_e1);

    if (t > epsilon)  // Ray intersection
    {
        return vec3(ray_origin + ray_vector * t);
    } else {
        // This means that there is a line intersection but not a ray intersection.
        return std::nullopt;
    }
}

template<typename Transformation>
core::math::Vector<3> transform(const core::math::Vector<3>& point, const Transformation& transformation) {
    using namespace core::math;
    const Vector<4> p0 {
        get<0>(point),
        get<1>(point),
        get<2>(point),
        one<ValueType<Vector<4>>>,
    };
    const auto p1 = transformation * p0;
    return Vector<3> {
        get<0>(p1),
        get<1>(p1),
        get<2>(p1),
    };
}

double elapsed(auto start) {
    const auto stop = std::chrono::high_resolution_clock::now();
    return 1e-3 * std::chrono::duration<double, std::milli>(stop - start).count();
}

template<typename... Textures>
auto createDescriptorSet(const core::Context& context, const Textures&... textures) {
    constexpr uint32_t texturesCount { sizeof...(Textures) };

    // descriptor pool
    const std::array poolSizes {
        VkDescriptorPoolSize {
                              .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                              .descriptorCount = sizeof...(Textures),
                              }
    };
    const auto descriptorPool = context.create(VkDescriptorPoolCreateInfo {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = {},
        .maxSets       = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data(),
    });

    // descriptor set layout
    std::array<VkDescriptorSetLayoutBinding, texturesCount> bindings;
    core::forEach<0, bindings.size()>([&]<int binding>() {
        bindings[binding] = VkDescriptorSetLayoutBinding {
            .binding            = binding,
            .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
    });
    const auto descriptorSetLayout = context.create(VkDescriptorSetLayoutCreateInfo {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext        = nullptr,
        .flags        = {},
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings    = bindings.data(),
    });

    // descriptor set
    const VkDescriptorSetAllocateInfo allocInfo {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext              = nullptr,
        .descriptorPool     = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &descriptorSetLayout,
    };

    VkDescriptorSet descriptorSet;
    if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    // write descriptor set
    std::array<VkWriteDescriptorSet, texturesCount> descriptorWrites;
    core::forEach<0, descriptorWrites.size()>([&]<int binding>() {
        const auto& texture = std::get<binding>(std::forward_as_tuple(textures...));

        descriptorWrites[binding] = VkWriteDescriptorSet {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = descriptorSet,
            .dstBinding       = binding,
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo       = texture.imageInfo(),
            .pBufferInfo      = texture.bufferInfo(),
            .pTexelBufferView = nullptr,
        };
    });
    vkUpdateDescriptorSets(context.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0,
                           nullptr);

    return std::make_tuple(descriptorPool, descriptorSetLayout, descriptorSet);
}

template<int radius>
constexpr auto generateTranslations() {
    constexpr auto                          length = 2 * radius + 1;
    constexpr auto                          size   = length * length;
    std::array<core::math::Vector<3>, size> translations;
    core::forEach<0, length, 0, length>([&]<int i, int j>() {
        constexpr auto index = i * length + j;
        translations[index]  = core::math::Vector<3> { 4 * (i - radius), -3, 4 * (j - radius) };
    });
    return translations;
}


enum Coordinate {
    x = 0,
    y,
    z,
};

template<Coordinate c>
constexpr auto translate(const float t) {
    if constexpr (c == x) {
        return core::math::Translation<> { t, 0, 0 };
    } else if constexpr (c == y) {
        return core::math::Translation<> { 0, t, 0 };
    } else if constexpr (c == z) {
        return core::math::Translation<> { 0, 0, t };
    } else {
        throw;
    }
};

template<Coordinate c>
constexpr auto rotate(const float d) {
    const auto coef = d > 0 ? -sqrt2o2 : sqrt2o2;
    if constexpr (c == x) {
        return core::math::Rotation<> {
            core::math::Quaternion<> { coef, 0, 0, sqrt2o2 }
        };
    } else if constexpr (c == y) {
        return core::math::Rotation<> {
            core::math::Quaternion<> { 0, coef, 0, sqrt2o2 }
        };
    } else if constexpr (c == z) {
        return core::math::Rotation<> {
            core::math::Quaternion<> { 0, 0, coef, sqrt2o2 }
        };
    } else {
        throw;
    }
}

template<Coordinate c>
constexpr auto flip() {
    return rotate<c>(90) * rotate<c>(90);
}


class Engine {
public:
    Engine(const std::string& windowName, const std::string& appName, const core::Window::Resolution& resolution)
        : input {}
        , context { windowName, appName, resolution, Callbacks { input } }
        , command { context }
        , presenter { command }
        , defaults { command }
        , renderer { context }
        , overlay { command, assets }
        , mainCamera { renderer.descriptor.setLayout, renderer.descriptor.set }
        , storage { command, defaults, mainCamera } {
        log::checkpoint("The surge of urge to purge started");
    }

    void loadAsset(const std::string& name, const load::AssetHandle& handle) {
        if (assets.contains(name)) {
            throw std::runtime_error("Asset [" + name + "] already exits");
        }
        const auto start = std::chrono::high_resolution_clock::now();
        std::visit(
            core::overload {
                [&](const load::LoadedTexture::Handle& handle) {
                    switch (handle.type) {
                    case load::LoadedTexture::Type::texture2d:
                        textures.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                         std::forward_as_tuple(command, load::LoadedTexture { handle },
                                                               load::Defaults::sampler, asset::Texture::texture2d));
                        break;
                    case load::LoadedTexture::Type::cube:
                        textures.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                         std::forward_as_tuple(command, load::LoadedTexture { handle },
                                                               load::Defaults::sampler, asset::Texture::cube));
                        break;
                    }
                    log::info(core::math::toString(elapsed(start)) + " Loaded texture asset " + handle.path.string());
                },
                [&](const load::LoadedSkybox::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::LoadedSkybox { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded skybox asset " +
                              handle.texturePath.string());
                },
                [&](const load::Gltf::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Gltf { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;

                    // renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                    //                         asset.materialDescriptorSetLayout,
                    //                         asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded gltf asset " + handle.path.string());
                },
                [&](const load::Obj::Handle& handle) {
                    const auto [iter, inserted] =
                        assets.emplace(std::piecewise_construct, std::forward_as_tuple(name),
                                       std::forward_as_tuple(command, load::Obj { handle, defaults }));
                    assert(inserted);
                    const auto& [_, asset] = *iter;
                    renderer.createPipeline(name, asset.vertexInputState, asset.shader,
                                            asset.materialDescriptorSetLayout, asset.jointMatricesDescriptorSetLayout);
                    log::info(core::math::toString(elapsed(start)) + " Loaded obj asset " + handle.meshPath.string());
                },
            },
            handle);
    }

    entity::Entity createEntity(const std::string& name, const core::math::StaticMatrix auto& matrix) {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.contains(name) ?
                                                    renderer.pipelines.at(name) :
                                                    std::pair { VK_NULL_HANDLE, VK_NULL_HANDLE };
        return entity::Entity { asset, pipelineLayout, pipeline, matrix };
    }

    entity::Skybox createSkybox(const std::string& name) {
        const auto& asset                     = assets.at(name);
        const auto [pipelineLayout, pipeline] = renderer.pipelines.at(name);
        return entity::Skybox { asset, pipelineLayout, pipeline, core::math::identity<4> };
    }

    ~Engine() {
        log::checkpoint("The surge of urge to purge terminated");
    }

    void run() {
        double elapsedTime = {};
        auto   start       = std::chrono::high_resolution_clock::now();

        Camera<false> playerCamera {
            16.0 / 9.0,
            { 0.0f, 3.0f,  4.0f  },
            { 0.0f, -0.5f, -1.0f },
        };
        Camera<true> skyboxCamera {
            16.0 / 9.0,
            { 0.0f, 0.0f, 0.0f  },
            { 0.0f, 0.0f, -1.0f },
        };
        Camera<false> lightCamera {
            16.0 / 9.0,
            { -1.0f, 1.0f,  3.0f  },
            { -1.0f, -1.0f, -1.0f },
        };

        auto skybox = createSkybox("skybox");

        // === initialize ===

        const Entity coordinates {
            // coordinate system
            .model    = storage.createModel(core::geometry::coordinates),
            .pipeline = storage.createPipeline<core::geometry::PositionAndColor, PushConstants>(
                core::shader::Type::coordinates),
            .matrix   = storage.createMatrix(PushConstants {
                  .matrix    = core::math::fullMatrix(core::math::identity<4>),
                  .baseColor = core::RGBA::white,
            }),
            .material = {},
        };

        std::vector<Entity> cubes;
        enum { xBack = 0, xFront, yBack, yFront, zBack, zFront };

        constexpr std::array cubeFaceMatrices {
            translate<x>(-0.5) * rotate<y>(+90),         //
            translate<x>(+0.5) * rotate<y>(-90),         //
            translate<y>(-0.5) * rotate<x>(-90),         //
            translate<y>(+0.5) * rotate<x>(+90),         //
            translate<z>(-0.5) * flip<x>(),              //
            core::math::fullMatrix(translate<z>(+0.5)),  //
        };

        const auto planeModel = storage.createModel(core::geometry::plane);
        const auto primitivePipeline =
            storage.createPipeline<core::geometry::Position, PushConstants>(core::shader::Type::primitive);

        constexpr core::math::Vector<3> lightPosition { -2, 2, 1 };
        constexpr auto                  lightColor = core::RGBA::white;

        const auto lightCube = std::invoke([&]() {
            std::array<Entity, cubeFaceMatrices.size()> faces;
            constexpr core::math::Translation<>         T { lightPosition };
            constexpr core::math::Scaling<>             S { 0.1f, 0.1f, 0.1f };
            core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                constexpr PushConstants matrix { T * S * cubeFaceMatrices.at(face), lightColor };
                faces[face] = { planeModel, primitivePipeline, storage.createMatrix(matrix), MaterialID {} };
            });
            return faces;
        });

        // constexpr core::math::Vector<3> lightPosition { -2, 2, 1 };
        // constexpr auto                  lightColor = core::RGBA::white;
        // {  // light cube
        //     constexpr uint32_t                  isLight {};
        //     constexpr core::math::Translation<> T { lightPosition };
        //     constexpr core::math::Scaling<>     S { 0.1f, 0.1f, 0.1f };
        //     core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
        //         constexpr PushConstants matrix { T * S * cubeFaceMatrices.at(face), core::RGBA::white, isLight };
        //         cubes.emplace_back(planeModel, primitivePipeline, storage.createMatrix(matrix), MaterialID {});
        //     });
        // }

        const auto untexturedCube = std::invoke([&]() {
            std::array<Entity, cubeFaceMatrices.size()> faces;
            constexpr core::math::Translation<>         T { 0, 0, 0 };
            constexpr std::array                        cubeFaceColors {
                core::RGBA::darkRed, core::RGBA::red,      core::RGBA::darkGreen,
                core::RGBA::green,   core::RGBA::darkBlue, core::RGBA::blue,
            };
            core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                constexpr PushConstants matrix { T * cubeFaceMatrices.at(face), cubeFaceColors.at(face) };
                faces[face] = { planeModel, primitivePipeline, storage.createMatrix(matrix), MaterialID {} };
            });
            return faces;
        });

        // {  // untextured cube
        //     constexpr core::math::Translation<> T { 0, 0, 0 };
        //     constexpr std::array                cubeFaceColors {
        //         core::RGBA::darkRed, core::RGBA::red,      core::RGBA::darkGreen,
        //         core::RGBA::green,   core::RGBA::darkBlue, core::RGBA::blue,
        //     };
        //     core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
        //         constexpr PushConstants matrix { T * cubeFaceMatrices.at(face), cubeFaceColors.at(face) };
        //         cubes.emplace_back(planeModel, primitivePipeline, storage.createMatrix(matrix), MaterialID {});
        //     });
        // }

        const std::array cubeDiffuseTextures {
            storage.createTexture(load::createTextureDataX(core::RGBA::darkRed, core::RGBA::black)),    //
            storage.createTexture(load::createTextureDataX(core::RGBA::red, core::RGBA::black)),        //
            storage.createTexture(load::createTextureDataY(core::RGBA::darkGreen, core::RGBA::black)),  //
            storage.createTexture(load::createTextureDataY(core::RGBA::green, core::RGBA::black)),      //
            storage.createTexture(load::createTextureDataZ(core::RGBA::darkBlue, core::RGBA::black)),   //
            storage.createTexture(load::createTextureDataZ(core::RGBA::blue, core::RGBA::black)),       //
        };

        const std::array cubeSpecularTextures {
            storage.createTexture(load::createTextureDataX(core::RGBA::black, core::RGBA::white)),  //
            storage.createTexture(load::createTextureDataY(core::RGBA::black, core::RGBA::white)),  //
            storage.createTexture(load::createTextureDataZ(core::RGBA::black, core::RGBA::white)),  //
        };

        const std::array cubeNormalTextures {
            storage.createTexture(load::createDefaultTextureData(core::RGBA::blue, core::RGBA::blue)),  //
            storage.createTexture(load::createDefaultTextureData(core::RGBA::blue, core::RGBA::blue)),  //
            storage.createTexture(load::createDefaultTextureData(core::RGBA::blue, core::RGBA::blue)),  //
        };

        const std::array cubeSimpleMaterials {
            storage.createSimpleMaterial(cubeDiffuseTextures.at(xBack)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(xFront)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(yBack)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(yFront)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(zBack)),
            storage.createSimpleMaterial(cubeDiffuseTextures.at(zFront)),
        };

        const std::array cubePhongMaterials {
            storage.createPhongMaterial(cubeDiffuseTextures.at(xBack), cubeSpecularTextures.at(x),
                                        cubeNormalTextures.at(x)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(xFront), cubeSpecularTextures.at(x),
                                        cubeNormalTextures.at(x)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(yBack), cubeSpecularTextures.at(y),
                                        cubeNormalTextures.at(y)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(yFront), cubeSpecularTextures.at(y),
                                        cubeNormalTextures.at(y)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(zBack), cubeSpecularTextures.at(z),
                                        cubeNormalTextures.at(z)),
            storage.createPhongMaterial(cubeDiffuseTextures.at(zFront), cubeSpecularTextures.at(z),
                                        cubeNormalTextures.at(z)),
        };

        {  // textured cube
            const auto model    = storage.createModel(core::geometry::planeTextured);
            const auto pipeline = storage.createPipeline<core::geometry::PositionTexture, PushConstants>(
                core::shader::Type::primitiveTextured, storage.simpleMaterialLayout);
            constexpr auto                      color { core::RGBA::white };
            constexpr core::math::Translation<> T { 2, 0, 0 };
            core::forEach<0, 6>([&]<int face>() {
                constexpr PushConstants matrix { T * cubeFaceMatrices.at(face), color };
                cubes.emplace_back(model, pipeline, storage.createMatrix(matrix), cubeSimpleMaterials.at(face));
            });
        }

        const auto planeTexturedNormalsModel = storage.createModel(core::geometry::planeTexturedNormals);
        {  // textured normal cube
            const auto model    = planeTexturedNormalsModel;
            const auto pipeline = storage.createPipeline<core::geometry::PositionNormalTexture, PushConstants>(
                core::shader::Type::primitiveTexturedNormal, storage.simpleMaterialLayout);
            constexpr auto                      color { core::RGBA::white };
            constexpr core::math::Translation<> T { 4, 0, 0 };
            core::forEach<0, 6>([&]<int face>() {
                constexpr PushConstants matrix { T * cubeFaceMatrices.at(face), color };
                cubes.emplace_back(model, pipeline, storage.createMatrix(matrix), cubeSimpleMaterials.at(face));
            });
        }

        const auto planeTexturedNormalTangentModel = storage.createModel(core::geometry::planeNormalTangentTexture);
        const auto phongModelNormalPipeline =
            storage.createPipeline<core::geometry::PositionNormalTangentTexture, PushConstants>(
                core::shader::Type::phongModelNormal, storage.phongMaterialLayout);

        {  // textured normal tangent cube
            const auto                          model    = planeTexturedNormalTangentModel;
            const auto                          pipeline = phongModelNormalPipeline;
            constexpr auto                      color { core::RGBA::white };
            constexpr core::math::Translation<> T { 4, 2, 0 };
            core::forEach<0, 6>([&]<int face>() {
                constexpr PushConstants matrix { T * cubeFaceMatrices.at(face), color };
                cubes.emplace_back(model, pipeline, storage.createMatrix(matrix), cubePhongMaterials.at(face));
            });
        }

        std::vector<Entity>         brickwalls;
        const std::filesystem::path brickwallFolder { "/home/ico/projects/surge/textures" };
        const auto                  brickwallMaterial = storage.createPhongMaterial(
            storage.loadTexture(brickwallFolder / "brickwall_diffuse.jpg", asset::Texture::texture2d),
            storage.blackTextureId,
            storage.loadTexture(brickwallFolder / "brickwall_normal.jpg", asset::Texture::texture2dNorm));
        {  // brickwall
            const auto model    = storage.createModel(core::geometry::square);
            const auto pipeline = storage.createPipeline<load::Gltf::Vertex, PushConstants>(
                core::shader::Type::shader, storage.phongMaterialLayout);
            constexpr auto radius { 10 };
            constexpr auto translations { generateTranslations<radius>() };
            core::forEach<0, translations.size()>([&]<int i>() {
                constexpr core::math::Translation translation { translations.at(i) };
                constexpr core::math::Rotation    rotation {
                    core::math::Quaternion<> { sqrt2o2, -sqrt2o2, 0, 0 }
                };
                constexpr core::math::Scaling scaling {
                    core::math::Vector<3> { 4, 4, 4 }
                };
                const PushConstants matrix {
                    .matrix    = translation * rotation * scaling,
                    .baseColor = core::Colors<core::Type::rgba>::coral,
                };
                brickwalls.emplace_back(model, pipeline, storage.createMatrix(matrix), brickwallMaterial);
            });
        }

        // constexpr auto dragonMatrix = rotate<x>(-90) * core::math::Scaling<> { 0.1, 0.1, 0.1 };
        // const Entity   dragon {
        //     // coordinate system
        //       .model = storage.createAsset<core::geometry::PositionNormal>(
        //         load::Gltf::Handle { "/home/ico/projects/extern/Vulkan/assets/models/chinesedragon.gltf" }),
        //       .pipeline =
        //       storage.createPipeline<core::geometry::PositionNormal>(core::shader::Type::primitiveNormal), .matrix =
        //       storage.createMatrix(PushConstants {
        //             .matrix    = dragonMatrix,
        //             .baseColor = core::RGBA::white,
        //             .isLight   = {},
        //     }),
        //       .material = {},
        // };

        constexpr auto              cerberusMatrix = rotate<x>(90);
        const std::filesystem::path cerberusFolder { "/home/ico/projects/extern/Vulkan/assets/models/cerberus" };
        const Entity                cerberus {
            // coordinate system
                           .model = storage.createAsset<core::geometry::PositionNormalTexture>(
                load::Gltf::Handle { cerberusFolder / "cerberus.gltf" }),
                           .pipeline = storage.createPipeline<core::geometry::PositionNormalTexture, PushConstants>(
                core::shader::Type::phongModel, storage.phongMaterialLayout),
                           .matrix   = storage.createMatrix(PushConstants {
                                 .matrix    = core::math::fullMatrix(cerberusMatrix),
                                 .baseColor = core::RGBA::white,
            }),
                           .material = storage.createPhongMaterial(
                storage.loadTexture(cerberusFolder / "albedo.ktx", asset::Texture::texture2d),
                storage.loadTexture(cerberusFolder / "metallic.ktx", asset::Texture::metallic),
                storage.loadTexture(cerberusFolder / "normal.ktx", asset::Texture::texture2d)),
        };

        const auto phongPipeline = storage.createPipeline<core::geometry::PositionNormalTexture, PushConstants>(
            core::shader::Type::phongModel, storage.phongMaterialLayout);
        std::vector<Entity> phongCube;
        {  // phong cube
            const auto                          model    = planeTexturedNormalsModel;
            const auto                          pipeline = phongPipeline;
            constexpr auto                      color { core::RGBA::white };
            constexpr core::math::Translation<> T { 0, 0, 0 };
            core::forEach<0, 6>([&]<int face>() {
                constexpr PushConstants matrix { T * cubeFaceMatrices.at(face), color };
                phongCube.emplace_back(model, pipeline, storage.createMatrix(matrix), cubePhongMaterials.at(face));
            });
        }


        const auto crateDiffuseTexture { storage.loadTexture("/home/ico/projects/surge/textures/container_diffuse.png",
                                                             asset::Texture::texture2d) };
        const auto crateSpecularTexture { storage.loadTexture(
            "/home/ico/projects/surge/textures/container_specular.png", asset::Texture::texture2d) };

        const auto crateMaterial = storage.createPhongMaterial(
            storage.loadTexture("/home/ico/projects/surge/textures/container_diffuse.png", asset::Texture::texture2d),
            storage.loadTexture("/home/ico/projects/surge/textures/container_specular.png", asset::Texture::texture2d),
            storage.defaultTextureId);

        std::vector<Entity> crate;
        {  // crate
            const auto     model    = planeTexturedNormalsModel;
            const auto     pipeline = phongPipeline;
            constexpr auto color { core::RGBA::white };
            core::forEach<0, 6>([&]<int face>() {
                constexpr PushConstants matrix { translate<x>(12.0) * cubeFaceMatrices.at(face), color };
                crate.emplace_back(model, pipeline, storage.createMatrix(matrix), crateMaterial);
            });
        }

        constexpr auto floorMatrix = translate<x>(-2.0);
        const Entity   floor {
            // coordinate system
              .model    = planeTexturedNormalTangentModel,
              .pipeline = phongModelNormalPipeline,
              .matrix   = storage.createMatrix(
                PushConstants { .matrix = core::math::fullMatrix(floorMatrix), .baseColor = core::RGBA::white }),
              .material = brickwallMaterial,
        };
        // {  // cerberus
        //     const auto diffuse = storage.createTexture("/home/ico/projects/surge/textures/brickwall_diffuse.jpg");
        //     const auto normal  = storage.createTexture("/home/ico/projects/surge/textures/brickwall_normal.jpg");
        // }
        // === initialize ===

        // Phong light model
        //  1) ambient = ambientStrength * ambientColor
        //      - global base illumination
        //      - passed with MV matrices' UBO?
        //  2) diffuse = sum(lights)
        //      - global directional light (Sun's light)
        //      - variable number of point lights
        //      - dedicated preallocated UBO?
        //
        // ...
        //  end) outColor = (ambient + diffuse) * objectColor;

        log::checkpoint("Main loop start");

        while (input.proceed) {
            if (elapsedTime > 1.0 / 144.0) {
                input.reset();
                context.pollEvents();

                // === entity playground ===
                // entities.back().nodes.get(1).state.translation = lightCamera.vecs.position;

                // for (auto& entity : entities) {
                //     entity.update(0, elapsedTime);
                // }
                // === entity playground ===

                playerCamera.update(input, context.window.resolution);
                // playerCamera = Camera<false> { 16.0 / 9.0, lightCamera.vecs.position, -lightCamera.vecs.position
                // };
                skyboxCamera.update(input, context.window.resolution);
                // lightCamera.update(input.timer, context.window.resolution);
                // playerCamera           = lightCamera;
                // renderer.lightPosition = lightPosition;
                skybox.update(skyboxCamera);
                renderer.update(playerCamera, lightColor, lightPosition);
                // overlay.update(input, playerCamera);

                // rotate cube
                const core::math::Rotation rotationY { core::math::toQuaternion(0.0f, 1.0f * input.timer, 0.0f) };
                const core::math::Rotation rotationX { core::math::toQuaternion(1.0f * input.timer, 0.0f, 0.0f) };
                // const core::math::Translation<> translation { 4.0f, 0.5f * std::sin(5.0f * input.timer), 0.0f };
                core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                    storage.matrices[face + 19].matrix = translate<x>(4.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate dragon
                // storage.matrices.at(dragon.matrix).matrix = translate<x>(6.0) * rotationY * dragonMatrix;

                // rotate cerberus
                storage.matrices.at(cerberus.matrix).matrix = translate<x>(8.0) * rotationY * cerberusMatrix;

                // rotate cube
                core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                    const auto matrixId = phongCube.at(face).matrix;
                    storage.matrices[matrixId.get()].matrix =
                        translate<x>(10.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate cube
                core::forEach<0, cubeFaceMatrices.size()>([&]<int face>() {
                    const auto matrixId = crate.at(face).matrix;
                    storage.matrices[matrixId.get()].matrix =
                        translate<x>(12.0) * rotationY * cubeFaceMatrices.at(face);
                });

                // rotate floor
                storage.matrices.at(floor.matrix).matrix = translate<x>(-2.0) * rotationY;

                // === rendering ===
                const auto commandBuffer = presenter.acquire();
                presenter.beginRendering();
                skybox.draw(commandBuffer, renderer.descriptor.set);


                storage.reset();


                storage.draw(commandBuffer, lightCube);

                storage.draw(commandBuffer, coordinates);
                storage.draw(commandBuffer, untexturedCube);

                for (const auto& face : cubes) {
                    storage.draw(commandBuffer, face);
                }
                for (const auto& tile : brickwalls) {
                    storage.draw(commandBuffer, tile);
                }
                // storage.draw(commandBuffer, dragon);
                storage.draw(commandBuffer, cerberus);
                for (const auto& face : phongCube) {
                    storage.draw(commandBuffer, face);
                }
                for (const auto& face : crate) {
                    storage.draw(commandBuffer, face);
                }
                storage.draw(commandBuffer, floor);

                core::forEach<0, cubeFaceMatrices.size(), 0, 2>([&]<int face, int triangle>() {
                    using namespace core::geometry;
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
                    const auto& matrix = storage.matrices[face + 19].matrix;
                    storage.draw(commandBuffer, asset::Line {
                                                    .a     = transform(a, matrix),
                                                    .b     = transform(b, matrix),
                                                    .color = core::Colors<core::Type::rgba>::white,
                                                });
                });

                // === draw ===

                // for (const auto& entity : entities)
                // {
                //     entity.draw(commandBuffer, renderer.descriptor.set);
                // }
                // overlay.draw(commandBuffer);

                presenter.endRendering();
                presenter.present(command);
                // === rendering ===

                start = std::chrono::high_resolution_clock::now();
            }

            const auto stop     = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration<double, std::milli>(stop - start).count();
            elapsedTime         = 1e-3 * duration;
            // log::update("Frame took " + std::to_string(duration));
        }
        log::checkpoint("Main loop end");

        vkDeviceWaitIdle(context.device);
    }

private:
    mutable Input                         input;
    core::Context                         context;
    core::Command                         command;
    core::Presenter                       presenter;
    load::Defaults                        defaults;
    std::map<std::string, asset::Asset>   assets;
    std::map<std::string, asset::Texture> textures;
    Renderer                              renderer;
    const Descriptor                      mainCamera;
    overlay::Overlay                      overlay;
    Storage                               storage;
    // std::map<std::string, Material>       materials;
    // std::map<std::string, Model>          models;
    // std::map<std::string, Animations>     animations;
};
}  // namespace surge