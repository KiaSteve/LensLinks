#pragma once

#include <cstdint>
#include <vector>

struct Frame
{
    std::vector<uint8_t> pixels;

    int width = 0;

    int height = 0;

    int stride = 0;
};