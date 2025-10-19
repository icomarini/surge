#pragma once

#include <imgui.h>

namespace surge::overlay
{

class Font
{
public:
    Font()
        : width {}
        , height {}
        , mipLevels { 1 }
        , arrayLayers { 1 }
        , name { "fonts" }
    {
        auto& io       = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;

        assert(io.Fonts->AddFontFromFileTTF("/home/ico/projects/extern/imgui/misc/fonts/ProggyClean.ttf", 20));

        unsigned char* data {};
        int            texWidth {}, texHeight {}, bytesPerPixel {};
        io.Fonts->GetTexDataAsRGBA32(&data, &texWidth, &texHeight, &bytesPerPixel);

        width           = static_cast<uint32_t>(texWidth);
        height          = static_cast<uint32_t>(texHeight);
        const auto size = bytesPerPixel * texWidth * texHeight;
        fonts.resize(size);
        fonts.assign(data, data + size);
    }

    const void* data() const
    {
        return static_cast<const void*>(fonts.data());
    }

    uint64_t memorySize() const
    {
        return width * height * 4 * sizeof(char);
    }

    std::vector<std::tuple<uint32_t, uint32_t, uint64_t>> offsets() const
    {
        return { { 0, 0, 0 } };
    }

    uint32_t    width;
    uint32_t    height;
    uint32_t    mipLevels;
    uint32_t    arrayLayers;
    std::string name;

private:
    std::vector<unsigned char> fonts;
};

}  // namespace surge::overlay
