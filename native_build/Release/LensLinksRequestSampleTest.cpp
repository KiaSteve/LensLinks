#include <windows.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>

#include <iostream>
#include <vector>

#include "..\..\native\mf_camera\lenslinks_media_source.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

// ============================================================
// LensLinks Media Source CLSID
// ============================================================

const CLSID CLSID_LensLinksMediaSource =
{
    0x8F8B9C41,
    0x4A6D,
    0x4C7B,
    {0x91, 0x2A, 0x7E, 0x52, 0x9D, 0x13, 0x84, 0x61}
};

// ============================================================
// LL_WriteFrame function type
// ============================================================

using LL_WriteFrameFn =
    bool (*)(const uint8_t*, int, int, int);

// ============================================================
// HRESULT helper
// ============================================================

static void PrintHR(
    const char* label,
    HRESULT hr)
{
    std::cout
        << label
        << " = 0x"
        << std::hex
        << std::uppercase
        << static_cast<unsigned long>(hr)
        << std::dec
        << std::endl;
}

// ============================================================
// Main
// ============================================================

int main()
{
    std::cout
        << "========================================\n"
        << " LensLinks RequestSample Test\n"
        << "========================================\n";

    // ========================================================
    // COM initialization
    // ========================================================

    HRESULT hr =
        CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

    PrintHR(
        "CoInitializeEx",
        hr);

    if (FAILED(hr))
    {
        return 1;
    }

    // ========================================================
    // Media Foundation startup
    // ========================================================

    hr =
        MFStartup(
            MF_VERSION,
            MFSTARTUP_FULL);

    PrintHR(
        "MFStartup",
        hr);

    if (FAILED(hr))
    {
        CoUninitialize();
        return 2;
    }

    // ========================================================
    // [0] Load LensLinksNative.dll
    //
    // We use LL_WriteFrame directly to put a known test frame
    // into VirtualCamera before RequestSample() is called.
    //
    // We intentionally do NOT call LL_Initialize() here.
    // ========================================================

    std::cout
        << "\n[0] Load LensLinksNative.dll\n";

    HMODULE nativeDll =
        GetModuleHandleW(
            L"LensLinksNative.dll");

    if (nativeDll == nullptr)
    {
        nativeDll =
            LoadLibraryW(
                L"C:\\Users\\Administrator\\lenslinks\\native_build\\Release\\LensLinksNative.dll");
    }

    if (nativeDll == nullptr)
    {
        std::cout
            << "Failed to load LensLinksNative.dll"
            << std::endl;

        MFShutdown();
        CoUninitialize();

        return 10;
    }

    std::cout
        << "LensLinksNative.dll loaded."
        << std::endl;

    // ========================================================
    // Locate LL_WriteFrame
    // ========================================================

    auto LL_WriteFrame =
        reinterpret_cast<LL_WriteFrameFn>(
            GetProcAddress(
                nativeDll,
                "LL_WriteFrame"));

    if (LL_WriteFrame == nullptr)
    {
        std::cout
            << "Failed to find LL_WriteFrame export."
            << std::endl;

        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 11;
    }

    std::cout
        << "LL_WriteFrame export found."
        << std::endl;

    // ========================================================
    // Create test RGB32 frame
    // ========================================================

    constexpr int TEST_WIDTH = 1280;
    constexpr int TEST_HEIGHT = 720;
    constexpr int BYTES_PER_PIXEL = 4;

    const int frameSize =
        TEST_WIDTH *
        TEST_HEIGHT *
        BYTES_PER_PIXEL;

    std::vector<uint8_t> testFrame(
        static_cast<size_t>(frameSize));

    // ========================================================
    // Generate a visible test pattern.
    //
    // RGB32 byte layout:
    //
    //   byte 0 = Blue
    //   byte 1 = Green
    //   byte 2 = Red
    //   byte 3 = Alpha
    // ========================================================

    for (int y = 0;
         y < TEST_HEIGHT;
         ++y)
    {
        for (int x = 0;
             x < TEST_WIDTH;
             ++x)
        {
            const size_t offset =
                static_cast<size_t>(
                    (y * TEST_WIDTH + x) * 4);

            testFrame[offset + 0] =
                static_cast<uint8_t>(
                    x % 256);

            testFrame[offset + 1] =
                static_cast<uint8_t>(
                    y % 256);

            testFrame[offset + 2] =
                180;

            testFrame[offset + 3] =
                255;
        }
    }

    std::cout
        << "Writing test frame "
        << TEST_WIDTH
        << "x"
        << TEST_HEIGHT
        << " RGB32 ("
        << frameSize
        << " bytes)"
        << std::endl;

    // ========================================================
    // Inject frame into VirtualCamera
    // ========================================================

    bool frameWritten =
        LL_WriteFrame(
            testFrame.data(),
            frameSize,
            TEST_WIDTH,
            TEST_HEIGHT);

    std::cout
        << "LL_WriteFrame = "
        << (frameWritten
                ? "SUCCESS"
                : "FAILED")
        << std::endl;

    if (!frameWritten)
    {
        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 12;
    }

    // ========================================================
    // [1] CoCreateInstance -> IMFActivate
    // ========================================================

    IMFActivate* activate =
        nullptr;

    std::cout
        << "\n[1] CoCreateInstance -> IMFActivate\n";

    hr =
        CoCreateInstance(
            CLSID_LensLinksMediaSource,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IMFActivate,
            reinterpret_cast<void**>(
                &activate));

    PrintHR(
        "CoCreateInstance(IMFActivate)",
        hr);

    if (FAILED(hr))
    {
        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 3;
    }

    // ========================================================
    // [2] ActivateObject -> IMFMediaSource
    // ========================================================

    IMFMediaSource* source =
        nullptr;

    std::cout
        << "\n[2] ActivateObject -> IMFMediaSource\n";

    hr =
        activate->ActivateObject(
            IID_IMFMediaSource,
            reinterpret_cast<void**>(
                &source));

    PrintHR(
        "ActivateObject(IMFMediaSource)",
        hr);

    if (FAILED(hr))
    {
        activate->Release();

        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 4;
    }

    // ========================================================
    // [3] QueryInterface -> private test interface
    // ========================================================

    std::cout
        << "\n[3] QueryInterface -> "
        << "ILensLinksMediaSourceTest\n";

    ILensLinksMediaSourceTest*
        testInterface = nullptr;

    hr =
        source->QueryInterface(
            __uuidof(
                ILensLinksMediaSourceTest),
            reinterpret_cast<void**>(
                &testInterface));

    PrintHR(
        "QueryInterface(ILensLinksMediaSourceTest)",
        hr);

    if (FAILED(hr))
    {
        std::cout
            << "Private test interface unavailable."
            << std::endl;

        source->Release();
        activate->Release();

        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 5;
    }

    // ========================================================
    // [4] Get actual IMFMediaStream
    // ========================================================

    IMFMediaStream* stream =
        nullptr;

    std::cout
        << "\n[4] GetLensLinksStream\n";

    hr =
        testInterface->GetLensLinksStream(
            &stream);

    PrintHR(
        "GetLensLinksStream",
        hr);

    if (FAILED(hr) ||
        stream == nullptr)
    {
        std::cout
            << "Failed to obtain IMFMediaStream."
            << std::endl;

        testInterface->Release();
        source->Release();
        activate->Release();

        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 6;
    }

    std::cout
        << "IMFMediaStream obtained successfully."
        << std::endl;

    // ========================================================
    // [5] CreatePresentationDescriptor
    // ========================================================

    IMFPresentationDescriptor*
        descriptor = nullptr;

    std::cout
        << "\n[5] CreatePresentationDescriptor\n";

    hr =
        source->CreatePresentationDescriptor(
            &descriptor);

    PrintHR(
        "CreatePresentationDescriptor",
        hr);

    if (FAILED(hr))
    {
        stream->Release();
        testInterface->Release();
        source->Release();
        activate->Release();

        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 7;
    }

    // ========================================================
    // [6] Start source
    // ========================================================

    std::cout
        << "\n[6] Start source\n";

    hr =
        source->Start(
            descriptor,
            nullptr,
            nullptr);

    PrintHR(
        "IMFMediaSource::Start",
        hr);

    if (FAILED(hr))
    {
        descriptor->Release();
        stream->Release();
        testInterface->Release();
        source->Release();
        activate->Release();

        FreeLibrary(nativeDll);

        MFShutdown();
        CoUninitialize();

        return 8;
    }

    // ========================================================
    // [7] RequestSample
    // ========================================================

    std::cout
        << "\n[7] RequestSample(nullptr)\n";

    hr =
        stream->RequestSample(
            nullptr);

    PrintHR(
        "IMFMediaStream::RequestSample",
        hr);

    if (SUCCEEDED(hr))
    {
        std::cout
            << "RequestSample SUCCESS."
            << std::endl;
    }
    else
    {
        std::cout
            << "RequestSample FAILED."
            << std::endl;
    }

    // ========================================================
    // [8] Read stream event
    // ========================================================

    std::cout
        << "\n[8] Read stream event\n";

    IMFMediaEvent* event =
        nullptr;

    hr =
        stream->GetEvent(
            0,
            &event);

    PrintHR(
        "IMFMediaStream::GetEvent",
        hr);

    if (SUCCEEDED(hr) &&
        event != nullptr)
    {
        MediaEventType eventType =
            MEUnknown;

        HRESULT eventStatus =
            S_OK;

        // ----------------------------------------------------
        // Event type
        // ----------------------------------------------------

        hr =
            event->GetType(
                &eventType);

        PrintHR(
            "GetEventType",
            hr);

        // ----------------------------------------------------
        // Event status
        // ----------------------------------------------------

        hr =
            event->GetStatus(
                &eventStatus);

        PrintHR(
            "GetEventStatus",
            hr);

        std::cout
            << "Event type = "
            << static_cast<int>(
                eventType)
            << std::endl;

        // ----------------------------------------------------
        // We expect MEMediaSample.
        // ----------------------------------------------------

        if (eventType == MEMediaSample)
        {
            std::cout
                << "MEMediaSample received."
                << std::endl;

            PROPVARIANT value;

            PropVariantInit(
                &value);

            hr =
                event->GetValue(
                    &value);

            PrintHR(
                "GetValue",
                hr);

            // ------------------------------------------------
            // The event should contain the IMFSample as
            // VT_UNKNOWN.
            // ------------------------------------------------

            if (SUCCEEDED(hr) &&
                value.vt == VT_UNKNOWN &&
                value.punkVal != nullptr)
            {
                IMFSample* sample =
                    nullptr;

                hr =
                    value.punkVal->QueryInterface(
                        IID_IMFSample,
                        reinterpret_cast<void**>(
                            &sample));

                PrintHR(
                    "QueryInterface(IMFSample)",
                    hr);

                if (SUCCEEDED(hr) &&
                    sample != nullptr)
                {
                    // ----------------------------------------
                    // Buffer count
                    // ----------------------------------------

                    DWORD bufferCount = 0;

                    hr =
                        sample->GetBufferCount(
                            &bufferCount);

                    PrintHR(
                        "IMFSample::GetBufferCount",
                        hr);

                    std::cout
                        << "Sample buffer count = "
                        << bufferCount
                        << std::endl;

                    // ----------------------------------------
                    // Sample time
                    // ----------------------------------------

                    LONGLONG sampleTime = 0;

                    hr =
                        sample->GetSampleTime(
                            &sampleTime);

                    PrintHR(
                        "IMFSample::GetSampleTime",
                        hr);

                    std::cout
                        << "Sample time = "
                        << sampleTime
                        << std::endl;

                    // ----------------------------------------
                    // Sample duration
                    // ----------------------------------------

                    LONGLONG duration = 0;

                    hr =
                        sample->GetSampleDuration(
                            &duration);

                    PrintHR(
                        "IMFSample::GetSampleDuration",
                        hr);

                    std::cout
                        << "Sample duration = "
                        << duration
                        << std::endl;

                    // ----------------------------------------
                    // Inspect first media buffer
                    // ----------------------------------------

                    IMFMediaBuffer* buffer =
                        nullptr;

                    hr =
                        sample->GetBufferByIndex(
                            0,
                            &buffer);

                    PrintHR(
                        "IMFSample::GetBufferByIndex(0)",
                        hr);

                    if (SUCCEEDED(hr) &&
                        buffer != nullptr)
                    {
                        DWORD maxLength = 0;
                        DWORD currentLength = 0;

                        hr =
                            buffer->GetMaxLength(
                                &maxLength);

                        PrintHR(
                            "IMFMediaBuffer::GetMaxLength",
                            hr);

                        hr =
                            buffer->GetCurrentLength(
                                &currentLength);

                        PrintHR(
                            "IMFMediaBuffer::GetCurrentLength",
                            hr);

                        std::cout
                            << "Buffer max length = "
                            << maxLength
                            << std::endl;

                        std::cout
                            << "Buffer current length = "
                            << currentLength
                            << std::endl;

                        buffer->Release();
                    }

                    sample->Release();
                }
            }
            else
            {
                std::cout
                    << "MEMediaSample event did not "
                    << "contain a valid VT_UNKNOWN sample."
                    << std::endl;
            }

            PropVariantClear(
                &value);
        }
        else
        {
            std::cout
                << "Expected MEMediaSample but "
                << "received another event."
                << std::endl;
        }

        event->Release();
        event = nullptr;
    }
    else
    {
        std::cout
            << "No stream event was returned."
            << std::endl;
    }

    // ========================================================
    // [9] Stop
    // ========================================================

    std::cout
        << "\n[9] Stop\n";

    hr =
        source->Stop();

    PrintHR(
        "IMFMediaSource::Stop",
        hr);

    // ========================================================
    // [10] Shutdown
    // ========================================================

    std::cout
        << "\n[10] Shutdown\n";

    hr =
        source->Shutdown();

    PrintHR(
        "IMFMediaSource::Shutdown",
        hr);

    // ========================================================
    // Cleanup
    // ========================================================

    descriptor->Release();
    descriptor = nullptr;

    stream->Release();
    stream = nullptr;

    testInterface->Release();
    testInterface = nullptr;

    source->Release();
    source = nullptr;

    activate->Release();
    activate = nullptr;

    FreeLibrary(
        nativeDll);

    MFShutdown();
    CoUninitialize();

    std::cout
        << "\n========================================\n"
        << " Test complete\n"
        << "========================================\n";

    return 0;
}


