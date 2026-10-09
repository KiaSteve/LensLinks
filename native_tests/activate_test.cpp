#include <windows.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>

#include <cstdint>
#include <iostream>
#include <vector>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")

// ============================================================
// LensLinks private/test interface
// ============================================================

// {A7D2D9B1-6C44-4D4F-9E6B-52C7B3D84110}
struct __declspec(uuid("A7D2D9B1-6C44-4D4F-9E6B-52C7B3D84110"))
ILensLinksMediaSourceTest : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetLensLinksStream(
        IMFMediaStream** ppStream) = 0;
};

// ============================================================
// LensLinks FFI
// ============================================================

using LL_WriteFrameFn = bool (*)(
    const uint8_t* rgba,
    int size,
    int width,
    int height);

// ============================================================
// Constants
// ============================================================

static constexpr int TEST_WIDTH = 1280;
static constexpr int TEST_HEIGHT = 720;
static constexpr int BYTES_PER_PIXEL = 4;

static constexpr int TEST_FRAME_COUNT = 30;

static constexpr size_t EXPECTED_FRAME_SIZE =
    static_cast<size_t>(TEST_WIDTH) *
    static_cast<size_t>(TEST_HEIGHT) *
    static_cast<size_t>(BYTES_PER_PIXEL);

static constexpr LONGLONG EXPECTED_FRAME_DURATION = 333333;

// ============================================================
// Helpers
// ============================================================

static void PrintHR(
    const char* label,
    HRESULT hr)
{
    std::cout
        << label
        << " hr=0x"
        << std::hex
        << static_cast<unsigned long>(hr)
        << std::dec
        << std::endl;
}

// ============================================================
// Locate LL_WriteFrame
// ============================================================

static LL_WriteFrameFn GetLLWriteFrame()
{
    HMODULE module =
        GetModuleHandleW(
            L"LensLinksNative.dll");

    if (module == nullptr)
    {
        std::cerr
            << "[Test] GetModuleHandleW("
               "LensLinksNative.dll) failed. "
            << "GLE="
            << GetLastError()
            << std::endl;

        return nullptr;
    }

    FARPROC proc =
        GetProcAddress(
            module,
            "LL_WriteFrame");

    if (proc == nullptr)
    {
        std::cerr
            << "[Test] GetProcAddress("
               "LL_WriteFrame) failed. "
            << "GLE="
            << GetLastError()
            << std::endl;

        return nullptr;
    }

    std::cout
        << "[Test] LL_WriteFrame located"
        << std::endl;

    return reinterpret_cast<LL_WriteFrameFn>(
        proc);
}

// ============================================================
// Create deterministic frame
//
// Each frame has a different first-pixel red value.
// This allows us to confirm that new frame data is actually
// being passed through the pipeline.
// ============================================================

static bool CreateTestFrame(
    std::vector<uint8_t>& frame,
    int frameNumber)
{
    frame.resize(
        EXPECTED_FRAME_SIZE);

    const uint8_t frameMarker =
        static_cast<uint8_t>(
            frameNumber & 0xFF);

    for (int y = 0; y < TEST_HEIGHT; ++y)
    {
        for (int x = 0; x < TEST_WIDTH; ++x)
        {
            const size_t offset =
                (
                    static_cast<size_t>(y) *
                    static_cast<size_t>(TEST_WIDTH) +
                    static_cast<size_t>(x)
                ) *
                BYTES_PER_PIXEL;

            // RGB32 memory layout:
            //
            // B G R A
            //
            // Blue changes with X.
            // Green changes with Y.
            // Red contains the frame number.
            // Alpha is always 255.

            frame[offset + 0] =
                static_cast<uint8_t>(
                    x & 0xFF);

            frame[offset + 1] =
                static_cast<uint8_t>(
                    y & 0xFF);

            frame[offset + 2] =
                frameMarker;

            frame[offset + 3] =
                0xFF;
        }
    }

    return true;
}

// ============================================================
// Verify one media sample
// ============================================================

