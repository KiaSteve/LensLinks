#include "lenslinks.h"

#include <iostream>

#include "../mf_camera/frame_provider.h"
#include "../mf_camera/virtual_camera_registration.h"
#include "../virtual_camera/virtual_camera.h"

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
    return VirtualCamera::instance().sendFrame(
        rgba,
        size,
        width,
        height);
}
