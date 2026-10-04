#include "surge/surge.hpp"

#include <bitset>

template<int radius>
constexpr auto generateTranslations() {
    constexpr auto                                     length = 2 * radius + 1;
    constexpr auto                                     size   = length * length;
    std::array<surge::core::math::Translation<>, size> translations;
    surge::forEach<0, length, 0, length>([&]<int i, int j>() {
        constexpr auto index = i * length + j;
        translations[index]  = surge::core::math::Translation<> { 4 * (i - radius), -3, 4 * (j - radius) };
    });
    return translations;
}


namespace surge::tiamat  // TIAMAT: Template Interfaces for Affine MATrices
{
enum class Entry : uint8_t {
    zero = 0,
    pone,
    mone,
    any,
};

std::string toString(const Entry e) {
    switch (e) {
    case Entry::zero:
        return "zero";
    case Entry::pone:
        return "+one";
    case Entry::mone:
        return "-one";
    case Entry::any:
        return "any!";
    default:
        throw("bad");
    }
}

using Layout = uint32_t;

static constexpr Entry entry(const Layout layout, const int position)
// requires(0 <= position && position < 16)
{
    const auto position1 = (layout >> (2 * position)) & 1;
    const auto position2 = (layout >> (2 * position + 1)) & 1;
    return position1 ? (position2 ? Entry::any : Entry::mone) : (position2 ? Entry::pone : Entry::zero);
}

template<uint8_t row, uint8_t col>
    requires(row < 4 && col < 4)
constexpr Entry getEntry(const Layout layout) {
    return entry(layout, row * 4 + col);
}


constexpr Layout set(Layout layout, int position, const Entry entry) {
    auto setBit   = [&](int p) { layout |= (1U << p); };
    auto clearBit = [&](int p) { layout &= ~(1U << p); };

    const auto left  = 2 * position;
    const auto right = 2 * position + 1;
    switch (entry) {
    case Entry::zero:
        clearBit(left);
        clearBit(right);
        return layout;
    case Entry::pone:
        clearBit(left);
        setBit(right);
        return layout;
    case Entry::mone:
        setBit(left);
        clearBit(right);
        return layout;
    case Entry::any:
        setBit(left);
        setBit(right);
        return layout;
    default:
        throw("bad");
    }
}

template<uint8_t row, uint8_t col>
constexpr Layout setEntry(const Layout layout, const Entry entry) {
    return set(layout, row * 4 + col, entry);
}

static constexpr std::size_t count(const Layout layout, const Entry entry) {
    std::size_t count {};
    core::forEach<0, 4, 0, 4>([&]<int row, int col>() {
        if (getEntry<row, col>(layout) == entry) {
            ++count;
        }
    });
    return count;
}

constexpr Entry operator+(const Entry a, const Entry b) {
    static constexpr std::array<std::array<Entry, 4>, 4> table {
        //           Entry::zero  Entry::pone  Entry::mone  Entry::any
        std::array { Entry::zero, Entry::pone, Entry::mone, Entry::any }, // Entry::zero
        std::array { Entry::pone, Entry::any,  Entry::zero, Entry::any }, // Entry::pone
        std::array { Entry::mone, Entry::zero, Entry::any,  Entry::any }, // Entry::mone
        std::array { Entry::any,  Entry::any,  Entry::any,  Entry::any }, // Entry::any
    };
    return table[static_cast<uint8_t>(a)][static_cast<uint8_t>(b)];
}

constexpr Entry operator*(const Entry a, const Entry b) {
    static constexpr std::array<std::array<Entry, 4>, 4> table {
        //           Entry::zero  Entry::pone  Entry::mone  Entry::any
        std::array { Entry::zero, Entry::zero, Entry::zero, Entry::zero }, // Entry::zero
        std::array { Entry::zero, Entry::pone, Entry::mone, Entry::any  }, // Entry::pone
        std::array { Entry::zero, Entry::mone, Entry::pone, Entry::any  }, // Entry::mone
        std::array { Entry::zero, Entry::any,  Entry::any,  Entry::any  }, // Entry::any
    };
    return table[static_cast<uint8_t>(a)][static_cast<uint8_t>(b)];
}

constexpr Layout multiply(const Layout a, const Layout b) {
    Layout c {};
    core::forEach<0, 4, 0, 4, 0, 4>([&]<int row, int col, int mid>() {
        const auto entryA = getEntry<row, mid>(a);
        const auto entryB = getEntry<mid, col>(b);
        const auto entryC = getEntry<row, col>(c);
        c                 = setEntry<row, col>(c, entryC + entryA * entryB);
    });
    return c;
}


template<typename V, Layout l>
    requires(l != 0)
class Matrix {
public:
    using Value                    = V;
    static constexpr Layout layout = l;
    constexpr Matrix()
        : data {} { };

public:
    std::array<Value, count(layout, Entry::any)> data;
};

template<uint8_t row, uint8_t col, Layout layout>
constexpr uint8_t index() {
    int idx { -1 };
    core::forEach<0, row * 4 + col + 1>([&]<int n>() {
        // const auto n = row * 4 + col;
        const auto e = entry(layout, n);
        if (e == Entry::any) {
            ++idx;
        }
    });
    return idx > -1 ? idx : throw("bad");
}

template<typename T>
static constexpr T zero = T { 0 };

template<typename T>
static constexpr T one = T { 1 };


template<uint8_t row, uint8_t col, typename V, Layout l>
constexpr auto& get(const Matrix<V, l>& m) {
    constexpr auto e = getEntry<row, col>(l);
    if constexpr (e == Entry::zero) {
        return zero<V>;
    } else if constexpr (e == Entry::pone) {
        return one<V>;
    } else if constexpr (e == Entry::mone) {
        return -one<V>;
    } else {
        return m.data[index<row, col, l>()];
    }
}

template<uint8_t row, uint8_t col, typename Value, Layout layout>
    requires(getEntry<row, col>(layout) == Entry::any)
constexpr auto& get(Matrix<Value, layout>& m) {
    return m.data[index<row, col, layout>()];
}

template<typename ValueA, Layout layoutA, typename ValueB, Layout layoutB>
constexpr auto operator*(const Matrix<ValueA, layoutA>& A, const Matrix<ValueB, layoutB>& B) {
    constexpr auto                                      layoutC = multiply(layoutA, layoutB);
    Matrix<std::common_type_t<ValueA, ValueB>, layoutC> C {};
    core::forEach<0, 4, 0, 4, 0, 4>([&]<uint8_t row, uint8_t col, uint8_t mid>() {
        if constexpr (getEntry<row, col>(layoutC) == Entry::any) {
            constexpr auto entryA = getEntry<row, mid>(layoutA);
            constexpr auto entryB = getEntry<mid, col>(layoutB);
            auto&          valueC = get<row, col>(C);
            if constexpr (entryA == Entry::any && entryB != Entry::any) {
                const auto&    valueA = get<row, mid>(A);
                constexpr auto valueB = get<mid, col>(B);
                valueC += valueA * valueB;
            } else if constexpr (entryA != Entry::any && entryB == Entry::any) {
                constexpr auto valueA = get<row, mid>(A);
                const auto&    valueB = get<mid, col>(B);
                valueC += valueA * valueB;
            } else if constexpr (entryA == Entry::any && entryB == Entry::any) {
                const auto& valueA = get<row, mid>(A);
                const auto& valueB = get<mid, col>(B);
                valueC += valueA * valueB;
            }
        }
    });
    return C;
}

constexpr Layout layout(const std::array<std::array<Entry, 4>, 4>& table) {
    Layout layout {};
    core::forEach<0, 4, 0, 4>([&]<int i, int j>() {
        const auto n = i * 4 + j;
        layout       = setEntry<i, j>(layout, table[i][j]);
    });
    return layout;
}

using Identity = Matrix<float, layout({
                                   //           Entry::zero  Entry::pone  Entry::mone  Entry::any
                                   std::array { Entry::pone, Entry::zero, Entry::zero, Entry::zero }, // Entry::zero
                                   std::array { Entry::zero, Entry::pone, Entry::zero, Entry::zero }, // Entry::pone
                                   std::array { Entry::zero, Entry::zero, Entry::pone, Entry::zero }, // Entry::mone
                                   std::array { Entry::zero, Entry::zero, Entry::zero, Entry::pone }, // Entry::any
})>;
constexpr Identity identity;

template<typename Mat>
bool testMat() {
    static_assert(multiply(Mat::layout, Mat::layout) == Mat::layout);
    static_assert(multiply(Identity::layout, Mat::layout) == Mat::layout);
    static_assert(multiply(Mat::layout, Identity::layout) == Mat::layout);

    constexpr Mat cm1;
    constexpr Mat cm2;
    static_assert((cm1 * cm1).layout == Mat::layout);

    static_assert((cm1 * identity).layout == Mat::layout);
    static_assert((identity * cm1).layout == Mat::layout);

    static_assert((cm1 * cm2).layout == Mat::layout);
    static_assert((cm2 * cm1).layout == Mat::layout);

    return true;
}

void test() {
    using namespace surge::tiamat;

    // Matrix
    // static_assert(get<0, 0>(identity) == 1);
    static_assert(get<1, 0>(identity) == 0);
    static_assert(get<0, 1>(identity) == 0);
    // static_assert(get<1, 1>(identity) == 1);
    // static_assert(sizeof(identity) == 1);
    std::cout << get<0, 0>(identity) << std::endl;

    using Translation =
        Matrix<float, layout({
                          //           Entry::zero  Entry::pone  Entry::mone  Entry::any
                          std::array { Entry::pone, Entry::zero, Entry::zero, Entry::any  }, // Entry::zero
                          std::array { Entry::zero, Entry::pone, Entry::zero, Entry::any  }, // Entry::pone
                          std::array { Entry::zero, Entry::zero, Entry::pone, Entry::any  }, // Entry::mone
                          std::array { Entry::zero, Entry::zero, Entry::zero, Entry::pone }, // Entry::any
    })>;
    testMat<Translation>();
    Translation translation;
    static_assert(sizeof(Translation) == 3 * sizeof(float));
    static_assert(multiply(Translation::layout, Translation::layout) == Translation::layout);
    static_assert(multiply(Identity::layout, Translation::layout) == Translation::layout);
    static_assert(multiply(Translation::layout, Identity::layout) == Translation::layout);
    constexpr Translation cTr;
    static_assert((cTr * cTr).layout == Translation::layout);
    static_assert((identity * cTr).layout == Translation::layout);
    static_assert((cTr * identity).layout == Translation::layout);
    get<0, 3>(translation) = 1.4;
    constexpr auto t00     = get<0, 0>(translation);
    static_assert(t00 == 1);
    static_assert(get<0, 0>(translation) == 1);
    static_assert(get<0, 1>(translation) == 0);
    {
        constexpr Translation t1 = std::invoke([] {
            Translation t;
            get<0, 3>(t) = 1;
            get<1, 3>(t) = 2;
            get<2, 3>(t) = 3;
            return t;
        });
        static_assert(get<0, 3>(t1 * identity) == 1);
        static_assert(get<1, 3>(t1) == 2);
        static_assert(get<2, 3>(t1) == 3);

        constexpr Translation t2 = std::invoke([] {
            Translation t;
            get<0, 3>(t) = 2;
            get<1, 3>(t) = 3;
            get<2, 3>(t) = 4;
            return t;
        });
        constexpr auto        t3 = t1 * t2;
        static_assert(get<0, 3>(t3) == 3);
        static_assert(get<1, 3>(t3) == 5);
        static_assert(get<2, 3>(t3) == 7);
    }

    Translation translation2;
    get<0, 3>(translation) = 2;

    static_assert(get<0, 0>(translation) == 1);
    static_assert(get<1, 0>(translation) == 0);
    static_assert(get<0, 1>(translation) == 0);
    static_assert(get<1, 1>(translation) == 1);
    // static_assert(get<0, 3>(translation) == 0);
    // get<0,3>
    // static_assert(sizeof(identity) == 1);

    using Rotation = Matrix<float, layout({
                                       //           Entry::zero  Entry::pone  Entry::mone  Entry::any
                                       std::array { Entry::any,  Entry::any,  Entry::any,  Entry::zero }, // Entry::zero
                                       std::array { Entry::any,  Entry::any,  Entry::any,  Entry::zero }, // Entry::pone
                                       std::array { Entry::any,  Entry::any,  Entry::any,  Entry::zero }, // Entry::mone
                                       std::array { Entry::zero, Entry::zero, Entry::zero, Entry::pone }, // Entry::any
    })>;
    testMat<Rotation>();
    Rotation rotation;
    static_assert(sizeof(Rotation) == 9 * sizeof(float));
    static_assert(multiply(Rotation::layout, Rotation::layout) == Rotation::layout);
    static_assert(multiply(Identity::layout, Rotation::layout) == Rotation::layout);
    static_assert(multiply(Rotation::layout, Identity::layout) == Rotation::layout);
    constexpr Rotation cRot;
    static_assert((cRot * cRot).layout == Rotation::layout);
    static_assert((identity * cRot).layout == Rotation::layout);
    static_assert((cRot * identity).layout == Rotation::layout);

    using Scale = Matrix<float, layout({
                                    //           Entry::zero  Entry::pone  Entry::mone  Entry::any
                                    std::array { Entry::any,  Entry::zero, Entry::zero, Entry::zero }, // Entry::zero
                                    std::array { Entry::zero, Entry::any,  Entry::zero, Entry::zero }, // Entry::pone
                                    std::array { Entry::zero, Entry::zero, Entry::any,  Entry::zero }, // Entry::mone
                                    std::array { Entry::zero, Entry::zero, Entry::zero, Entry::pone }, // Entry::any
    })>;
    Scale scale;
    testMat<Scale>();
    {
        constexpr auto s1 = std::invoke([] {
            Scale s;
            get<0, 0>(s) = 1;
            get<1, 1>(s) = 2;
            get<2, 2>(s) = 3;
            return s;
        });
        static_assert(get<0, 0>(s1 * identity) == 1);
        static_assert(get<1, 1>(s1) == 2);
        static_assert(get<2, 2>(s1) == 3);

        constexpr auto s2 = std::invoke([] {
            Scale s;
            get<0, 0>(s) = 2;
            get<1, 1>(s) = 3;
            get<2, 2>(s) = 4;
            return s;
        });
        constexpr auto s3 = s1 * s2;
        static_assert(get<0, 0>(s3) == 2);
        static_assert(get<1, 1>(s3) == 6);
        static_assert(get<2, 2>(s3) == 12);
    }
    static_assert(sizeof(Scale) == 3 * sizeof(float));
    static_assert(multiply(Scale::layout, Scale::layout) == Scale::layout);
    static_assert(multiply(Identity::layout, Scale::layout) == Scale::layout);
    static_assert(multiply(Scale::layout, Identity::layout) == Scale::layout);


    using Transformation =
        Matrix<float, layout({
                          //           Entry::zero  Entry::pone  Entry::mone  Entry::any
                          std::array { Entry::any,  Entry::any,  Entry::any,  Entry::any  }, // Entry::zero
                          std::array { Entry::any,  Entry::any,  Entry::any,  Entry::any  }, // Entry::pone
                          std::array { Entry::any,  Entry::any,  Entry::any,  Entry::any  }, // Entry::mone
                          std::array { Entry::zero, Entry::zero, Entry::zero, Entry::pone }, // Entry::any
    })>;
    testMat<Transformation>();
    Transformation transformation;
    static_assert(sizeof(Transformation) == 12 * sizeof(float));
    static_assert(multiply(Transformation::layout, Transformation::layout) == Transformation::layout);
    static_assert(multiply(Identity::layout, Transformation::layout) == Transformation::layout);
    static_assert(multiply(Transformation::layout, Identity::layout) == Transformation::layout);
    static_assert(multiply(multiply(Translation::layout, Rotation::layout), Scale::layout) == Transformation::layout);
    static_assert(multiply(multiply(Translation::layout, Rotation::layout), Identity::layout) ==
                  Transformation::layout);
    static_assert(multiply(Translation::layout, multiply(Scale::layout, Rotation::layout)) == Transformation::layout);

    static_assert(multiply(Translation::layout, multiply(Rotation::layout, Scale::layout)) == Transformation::layout);
    static_assert(multiply(Translation::layout, multiply(Rotation::layout, Identity::layout)) ==
                  Transformation::layout);
    std::exit(0);
};

}  // namespace surge::tiamat

