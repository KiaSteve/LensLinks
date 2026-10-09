#include "virtual_camera_registration.h"

#include <iostream>

VirtualCameraRegistration::VirtualCameraRegistration()
{
}

VirtualCameraRegistration::~VirtualCameraRegistration()
{
    stop();
}

bool VirtualCameraRegistration::start()
{
    if (started_)
        return true;

    // Media Foundation is already initialized by the application/test.
    // Do NOT call MFStartup() here.

    const wchar_t* sourceId =
        L"{8F8B9C41-4A6D-4C7B-912A-7E529D138461}";

    std::cout
        << "[VirtualCamera] Creating virtual camera..."
        << std::endl;

    HRESULT hr = MFCreateVirtualCamera(
        MFVirtualCameraType_SoftwareCameraSource,
        MFVirtualCameraLifetime_Session,
        MFVirtualCameraAccess_CurrentUser,
        L"LensLinks Camera",
        sourceId,
        nullptr,
        0,
        &camera_);

    if (FAILED(hr))
    {
        std::cerr
            << "[VirtualCamera] MFCreateVirtualCamera failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        camera_ = nullptr;
        return false;
    }

    std::cout
        << "[VirtualCamera] Starting IMFVirtualCamera..."
        << std::endl;

    hr = camera_->Start(nullptr);

    if (FAILED(hr))
    {
        std::cerr
            << "[VirtualCamera] IMFVirtualCamera::Start failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        camera_->Shutdown();
        camera_->Release();
        camera_ = nullptr;

        return false;
    }

    started_ = true;

    std::cout
        << "[VirtualCamera] LensLinks virtual camera started successfully."
        << std::endl;

    return true;
}

void VirtualCameraRegistration::stop()
{
    if (!started_)
        return;

    std::cout
        << "[VirtualCamera] Stopping virtual camera..."
        << std::endl;

    if (camera_ != nullptr)
    {
        camera_->Stop();
        camera_->Shutdown();
        camera_->Release();
        camera_ = nullptr;
    }

    started_ = false;

    // Do NOT call MFShutdown() here.
    // Whoever called MFStartup() is responsible for shutting it down.

    std::cout
        << "[VirtualCamera] LensLinks virtual camera stopped."
        << std::endl;
}

bool VirtualCameraRegistration::isStarted() const
{
    return started_;
}