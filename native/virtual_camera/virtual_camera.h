#pragma once

#include <cstdint>
#include <mutex>
#include <vector>

struct CameraFrame
{
    std::vector<uint8_t> pixels;

    int width = 0;
    int height = 0;

    bool empty() const
    {
        return pixels.empty();
    }
};

class VirtualCamera
{
public:
    static VirtualCamera& instance();

    bool start(int width, int height);

    bool sendFrame(
        const uint8_t* rgba,
        int size,
        int width,
        int height);

    bool getFrame(CameraFrame& frame);

    void stop();

private:
    VirtualCamera() = default;

    std::mutex mutex_;

    CameraFrame frame_;
};