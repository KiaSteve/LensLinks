#include <windows.h>
#include <mfapi.h>
#include <iostream>

#include "../native/mf_camera/virtual_camera_registration.h"

int main()
{
    std::cout << "[VirtualCameraTest] Starting..." << std::endl;

    HRESULT hr = CoInitializeEx(
        nullptr,
        COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        std::cerr
            << "[VirtualCameraTest] CoInitializeEx failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return 1;
    }

    std::cout
        << "[VirtualCameraTest] COM initialized"
        << std::endl;

    hr = MFStartup(MF_VERSION);

    if (FAILED(hr))
    {
        std::cerr
            << "[VirtualCameraTest] MFStartup failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        CoUninitialize();
        return 1;
    }

    std::cout
        << "[VirtualCameraTest] Media Foundation initialized"
        << std::endl;

    {
        VirtualCameraRegistration registration;

        std::cout
            << "[VirtualCameraTest] Calling start()..."
            << std::endl;

        bool started = registration.start();

        if (!started)
        {
            std::cerr
                << "[VirtualCameraTest] FAILED to start virtual camera"
                << std::endl;

            MFShutdown();
            CoUninitialize();

            return 2;
        }

        std::cout
            << "[VirtualCameraTest] =================================="
            << std::endl;

        std::cout
            << "[VirtualCameraTest] LensLinks virtual camera is RUNNING"
            << std::endl;

        std::cout
            << "[VirtualCameraTest] Keep this window open."
            << std::endl;

        std::cout
            << "[VirtualCameraTest] Check Windows camera devices now."
            << std::endl;

        std::cout
            << "[VirtualCameraTest] Press ENTER to stop."
            << std::endl;

        std::cin.get();
    }

    MFShutdown();
    CoUninitialize();

    std::cout
        << "[VirtualCameraTest] Finished"
        << std::endl;

    return 0;
}