#pragma once

#include <cstdint>

#ifdef _WIN32
#define LL_EXPORT __declspec(dllexport)
#else
#define LL_EXPORT
#endif

extern "C"
{
    LL_EXPORT bool LL_Initialize();

    LL_EXPORT void LL_Shutdown();

    LL_EXPORT bool LL_WriteFrame(
        const uint8_t* rgba,
        int size,
        int width,
        int height);

    LL_EXPORT bool LL_WriteJpeg(const uint8_t* jpeg, int size);
}