static bool VerifyMediaSample(
    IMFMediaStream* mediaStream,
    int expectedFrameNumber,
    LONGLONG expectedSampleTime)
{
    if (mediaStream == nullptr)
    {
        std::cerr
            << "[Test] mediaStream is null"
            << std::endl;

        return false;
    }

    IMFMediaEvent* streamEvent = nullptr;

    HRESULT hr =
        mediaStream->GetEvent(
            0,
            &streamEvent);

    PrintHR(
        "[Test] Stream GetEvent",
        hr);

    if (FAILED(hr) ||
        streamEvent == nullptr)
    {
        return false;
    }

    MediaEventType eventType =
        MEUnknown;

    hr =
        streamEvent->GetType(
            &eventType);

    PrintHR(
        "[Test] Stream Event GetType",
        hr);

    if (FAILED(hr))
    {
        streamEvent->Release();
        return false;
    }

    if (eventType != MEMediaSample)
    {
        std::cerr
            << "[Test] ERROR: expected MEMediaSample, got "
            << static_cast<int>(eventType)
            << std::endl;

        streamEvent->Release();
        return false;
    }

    PROPVARIANT eventValue;

    PropVariantInit(
        &eventValue);

    hr =
        streamEvent->GetValue(
            &eventValue);

    PrintHR(
        "[Test] Stream Event GetValue",
        hr);

    if (FAILED(hr))
    {
        PropVariantClear(
            &eventValue);

        streamEvent->Release();

        return false;
    }

    bool success = false;

    if (eventValue.vt == VT_UNKNOWN &&
        eventValue.punkVal != nullptr)
    {
        IMFSample* sample = nullptr;

        hr =
            eventValue.punkVal
                ->QueryInterface(
                    IID_PPV_ARGS(
                        &sample));

        PrintHR(
            "[Test] QueryInterface(IMFSample)",
            hr);

        if (SUCCEEDED(hr) &&
            sample != nullptr)
        {
            IMFMediaBuffer* sampleBuffer =
                nullptr;

            hr =
                sample->GetBufferByIndex(
                    0,
                    &sampleBuffer);

            PrintHR(
                "[Test] GetBufferByIndex",
                hr);

            if (SUCCEEDED(hr) &&
                sampleBuffer != nullptr)
            {
                DWORD maxLength = 0;
                DWORD currentLength = 0;

                hr =
                    sampleBuffer->GetMaxLength(
                        &maxLength);

                PrintHR(
                    "[Test] GetMaxLength",
                    hr);

                if (SUCCEEDED(hr))
                {
                    hr =
                        sampleBuffer
                            ->GetCurrentLength(
                                &currentLength);

                    PrintHR(
                        "[Test] GetCurrentLength",
                        hr);
                }

                if (currentLength !=
                    EXPECTED_FRAME_SIZE)
                {
                    std::cerr
                        << "[Test] ERROR: frame "
                           "buffer size mismatch. "
                        << "Expected="
                        << EXPECTED_FRAME_SIZE
                        << " Actual="
                        << currentLength
                        << std::endl;

                    sampleBuffer->Release();
                    sample->Release();

                    PropVariantClear(
                        &eventValue);

                    streamEvent->Release();

                    return false;
                }

                // ------------------------------------------------
                // Verify frame marker
                // ------------------------------------------------

                BYTE* sampleData = nullptr;

                DWORD sampleMax = 0;
                DWORD sampleCurrent = 0;

                hr =
                    sampleBuffer->Lock(
                        &sampleData,
                        &sampleMax,
                        &sampleCurrent);

                PrintHR(
                    "[Test] Sample buffer Lock",
                    hr);

                if (SUCCEEDED(hr) &&
                    sampleData != nullptr &&
                    sampleCurrent >= 4)
                {
                    const uint8_t expectedRed =
                        static_cast<uint8_t>(
                            expectedFrameNumber & 0xFF);

                    const bool pixelCorrect =
                        sampleData[0] == 0 &&
                        sampleData[1] == 0 &&
                        sampleData[2] == expectedRed &&
                        sampleData[3] == 0xFF;

                    if (!pixelCorrect)
                    {
                        std::cerr
                            << "[Test] ERROR: frame "
                               "marker mismatch. "
                            << "Expected BGRA=0 0 "
                            << static_cast<unsigned int>(
                                   expectedRed)
                            << " 255"
                            << " Actual="
                            << static_cast<unsigned int>(
                                   sampleData[0])
                            << " "
                            << static_cast<unsigned int>(
                                   sampleData[1])
                            << " "
                            << static_cast<unsigned int>(
                                   sampleData[2])
                            << " "
                            << static_cast<unsigned int>(
                                   sampleData[3])
                            << std::endl;

                        sampleBuffer->Unlock();

                        sampleBuffer->Release();
                        sample->Release();

                        PropVariantClear(
                            &eventValue);

                        streamEvent->Release();

                        return false;
                    }

                    sampleBuffer->Unlock();
                }
                else
                {
                    std::cerr
                        << "[Test] ERROR: unable to "
                           "lock sample buffer"
                        << std::endl;

                    sampleBuffer->Release();
                    sample->Release();

                    PropVariantClear(
                        &eventValue);

                    streamEvent->Release();

                    return false;
                }

                sampleBuffer->Release();
                sampleBuffer = nullptr;
            }
            else
            {
                sample->Release();

                PropVariantClear(
                    &eventValue);

                streamEvent->Release();

                return false;
            }

            // ------------------------------------------------
            // Verify timestamp
            // ------------------------------------------------

            LONGLONG sampleTime = 0;

            hr =
                sample->GetSampleTime(
                    &sampleTime);

            PrintHR(
                "[Test] GetSampleTime",
                hr);

            if (FAILED(hr))
            {
                sample->Release();

                PropVariantClear(
                    &eventValue);

                streamEvent->Release();

                return false;
            }

            if (sampleTime !=
                expectedSampleTime)
            {
                std::cerr
                    << "[Test] ERROR: timestamp mismatch. "
                    << "Expected="
                    << expectedSampleTime
                    << " Actual="
                    << sampleTime
                    << std::endl;

                sample->Release();

                PropVariantClear(
                    &eventValue);

                streamEvent->Release();

                return false;
            }

            // ------------------------------------------------
            // Verify duration
            // ------------------------------------------------

            LONGLONG sampleDuration = 0;

            hr =
                sample->GetSampleDuration(
                    &sampleDuration);

            PrintHR(
                "[Test] GetSampleDuration",
                hr);

            if (FAILED(hr))
            {
                sample->Release();

                PropVariantClear(
                    &eventValue);

                streamEvent->Release();

                return false;
            }

            if (sampleDuration !=
                EXPECTED_FRAME_DURATION)
            {
                std::cerr
                    << "[Test] ERROR: duration mismatch. "
                    << "Expected="
                    << EXPECTED_FRAME_DURATION
                    << " Actual="
                    << sampleDuration
                    << std::endl;

                sample->Release();

                PropVariantClear(
                    &eventValue);

                streamEvent->Release();

                return false;
            }

            success = true;

            sample->Release();
            sample = nullptr;
        }
    }

    PropVariantClear(
        &eventValue);

    streamEvent->Release();
    streamEvent = nullptr;

    return success;
}

