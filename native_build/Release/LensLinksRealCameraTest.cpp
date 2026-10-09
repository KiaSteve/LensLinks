#include <windows.h>

#include <cstdint>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

using LL_InitializeFn = bool (*)();

using LL_ShutdownFn = void (*)();

using LL_WriteFrameFn = bool (*)(
    const uint8_t* rgba,
    int size,
    int width,
    int height);

static void PrintStatus(
    const char* label,
    bool success)
{
    std::cout
        << label
        << " = "
        << (success ? "SUCCESS" : "FAILED")
        << std::endl;
}

int main()
{
    std::cout
        << "========================================\n"
        << " LensLinks REAL Camera Test\n"
        << "========================================\n";

    constexpr int WIDTH = 1280;
    constexpr int HEIGHT = 720;

    const size_t bufferSize =
        static_cast<size_t>(WIDTH) *
        static_cast<size_t>(HEIGHT) *
        4u;

    std::cout
        << "\n[1] Load LensLinksNative.dll\n";

    HMODULE dll =
        LoadLibraryW(
            L"LensLinksNative.dll");

    if (dll == nullptr)
    {
        std::cout
            << "LoadLibrary failed. Error = "
            << GetLastError()
            << std::endl;

        return 1;
    }

    std::cout
        << "LensLinksNative.dll loaded."
        << std::endl;

    auto LL_Initialize =
        reinterpret_cast<LL_InitializeFn>(
            GetProcAddress(
                dll,
                "LL_Initialize"));

    auto LL_Shutdown =
        reinterpret_cast<LL_ShutdownFn>(
            GetProcAddress(
                dll,
                "LL_Shutdown"));

    auto LL_WriteFrame =
        reinterpret_cast<LL_WriteFrameFn>(
            GetProcAddress(
                dll,
                "LL_WriteFrame"));

    if (LL_Initialize == nullptr)
    {
        std::cout
            << "LL_Initialize export NOT found."
            << std::endl;

        FreeLibrary(dll);
        return 2;
    }

    if (LL_Shutdown == nullptr)
    {
        std::cout
            << "LL_Shutdown export NOT found."
            << std::endl;

        FreeLibrary(dll);
        return 3;
    }

    if (LL_WriteFrame == nullptr)
    {
        std::cout
            << "LL_WriteFrame export NOT found."
            << std::endl;

        FreeLibrary(dll);
        return 4;
    }

    std::cout
        << "All LensLinks exports found."
        << std::endl;

    std::cout
        << "\n[2] LL_Initialize()\n";

    bool initialized =
        LL_Initialize();

    PrintStatus(
        "LL_Initialize",
        initialized);

    if (!initialized)
    {
        std::cout
            << "LensLinks initialization failed."
            << std::endl;

        FreeLibrary(dll);
        return 5;
    }

    std::cout
        << "\nLensLinks virtual camera should now be registered."
        << std::endl;

    std::cout
        << "Camera name: LensLinks Camera"
        << std::endl;

    std::cout
        << "\n[3] Create test frame\n";

    std::vector<uint8_t> frame(
        bufferSize);

    /*
     * Generate a simple moving test pattern.
     *
     * This is deliberately obvious when viewed through
     * an actual camera application.
     */

    std::cout
        << "Frame size = "
        << WIDTH
        << "x"
        << HEIGHT
        << " RGB32"
        << std::endl;

    std::cout
        << "Buffer size = "
        << frame.size()
        << " bytes"
        << std::endl;

    std::cout
        << "\n[4] Writing frames\n";
    std::cout
        << "The test will continue running."
        << std::endl;

    std::cout
        << "\n========================================\n"
        << " IMPORTANT\n"
        << "========================================\n"
        << "Open Windows Camera now.\n"
        << "Look for: LensLinks Camera\n"
        << "\n"
        << "Press Ctrl+C in this window to stop.\n"
        << "========================================\n"
        << std::endl;

    int frameNumber = 0;

    while (true)
    {
        /*
         * Create a moving color pattern.
         *
         * RGB32 memory layout here is treated as:
         * B G R A
         */

        for (int y = 0; y < HEIGHT; ++y)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                const size_t index =
                    (
                        static_cast<size_t>(y) *
                        static_cast<size_t>(WIDTH) +
                        static_cast<size_t>(x)
                    ) * 4u;

                const uint8_t r =
                    static_cast<uint8_t>(
                        (x + frameNumber) & 0xFF);

                const uint8_t g =
                    static_cast<uint8_t>(
                        (y + frameNumber) & 0xFF);

                const uint8_t b =
                    static_cast<uint8_t>(
                        (x + y + frameNumber) & 0xFF);

                frame[index + 0] = b;
                frame[index + 1] = g;
                frame[index + 2] = r;
                frame[index + 3] = 255;
            }
        }

        bool written =
            LL_WriteFrame(
                frame.data(),
                static_cast<int>(
                    frame.size()),
                WIDTH,
                HEIGHT);

        if (!written)
        {
            std::cout
                << "[Frame "
                << frameNumber
                << "] LL_WriteFrame FAILED"
                << std::endl;
        }
        else if (
            frameNumber % 30 == 0)
        {
            std::cout
                << "[Frame "
                << frameNumber
                << "] LL_WriteFrame SUCCESS"
                << std::endl;
        }

        ++frameNumber;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(33));
    }

    /*
     * Normally unreachable because the loop is infinite.
     */

    LL_Shutdown();

    FreeLibrary(dll);

    return 0;
}