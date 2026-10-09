#pragma once

#include <vector>
#include <cstdint>

#include "../shared/frame.h"

class JpegDecoder
{
public:
    JpegDecoder();
    ~JpegDecoder();

    bool decode(
        const std::vector<uint8_t>& jpeg,
        Frame& frame);
};