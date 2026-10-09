#pragma once

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfvirtualcamera.h>

class VirtualCameraRegistration
{
public:
    VirtualCameraRegistration();
    ~VirtualCameraRegistration();

    bool start();
    void stop();

    bool isStarted() const;

private:
    IMFVirtualCamera* camera_ = nullptr;
    bool started_ = false;
};