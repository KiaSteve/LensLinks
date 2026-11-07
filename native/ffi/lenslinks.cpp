#include "lenslinks.h"

#include <iostream>

#include "../mf_camera/frame_provider.h"
#include "../mf_camera/virtual_camera_registration.h"
#include "../virtual_camera/virtual_camera.h"
#include "../virtual_camera/jpeg_decode.h"
#include "../virtual_camera/recorder.h"

#include <vector>

static FrameProvider g_frameProvider;
static VirtualCameraRegistration g_cameraRegistration;

LL_EXPORT bool LL_Initialize()
{
    std::cout << "[LensLinks] Initializing..." << std::endl;

    if (!VirtualCamera::instance().start(1280, 720))
    {
        std::cerr << "[LensLinks] VirtualCamera start failed" << std::endl;
        return false;
    }

    if (!g_frameProvider.start())
    {
        std::cerr << "[LensLinks] FrameProvider start failed" << std::endl;

        VirtualCamera::instance().stop();
        return false;
    }

    if (!g_cameraRegistration.start())
    {
        std::cerr
            << "[LensLinks] Media Foundation virtual camera registration failed"
            << std::endl;

        g_frameProvider.stop();
        VirtualCamera::instance().stop();

        return false;
    }

    std::cout
        << "[LensLinks] Initialization complete"
        << std::endl;

    return true;
}

LL_EXPORT void LL_Shutdown()
{
    std::cout
        << "[LensLinks] Shutting down..."
        << std::endl;

    Recorder_Stop();

    g_cameraRegistration.stop();

    g_frameProvider.stop();

    VirtualCamera::instance().stop();

    std::cout
        << "[LensLinks] Shutdown complete"
        << std::endl;
}

LL_EXPORT bool LL_WriteFrame(
    const uint8_t* rgba,
    int size,
    int width,
    int height)
{
    if (rgba != nullptr &&
        width > 0 &&
        height > 0 &&
        static_cast<size_t>(size) ==
            static_cast<size_t>(width) * static_cast<size_t>(height) * 4u)
    {
        Recorder_Submit(rgba, width, height);
    }

    return VirtualCamera::instance().sendFrame(
        rgba,
        size,
        width,
        height);
}

LL_EXPORT bool LL_WriteJpeg(
    const uint8_t* jpeg,
    int size)
{
    if (jpeg == nullptr || size <= 0)
    {
        return false;
    }

    // The camera is fixed at 1280x720; the frame is fitted inside it
    // with black bars, so portrait and landscape both display undistorted.
    static thread_local std::vector<uint8_t> rgba;

    if (!DecodeJpegLetterboxRgba(
            jpeg,
            static_cast<size_t>(size),
            1280,
            720,
            rgba))
    {
        return false;
    }

    Recorder_Submit(rgba.data(), 1280, 720);

    return VirtualCamera::instance().sendFrame(
        rgba.data(),
        static_cast<int>(rgba.size()),
        1280,
        720);
}

LL_EXPORT void LL_SetRotation(int degrees)
{
    SetJpegRotationDegrees(degrees);
}

LL_EXPORT bool LL_StartRecording(const wchar_t* path)
{
    return Recorder_Start(path, 1280, 720);
}

LL_EXPORT bool LL_StopRecording()
{
    return Recorder_Stop();
}

LL_EXPORT bool LL_IsRecording()
{
    return Recorder_IsRecording();
}
