#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Decodes a JPEG with WIC, scales it to fit inside outW x outH keeping its
// aspect ratio, centers it on a black canvas, and returns RGBA bytes
// (R,G,B,A order, alpha 255) of exactly outW * outH * 4 bytes.
bool DecodeJpegLetterboxRgba(
    const uint8_t* jpeg,
    size_t size,
    int outW,
    int outH,
    std::vector<uint8_t>& rgba);