int main() {
    try {
        surge::tiamat::test();
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

        // create scene
        const auto mainSceneId = engine.storage.createScene();
        auto&      mainScene   = engine.storage.scenes.at(mainSceneId);

        // load
        const auto skyboxAsset = engine.loader.loadAsset<surge::ShaderType::skybox>(
            engine.storage.createModel(surge::geom::cube),
            engine.storage.createSimpleMaterial(
                engine.storage.createTexture(surgeTextureFolder / "skybox.ktx", surge::Texture::cube)));
        const auto skybox = engine.createEntity(skyboxAsset);


        const auto coordinatesAsset = engine.loader.loadAsset<surge::ShaderType::coordinates>(
            engine.storage.createModel(surge::geom::coordinates));
        const auto coordinates = engine.createEntity(coordinatesAsset);

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
        const auto lightCubeAsset     = engine.loader.loadAsset<surge::ShaderType::primitiveTextured>(
            planeTexturedModel, engine.storage.defaultMaterialId);
        const auto lightCube = surge::createArray<surge::Entity, cubeFaces.size()>(
            [&]<int faceId>(auto& face) { face = engine.createEntity(lightCubeAsset); });

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

        const auto texturedCubeAssets = surge::createArray<surge::Asset, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face = engine.loader.loadAsset<surge::ShaderType::primitiveTextured>(planeTexturedModel,
                                                                                 cubeSimpleMaterials.at(faceId));
        });
        const auto texturedCube = surge::createArray<surge::Entity, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face = engine.createEntity(texturedCubeAssets.at(faceId), surge::translate<x>(2.0) * cubeFaces.at(faceId));
        });

        const auto planeTexturedNormalsModel = engine.storage.createModel(surge::geom::planeTexturedNormals);

        const auto texturedNormalCubeAssets =
            surge::createArray<surge::Asset, cubeFaces.size()>([&]<int faceId>(auto& face) {
                face = engine.loader.loadAsset<surge::ShaderType::primitiveTexturedNormal>(
                    planeTexturedNormalsModel, cubeSimpleMaterials.at(faceId));
            });
        const auto texturedNormalCube = surge::createArray<surge::Entity, cubeFaces.size()>(
            [&]<int faceId>(auto& face) { face = engine.createEntity(texturedNormalCubeAssets.at(faceId)); });

        const auto phongCubeAssets = surge::createArray<surge::Asset, cubeFaces.size()>([&]<int faceId>(auto& face) {
            face = engine.loader.loadAsset<surge::ShaderType::phongModel>(planeTexturedNormalsModel,
                                                                          cubePhongMaterials.at(faceId));
        });
        const auto phongCube       = surge::createArray<surge::Entity, cubeFaces.size()>(
            [&]<int faceId>(auto& face) { face = engine.createEntity(phongCubeAssets.at(faceId)); });

        const auto planeTexturedNormalTangentModel = engine.storage.createModel(surge::geom::planeNormalTangentTexture);

        const auto phongNormalCubeAssets =
            surge::createArray<surge::Asset, cubeFaces.size()>([&]<int faceId>(auto& face) {
                face = engine.loader.loadAsset<surge::ShaderType::phongModelNormal>(planeTexturedNormalTangentModel,
                                                                                    cubePhongMaterials.at(faceId));
            });
        const auto phongNormalCube = surge::createArray<surge::Entity, cubeFaces.size()>(
            [&]<int faceId>(auto& face) { face = engine.createEntity(phongNormalCubeAssets.at(faceId)); });

        const auto brickwallMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(surgeTextureFolder / "brickwall_diffuse.jpg", surge::Texture::texture2d),
            engine.storage.blackTextureId,
            engine.storage.createTexture(surgeTextureFolder / "brickwall_normal.jpg", surge::Texture::texture2dNorm));
        const auto brickwallAsset = engine.loader.loadAsset<surge::ShaderType::phongModelNormal>(
            planeTexturedNormalTangentModel, brickwallMaterial);

        std::vector<surge::Entity> brickwalls;
        for (const auto& translation : generateTranslations<10>()) {
            brickwalls.emplace_back(
                engine.createEntity(brickwallAsset, translation * surge::scale(4.0) * surge::rotate<x>(90)));
        }

        const auto crateMaterial = engine.storage.createPhongMaterial(
            engine.storage.createTexture(surgeTextureFolder / "container_diffuse.png", surge::Texture::texture2d),
            engine.storage.createTexture(surgeTextureFolder / "container_specular.png", surge::Texture::texture2d),
            engine.storage.whiteTextureId);
        const auto crateAsset = engine.loader.loadAsset<surge::ShaderType::phongModelNormal>(
            planeTexturedNormalTangentModel, crateMaterial);
        const auto crate = surge::createArray<surge::Entity, cubeFaces.size()>(
            [&]<int faceId>(auto& face) { face = engine.createEntity(crateAsset); });

        const auto floorAsset = engine.loader.loadAsset<surge::ShaderType::phongModelNormal>(
            planeTexturedNormalTangentModel, brickwallMaterial);
        const auto floor = engine.createEntity(floorAsset);

        // ===========================================================================================
        const auto dragonAsset = engine.loader.loadAsset<surge::ShaderType::primitiveNormal>(
            surge::GltfHandle { vulkanAssetFolder / "models/chinesedragon.gltf" });
        const auto dragon = engine.createEntity(dragonAsset);

        const std::filesystem::path cerberusFolder { vulkanAssetFolder / "models/cerberus" };
        using TextureType = surge::load::Gltf::TextureType;
        const std::map<TextureType, surge::TextureID> cerberusTextures {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(cerberusFolder / "albedo.ktx",   surge::Texture::texture2d) },
            { TextureType::metallicRoughnessTexture,
             engine.storage.createTexture(cerberusFolder / "metallic.ktx", surge::Texture::metallic)  },
            { TextureType::normalTexture,
             engine.storage.createTexture(cerberusFolder / "normal.ktx",   surge::Texture::texture2d) },
        };
        const auto cerberusAsset = engine.loader.loadAsset<surge::ShaderType::phongModelNormal>(
            surge::GltfHandle { cerberusFolder / "cerberus.gltf" }, cerberusTextures);
        const auto cerberus = engine.createEntity(cerberusAsset);

        const auto cesiumManAsset = engine.loader.loadAsset<surge::ShaderType::primitiveTexturedNormalAnimated>(
            surge::GltfHandle { vulkanAssetFolder / "models/CesiumMan/glTF-Embedded/CesiumMan.gltf" });
        const auto cesiumManAnimationChannel = engine.storage.createAnimationChannel(cesiumManAsset, 0);
        const auto cesiumMan                 = engine.createEntity(cesiumManAsset, cesiumManAnimationChannel);

        const auto buggyAsset = engine.loader.loadAsset<surge::ShaderType::primitiveNormal>(
            surge::GltfHandle { vulkanAssetFolder / "models/gltf/glTF-Embedded/Buggy.gltf" });
        const auto buggy = engine.createEntity(buggyAsset);

        const std::filesystem::path                   armorFolder { vulkanAssetFolder / "models/armor" };
        const std::map<TextureType, surge::TextureID> armorTextures1 {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(armorFolder / "colormap_rgba.ktx", surge::Texture::texture2d) },
            { TextureType::metallicRoughnessTexture, engine.storage.whiteTextureId },
            { TextureType::normalTexture,
             engine.storage.createTexture(armorFolder / "normalmap_rgba.ktx", surge::Texture::texture2d) },
        };
        const auto armorAsset1 = engine.loader.loadAsset<surge::ShaderType::phongModelNormal>(
            surge::GltfHandle { armorFolder / "armor.gltf" }, armorTextures1);
        const auto armor1 = engine.createEntity(armorAsset1);


        const std::map<TextureType, surge::TextureID> armorTextures2 {
            { TextureType::baseColorTexture,
             engine.storage.createTexture(armorFolder / "colormap_rgba.ktx", surge::Texture::texture2d) },
        };
        const auto armorAsset2 = engine.loader.loadAsset<surge::ShaderType::primitiveTexturedNormal>(
            surge::GltfHandle { armorFolder / "armor.gltf" }, armorTextures2);
        const auto armor2 = engine.createEntity(armorAsset2);

        const auto oaktreeAsset = engine.loader.loadAsset<surge::ShaderType::primitiveTexturedNormal>(
            surge::GltfHandle { vulkanAssetFolder / "models/oaktree.gltf" });
        const auto oaktree = engine.createEntity(oaktreeAsset);


        double elapsedTime = {};
        auto   start       = std::chrono::high_resolution_clock::now();
        while (engine.input.proceed) {
            if (elapsedTime > 1.0 / 60.0) {
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

                engine.update(cesiumManAnimationChannel, elapsedTime);

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
                // engine.update(cesiumMan, surge::translate<x>(-4.0) * rotationY);
                engine.update(cesiumMan, surge::translate<x>(-4.0));
                engine.update(buggy, surge::translate<x>(-6.0) * surge::scale(0.01) * rotationY);
                engine.update(armor1, surge::translate<x>(-8.0) * surge::scale(0.3) * rotationY);
                engine.update(armor2,
                              surge::translate<x>(-8.0) * surge::translate<z>(2.0) * surge::scale(0.3) * rotationY);
                engine.update(oaktree, surge::translate<x>(-10.0) * rotationY);
                // engine.update(pathfinder, surge::translate<z>(4.0) * rotationY);

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
                // mainScene.entities.push_back(pathfinder);


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
                // std::cout << "\rciao " << elapsedTime << " | ";
                // for (const auto x : engine.storage.animationChannels.at(0).jointMatrices.back()) {
                //     std::cout << x << " | ";
                // }
                // std::cout << std::flush;
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
