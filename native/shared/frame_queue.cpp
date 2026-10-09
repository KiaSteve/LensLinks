#include "frame_queue.h"

FrameQueue& FrameQueue::instance()
{
    static FrameQueue queue;
    return queue;
}

void FrameQueue::setFrame(const Frame& frame)
{
    std::lock_guard<std::mutex> lock(mutex_);

    frame_ = frame;

    hasFrame_ = true;
}

bool FrameQueue::getFrame(Frame& frame)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!hasFrame_)
        return false;

    frame = frame_;

    return true;
}