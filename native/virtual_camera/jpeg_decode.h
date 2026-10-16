#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Rotation applied to every decoded frame before it is fitted into the
// camera frame. Accepts 0, 90, 180 or 270 degrees clockwise; any other
// value is rounded down to a multiple of 90 and wrapped.
void SetJpegRotationDegrees(int degrees);

// Decodes a JPEG with WIC, rotates it (see above), scales it to fit inside
// outW x outH keeping its aspect ratio, centers it on a black canvas, and
// returns RGBA bytes (R,G,B,A order, alpha 255) of exactly
// outW * outH * 4 bytes.
bool DecodeJpegLetterboxRgba(
    const uint8_t* jpeg,
    size_t size,
    int outW,
    int outH,
    std::vector<uint8_t>& rgba);
