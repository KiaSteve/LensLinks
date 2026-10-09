#include "frame_provider.h"

#include "../virtual_camera/virtual_camera.h"

#include <chrono>
#include <iostream>

FrameProvider::FrameProvider()
{
}

FrameProvider::~FrameProvider()
{
    stop();
}

bool FrameProvider::start()
{
    if (running_)
        return true;

    running_ = true;

    thread_ = std::thread(
        &FrameProvider::worker,
        this);

    std::cout
        << "FrameProvider started"
        << std::endl;

    return true;
}

void FrameProvider::stop()
{
    if (!running_)
        return;

    running_ = false;

    if (thread_.joinable())
        thread_.join();

    std::cout
        << "FrameProvider stopped"
        << std::endl;
}

void FrameProvider::worker()
{
    while (running_)
    {
        CameraFrame frame;

        if (VirtualCamera::instance().getFrame(frame))
        {
            std::cout
                << "[FrameProvider] "
                << frame.width
                << "x"
                << frame.height
                << "  "
                << frame.pixels.size()
                << " bytes"
                << std::endl;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(33));
    }
}
