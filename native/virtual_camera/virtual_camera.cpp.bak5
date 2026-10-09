#include "virtual_camera.h"

#include <cstring>
#include <iostream>
#include <limits>

VirtualCamera& VirtualCamera::instance()
{
    static VirtualCamera camera;
    return camera;
}

bool VirtualCamera::start(
    int width,
    int height)
{
    if (width <= 0 || height <= 0)
    {
        std::cout
            << "[VirtualCamera] invalid start dimensions "
            << width
            << "x"
            << height
            << std::endl;

        return false;
    }

    const size_t expectedSize =
        static_cast<size_t>(width) *
        static_cast<size_t>(height) *
        4u;

    if (expectedSize >
        static_cast<size_t>(
            std::numeric_limits<int>::max()))
    {
        std::cout
            << "[VirtualCamera] frame size too large"
            << std::endl;

        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    frame_.pixels.clear();

    frame_.width = width;
    frame_.height = height;

    std::cout
        << "[VirtualCamera] started "
        << width
        << "x"
        << height
        << " RGB32"
        << std::endl;

    return true;
}

bool VirtualCamera::sendFrame(
    const uint8_t* rgba,
    int size,
    int width,
    int height)
{
    if (rgba == nullptr)
    {
        std::cout
            << "[VirtualCamera] sendFrame rejected: "
            << "null pixel buffer"
            << std::endl;

        return false;
    }

    if (size <= 0)
    {
        std::cout
            << "[VirtualCamera] sendFrame rejected: "
            << "invalid size="
            << size
            << std::endl;

        return false;
    }

    if (width <= 0 || height <= 0)
    {
        std::cout
            << "[VirtualCamera] sendFrame rejected: "
            << "invalid dimensions "
            << width
            << "x"
            << height
            << std::endl;

        return false;
    }

    const size_t expectedSize =
        static_cast<size_t>(width) *
        static_cast<size_t>(height) *
        4u;

    if (expectedSize !=
        static_cast<size_t>(size))
    {
        std::cout
            << "[VirtualCamera] sendFrame rejected: "
            << "expected "
            << expectedSize
            << " bytes, received "
            << size
            << std::endl;

        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // ------------------------------------------------------------
    // If start() established a camera format, incoming frames
    // must match it.
    // ------------------------------------------------------------

    if (frame_.width != 0 &&
        frame_.height != 0)
    {
        if (width != frame_.width ||
            height != frame_.height)
        {
            std::cout
                << "[VirtualCamera] sendFrame rejected: "
                << "format mismatch. Camera="
                << frame_.width
                << "x"
                << frame_.height
                << " incoming="
                << width
                << "x"
                << height
                << std::endl;

            return false;
        }
    }

    frame_.pixels.resize(
        static_cast<size_t>(size));

    std::memcpy(
        frame_.pixels.data(),
        rgba,
        static_cast<size_t>(size));

    frame_.width = width;
    frame_.height = height;

    return true;
}

bool VirtualCamera::getFrame(
    CameraFrame& frame)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (frame_.pixels.empty())
    {
        return false;
    }

    if (frame_.width <= 0 ||
        frame_.height <= 0)
    {
        return false;
    }

    frame = frame_;

    return true;
}

void VirtualCamera::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);

    frame_.pixels.clear();

    frame_.width = 0;
    frame_.height = 0;

    std::cout
        << "[VirtualCamera] stopped"
        << std::endl;
}