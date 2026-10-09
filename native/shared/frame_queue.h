#pragma once

#include <mutex>

#include "frame.h"

class FrameQueue
{
public:
    static FrameQueue& instance();

    void setFrame(const Frame& frame);

    bool getFrame(Frame& frame);

private:
    FrameQueue() = default;

    std::mutex mutex_;

    Frame frame_;

    bool hasFrame_ = false;
};