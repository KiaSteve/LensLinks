#pragma once

#include <atomic>
#include <thread>

class FrameProvider
{
public:
    FrameProvider();

    ~FrameProvider();

    bool start();

    void stop();

private:
    void worker();

    std::atomic<bool> running_{false};

    std::thread thread_;
};