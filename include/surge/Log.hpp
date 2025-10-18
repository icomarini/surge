#pragma once

#include "surge/core/colors.hpp"

namespace surge::log
{
enum class Type
{
    urge,
    info,
    error,
};

enum class Font
{
    regular = 0,
    bold,
};

std::pair<std::string, core::Colors<core::Type::ansi>::Format> convert(const Type type)
{
    if (type == Type::urge)
    {
        return { "URGE", core::Colors<core::Type::ansi>::white };
    }
    else if (type == Type::info)
    {
        return { "INFO", core::Colors<core::Type::ansi>::green };
    }
    else if (type == Type::error)
    {
        return { "ERROR", core::Colors<core::Type::ansi>::red };
    }
}

std::string format(const uint8_t color, const Font font)
{
    return std::format("\033[{};{}m", static_cast<int>(font), color);
}

std::string format()
{
    return "\033[0m";
}

template<Type type>
void print(const std::string& line)
{
    const auto [tag, color] = convert(type);
    using Colors            = core::Colors<core::Type::ansi>;
    std::stringstream stream;
    stream << format(Colors::blue, Font::bold) << "[" << format(Colors::blue, Font::regular) << "surge of "
           << format(color, Font::bold) << tag << format(Colors::blue, Font::bold) << "] "
           << format(Colors::white, Font::regular) << line;
    const std::string string = stream.str();
    if constexpr (type == Type::error)
    {
        std::cerr << string << std::endl;
    }
    else
    {
        std::cout << string << std::endl;
    }
};

void urge(const std::string& line)
{
    print<Type::urge>(line);
}

void info(const std::string& line)
{
    print<Type::info>(line);
}

void error(const std::string& line)
{
    print<Type::error>(line);
}
}  // namespace surge::log