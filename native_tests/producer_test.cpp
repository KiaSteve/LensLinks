// producer_test.cpp
//
// Stands in for the real producer app: initializes LensLinks (which registers
// the virtual camera) and streams RGBA frames through LL_WriteFrame.
// Expected picture in a camera app: red | green | blue thirds with a white
// bar sweeping left to right. If red and blue are swapped, flip
// kInputIsRgba in shared_frame.cpp.

#include <windows.h>
#include <mfapi.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

extern "C"
{
    bool LL_Initialize();
    void LL_Shutdown();
    bool LL_WriteFrame(const uint8_t* rgba, int size, int width, int height);
}

int main()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);

    if (!LL_Initialize())
    {
        std::cout << "[ProducerTest] LL_Initialize failed" << std::endl;
        return 1;
    }

    const int W = 1280;
    const int H = 720;

    // Base picture, RGBA byte order: R,G,B,A.
    std::vector<uint8_t> base(static_cast<size_t>(W) * H * 4);

    for (int y = 0; y < H; ++y)
    {
        for (int x = 0; x < W; ++x)
        {
            uint8_t* p = &base[(static_cast<size_t>(y) * W + x) * 4];

            p[0] = (x < W / 3) ? 255 : 0;                    // R
            p[1] = (x >= W / 3 && x < 2 * W / 3) ? 255 : 0;  // G
            p[2] = (x >= 2 * W / 3) ? 255 : 0;               // B
            p[3] = 255;
        }
    }

    std::cout << "[ProducerTest] streaming. Open a camera app and select "
                 "LensLinks Camera. Press ENTER to stop." << std::endl;

    std::atomic<bool> running(true);

    std::thread worker([&]()
    {
        std::vector<uint8_t> frame(base.size());
        unsigned n = 0;

        while (running)
        {
            std::memcpy(frame.data(), base.data(), base.size());

            const int barX = static_cast<int>((n * 8u) % static_cast<unsigned>(W - 40));

            for (int y = 0; y < H; ++y)
            {
                std::memset(&frame[(static_cast<size_t>(y) * W + barX) * 4], 255, 40u * 4u);
            }

            LL_WriteFrame(frame.data(), static_cast<int>(frame.size()), W, H);

            ++n;
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    });

    std::cin.get();

    running = false;
    worker.join();

    LL_Shutdown();
    MFShutdown();
    CoUninitialize();

    return 0;
}