// ============================================================
// Main
// ============================================================

int main()
{
    HRESULT hr = S_OK;

    bool overallSuccess = true;

    // ========================================================
    // COM
    // ========================================================

    hr =
        CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

    if (FAILED(hr))
    {
        PrintHR(
            "[Test] CoInitializeEx failed",
            hr);

        return 1;
    }

    std::cout
        << "[Test] COM initialized"
        << std::endl;

    // ========================================================
    // Media Foundation
    // ========================================================

    hr =
        MFStartup(
            MF_VERSION);

    if (FAILED(hr))
    {
        PrintHR(
            "[Test] MFStartup failed",
            hr);

        CoUninitialize();

        return 1;
    }

    std::cout
        << "[Test] Media Foundation initialized"
        << std::endl;

    // ========================================================
    // LensLinks CLSID
    // ========================================================

    const CLSID clsid =
    {
        0x8F8B9C41,
        0x4A6D,
        0x4C7B,
        {
            0x91,
            0x2A,
            0x7E,
            0x52,
            0x9D,
            0x13,
            0x84,
            0x61
        }
    };

    // ========================================================
    // Create IMFActivate
    // ========================================================

    IMFActivate* activate = nullptr;

    hr =
        CoCreateInstance(
            clsid,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IMFActivate,
            reinterpret_cast<void**>(
                &activate));

    PrintHR(
        "[Test] CoCreateInstance(IMFActivate)",
        hr);

    if (FAILED(hr))
    {
        MFShutdown();
        CoUninitialize();

        return 1;
    }

    std::cout
        << "[Test] IMFActivate acquired"
        << std::endl;

    // ========================================================
    // Activate source
    // ========================================================

    IMFMediaSource* source = nullptr;

    hr =
        activate->ActivateObject(
            IID_IMFMediaSource,
            reinterpret_cast<void**>(
                &source));

    PrintHR(
        "[Test] ActivateObject",
        hr);

    if (FAILED(hr))
    {
        activate->Release();

        MFShutdown();
        CoUninitialize();

        return 1;
    }

    std::cout
        << "[Test] IMFMediaSource acquired"
        << std::endl;

    // ========================================================
    // Presentation descriptor
    // ========================================================

    IMFPresentationDescriptor*
        presentationDescriptor = nullptr;

    hr =
        source->CreatePresentationDescriptor(
            &presentationDescriptor);

    PrintHR(
        "[Test] CreatePresentationDescriptor",
        hr);

    if (FAILED(hr))
    {
        source->Shutdown();
        source->Release();

        activate->ShutdownObject();
        activate->Release();

        MFShutdown();
        CoUninitialize();

        return 1;
    }

    // ========================================================
    // Stream descriptor
    // ========================================================

    DWORD streamCount = 0;

    hr =
        presentationDescriptor
            ->GetStreamDescriptorCount(
                &streamCount);

    PrintHR(
        "[Test] GetStreamDescriptorCount",
        hr);

    std::cout
        << "[Test] Stream count: "
        << streamCount
        << std::endl;

    if (FAILED(hr) ||
        streamCount == 0)
    {
        presentationDescriptor->Release();

        source->Shutdown();
        source->Release();

        activate->ShutdownObject();
        activate->Release();

        MFShutdown();
        CoUninitialize();

        return 1;
    }

    BOOL selected = FALSE;

    IMFStreamDescriptor*
        streamDescriptor = nullptr;

    hr =
        presentationDescriptor
            ->GetStreamDescriptorByIndex(
                0,
                &selected,
                &streamDescriptor);

    PrintHR(
        "[Test] GetStreamDescriptorByIndex",
        hr);

    if (FAILED(hr) ||
        streamDescriptor == nullptr)
    {
        presentationDescriptor->Release();

        source->Shutdown();
        source->Release();

        activate->ShutdownObject();
        activate->Release();

        MFShutdown();
        CoUninitialize();

        return 1;
    }

    // ========================================================
    // Media type
    // ========================================================

    IMFMediaTypeHandler*
        mediaTypeHandler = nullptr;

    hr =
        streamDescriptor
            ->GetMediaTypeHandler(
                &mediaTypeHandler);

    PrintHR(
        "[Test] GetMediaTypeHandler",
        hr);

    if (SUCCEEDED(hr))
    {
        DWORD mediaTypeCount = 0;

        hr =
            mediaTypeHandler
                ->GetMediaTypeCount(
                    &mediaTypeCount);

        PrintHR(
            "[Test] GetMediaTypeCount",
            hr);

        std::cout
            << "[Test] Media type count: "
            << mediaTypeCount
            << std::endl;

        if (SUCCEEDED(hr) &&
            mediaTypeCount > 0)
        {
            IMFMediaType* mediaType = nullptr;

            hr =
                mediaTypeHandler
                    ->GetMediaTypeByIndex(
                        0,
                        &mediaType);

            PrintHR(
                "[Test] GetMediaTypeByIndex(0)",
                hr);

            if (SUCCEEDED(hr) &&
                mediaType != nullptr)
            {
                UINT32 width = 0;
                UINT32 height = 0;

                hr =
                    MFGetAttributeSize(
                        mediaType,
                        MF_MT_FRAME_SIZE,
                        &width,
                        &height);

                PrintHR(
                    "[Test] Get frame size",
                    hr);

                UINT32 fpsNum = 0;
                UINT32 fpsDen = 0;

                hr =
                    MFGetAttributeRatio(
                        mediaType,
                        MF_MT_FRAME_RATE,
                        &fpsNum,
                        &fpsDen);

                PrintHR(
                    "[Test] Get frame rate",
                    hr);

                if (SUCCEEDED(hr))
                {
                    std::cout
                        << "[Test] Media type: "
                        << width
                        << "x"
                        << height
                        << " @ "
                        << fpsNum
                        << "/"
                        << fpsDen
                        << " FPS"
                        << std::endl;
                }

                mediaType->Release();
            }
        }

        mediaTypeHandler->Release();
    }

    // ========================================================
    // Obtain actual LensLinks media stream
    // ========================================================

    ILensLinksMediaSourceTest*
        lensLinksTest = nullptr;

    hr =
        source->QueryInterface(
            __uuidof(
                ILensLinksMediaSourceTest),
            reinterpret_cast<void**>(
                &lensLinksTest));

    PrintHR(
        "[Test] QueryInterface("
        "ILensLinksMediaSourceTest)",
        hr);

    IMFMediaStream* mediaStream = nullptr;

    if (SUCCEEDED(hr))
    {
        hr =
            lensLinksTest
                ->GetLensLinksStream(
                    &mediaStream);

        PrintHR(
            "[Test] GetLensLinksStream",
            hr);

        lensLinksTest->Release();
        lensLinksTest = nullptr;
    }

    if (mediaStream == nullptr)
    {
        std::cerr
            << "[Test] ERROR: IMFMediaStream unavailable"
            << std::endl;

        overallSuccess = false;
    }

    // ========================================================
    // Start source
    // ========================================================

    std::cout
        << "[Test] Starting media source..."
        << std::endl;

    hr =
        source->Start(
            presentationDescriptor,
            nullptr,
            nullptr);

    PrintHR(
        "[Test] Source Start",
        hr);

    if (FAILED(hr))
    {
        overallSuccess = false;
    }

    // ========================================================
    // Read source startup events
    // ========================================================

    if (SUCCEEDED(hr))
    {
        for (int i = 0; i < 2; ++i)
        {
            IMFMediaEvent* event = nullptr;

            hr =
                source->GetEvent(
                    0,
                    &event);

            PrintHR(
                "[Test] Source GetEvent",
                hr);

            if (FAILED(hr))
            {
                overallSuccess = false;
                break;
            }

            MediaEventType eventType =
                MEUnknown;

            hr =
                event->GetType(
                    &eventType);

            PrintHR(
                "[Test] Event GetType",
                hr);

            event->Release();

            if (FAILED(hr))
            {
                overallSuccess = false;
                break;
            }
        }
    }

    // ========================================================
    // Locate LL_WriteFrame
    // ========================================================

    LL_WriteFrameFn writeFrame =
        GetLLWriteFrame();

    if (writeFrame == nullptr)
    {
        overallSuccess = false;
    }

    // ========================================================
    // Continuous 30-frame test
    // ========================================================

    if (mediaStream != nullptr &&
        writeFrame != nullptr &&
        overallSuccess)
    {
        std::cout
            << std::endl
            << "[Test] =================================="
            << std::endl
            << "[Test] Starting "
            << TEST_FRAME_COUNT
            << "-frame continuous test"
            << std::endl
            << "[Test] =================================="
            << std::endl;

        LONGLONG expectedSampleTime = 0;

        for (int frameNumber = 0;
             frameNumber < TEST_FRAME_COUNT;
             ++frameNumber)
        {
            std::vector<uint8_t> frame;

            if (!CreateTestFrame(
                    frame,
                    frameNumber))
            {
                std::cerr
                    << "[Test] ERROR: failed to create frame "
                    << frameNumber
                    << std::endl;

                overallSuccess = false;
                break;
            }

            std::cout
                << "[Test] Frame "
                << (frameNumber + 1)
                << "/"
                << TEST_FRAME_COUNT
                << ": LL_WriteFrame..."
                << std::endl;

            const bool writeResult =
                writeFrame(
                    frame.data(),
                    static_cast<int>(
                        frame.size()),
                    TEST_WIDTH,
                    TEST_HEIGHT);

            if (!writeResult)
            {
                std::cerr
                    << "[Test] ERROR: LL_WriteFrame failed "
                       "for frame "
                    << frameNumber
                    << std::endl;

                overallSuccess = false;
                break;
            }

            hr =
                mediaStream->RequestSample(
                    nullptr);

            PrintHR(
                "[Test] RequestSample",
                hr);

            if (FAILED(hr))
            {
                std::cerr
                    << "[Test] ERROR: RequestSample failed "
                       "for frame "
                    << frameNumber
                    << std::endl;

                overallSuccess = false;
                break;
            }

            const bool frameSuccess =
                VerifyMediaSample(
                    mediaStream,
                    frameNumber,
                    expectedSampleTime);

            if (!frameSuccess)
            {
                std::cerr
                    << "[Test] ERROR: verification failed "
                       "for frame "
                    << frameNumber
                    << std::endl;

                overallSuccess = false;
                break;
            }

            std::cout
                << "[Test] Frame "
                << (frameNumber + 1)
                << "/"
                << TEST_FRAME_COUNT
                << " VERIFIED"
                << std::endl;

            expectedSampleTime +=
                EXPECTED_FRAME_DURATION;
        }
    }

    // ========================================================
    // Stop
    // ========================================================

    if (source != nullptr)
    {
        hr =
            source->Stop();

        PrintHR(
            "[Test] Source Stop",
            hr);
    }

    // ========================================================
    // Cleanup
    // ========================================================

    if (mediaStream != nullptr)
    {
        mediaStream->Release();
        mediaStream = nullptr;
    }

    if (streamDescriptor != nullptr)
    {
        streamDescriptor->Release();
        streamDescriptor = nullptr;
    }

    if (presentationDescriptor != nullptr)
    {
        presentationDescriptor->Release();
        presentationDescriptor = nullptr;
    }

    if (source != nullptr)
    {
        hr =
            source->Shutdown();

        PrintHR(
            "[Test] Source Shutdown",
            hr);

        source->Release();
        source = nullptr;
    }

    if (activate != nullptr)
    {
        hr =
            activate->ShutdownObject();

        PrintHR(
            "[Test] Activation ShutdownObject",
            hr);

        activate->Release();
        activate = nullptr;
    }

    MFShutdown();
    CoUninitialize();

    // ========================================================
    // Final result
    // ========================================================

    if (overallSuccess)
    {
        std::cout
            << std::endl
            << "[Test] =================================="
            << std::endl
            << "[Test] 30-FRAME MEDIA PIPELINE TEST PASSED"
            << std::endl
            << "[Test] =================================="
            << std::endl;

        return 0;
    }

    std::cerr
        << std::endl
        << "[Test] =================================="
        << std::endl
        << "[Test] 30-FRAME MEDIA PIPELINE TEST FAILED"
        << std::endl
        << "[Test] =================================="
        << std::endl;

    return 1;
}