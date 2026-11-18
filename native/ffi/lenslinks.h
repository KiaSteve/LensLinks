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

    LL_EXPORT void LL_SetRotation(int degrees);

    // Recording to an MP4 file. The path is UTF-16.
    LL_EXPORT bool LL_StartRecording(const wchar_t* path);

    LL_EXPORT bool LL_StopRecording();

    LL_EXPORT bool LL_IsRecording();

    // 16-bit little-endian mono PCM at 44100 Hz; used only while recording.
    LL_EXPORT void LL_WriteAudio(const uint8_t* pcm, int size);
}
