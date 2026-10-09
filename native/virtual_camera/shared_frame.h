#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Cross-process frame transport between the producer process
// (LL_WriteFrame) and Frame Server (RequestSample).

// Producer side. Input is RGBA (see kInputIsRgba in shared_frame.cpp);
// the stored frame is B,G,R,X to match the camera's RGB32 media type.
bool SharedFrame_Publish(
    const uint8_t* rgba,
    size_t size,
    int width,
    int height);

// Frame Server side. Returns false when no producer is writing, or when the
// newest frame is older than maxAgeMs.
bool SharedFrame_Read(
    std::vector<uint8_t>& bgra,
    int& width,
    int& height,
    unsigned maxAgeMs);
