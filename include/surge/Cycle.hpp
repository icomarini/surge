#pragma once

#include <vector>

namespace surge
{
template<typename T>
class Cycle
{
public:
    Cycle(const uint32_t size)
        : data { size }
        , currentIndex {}
    {
    }

    uint32_t size() const
    {
        return data.size();
    }

    void set(const uint32_t index, T&& value)
    {
        data.at(index) = value;
    }

    const T& current() const
    {
        return data.at(currentIndex);
    }

    void rotate()
    {
        currentIndex = (currentIndex + 1) % data.size();
    }

private:
    std::vector<T> data;
    uint32_t       currentIndex;
};

}  // namespace surge