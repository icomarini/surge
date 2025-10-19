#pragma once

#include "surge/core/colors.hpp"

namespace surge::log
{
enum class Type
{
    checkpoint,
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
    switch (type)
    {
    case Type::checkpoint:
        return { "URGE", core::Colors<core::Type::ansi>::white };
    case Type::info:
        return { "INFO", core::Colors<core::Type::ansi>::green };
    case Type::error:
        return { "PURGE", core::Colors<core::Type::ansi>::red };
    default:
        throw;
    }
}

std::string format(const uint8_t color, const Font font)
{
    return std::format("\033[{};{}m", static_cast<int>(font), color);
}

std::string format(const Font font)
{
    return std::format("\033[{};m", static_cast<int>(font));
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
    stream << format(Colors::white, Font::bold) << "[" << format(Colors::white, Font::regular) << "surge of "
           << format(color, Font::bold) << tag << format(Colors::white, Font::bold) << "] " << format() << line;
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

void checkpoint(const std::string& line)
{
    print<Type::checkpoint>(line);
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