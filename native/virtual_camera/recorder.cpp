// recorder.cpp
//
// Records what the virtual camera shows into an H.264 MP4 file using the
// Media Foundation sink writer. Frames arrive through Recorder_Submit and are
// encoded on a worker thread, so the caller never waits for the encoder. If
// frames arrive faster than they can be encoded, the unwritten one is replaced.

#include "recorder.h"

#define NOMINMAX
#include <windows.h>

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using Microsoft::WRL::ComPtr;

namespace
{
    constexpr UINT32 kBitrate = 4000000;
    constexpr LONGLONG kMinFrameDuration = 10000;     // 1 ms, in 100 ns units
    constexpr LONGLONG kLastFrameDuration = 1000000;  // 100 ms

    struct State
    {
        std::mutex control;  // serialises Start and Stop
        std::mutex mutex;    // protects the pending frame
        std::condition_variable cv;
        std::thread worker;

        std::atomic<bool> recording{false};
        bool stopRequested = false;

        std::vector<uint8_t> pending;
        bool hasPending = false;
        LONGLONG pendingTime = 0;

        int width = 0;
        int height = 0;

        LONGLONG startCounter = 0;
        LONGLONG frequency = 1;

        bool finalizeOk = false;
    };

    State& state()
    {
        // Never destroyed: the process may exit while the worker still exists.
        static State* s = new State();
        return *s;
    }

    LONGLONG elapsedHns(const State& s)
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        return (now.QuadPart - s.startCounter) * 10000000LL / s.frequency;
    }

    int clampInt(int value, int low, int high)
    {
        return value < low ? low : (value > high ? high : value);
    }

    // RGBA (R,G,B,A byte order) to NV12, BT.709, limited range. Chroma is
    // averaged over each 2x2 block.
    void rgbaToNv12(
        const uint8_t* rgba,
        int width,
        int height,
        std::vector<uint8_t>& nv12)
    {
        const size_t w = static_cast<size_t>(width);
        const size_t h = static_cast<size_t>(height);

        nv12.resize(w * h * 3 / 2);

        uint8_t* yPlane = nv12.data();
        uint8_t* uvPlane = nv12.data() + w * h;

        for (size_t y = 0; y < h; y += 2)
        {
            const uint8_t* row0 = rgba + y * w * 4;
            const uint8_t* row1 = row0 + w * 4;

            uint8_t* y0 = yPlane + y * w;
            uint8_t* y1 = y0 + w;
            uint8_t* uv = uvPlane + (y / 2) * w;

            for (size_t x = 0; x < w; x += 2)
            {
                int uSum = 0;
                int vSum = 0;

                for (int k = 0; k < 4; ++k)
                {
                    const uint8_t* p = (k < 2 ? row0 : row1) + (x + (k & 1)) * 4;

                    const int r = p[0];
                    const int g = p[1];
                    const int b = p[2];

                    const int luma = ((47 * r + 157 * g + 16 * b + 128) >> 8) + 16;

                    (k < 2 ? y0 : y1)[x + (k & 1)] =
                        static_cast<uint8_t>(clampInt(luma, 16, 235));

                    uSum += ((-26 * r - 87 * g + 112 * b + 128) >> 8) + 128;
                    vSum += ((112 * r - 102 * g - 10 * b + 128) >> 8) + 128;
                }

                uv[x] = static_cast<uint8_t>(clampInt((uSum + 2) / 4, 16, 240));
                uv[x + 1] = static_cast<uint8_t>(clampInt((vSum + 2) / 4, 16, 240));
            }
        }
    }

    HRESULT setVideoType(
        IMFMediaType* type,
        const GUID& subtype,
        int width,
        int height)
    {
        HRESULT hr = type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        if (FAILED(hr)) return hr;

        hr = type->SetGUID(MF_MT_SUBTYPE, subtype);
        if (FAILED(hr)) return hr;

        hr = type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
        if (FAILED(hr)) return hr;

        hr = MFSetAttributeSize(type, MF_MT_FRAME_SIZE, width, height);
        if (FAILED(hr)) return hr;

        hr = MFSetAttributeRatio(type, MF_MT_FRAME_RATE, 30, 1);
        if (FAILED(hr)) return hr;

        return MFSetAttributeRatio(type, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    }

    HRESULT createWriter(
        const std::wstring& path,
        int width,
        int height,
        ComPtr<IMFSinkWriter>& writer,
        DWORD& streamIndex)
    {
        HRESULT hr = MFCreateSinkWriterFromURL(
            path.c_str(),
            nullptr,
            nullptr,
            &writer);
        if (FAILED(hr)) return hr;

        ComPtr<IMFMediaType> outputType;
        hr = MFCreateMediaType(&outputType);
        if (FAILED(hr)) return hr;

        hr = setVideoType(outputType.Get(), MFVideoFormat_H264, width, height);
        if (FAILED(hr)) return hr;

        hr = outputType->SetUINT32(MF_MT_AVG_BITRATE, kBitrate);
        if (FAILED(hr)) return hr;

        hr = writer->AddStream(outputType.Get(), &streamIndex);
        if (FAILED(hr)) return hr;

        ComPtr<IMFMediaType> inputType;
        hr = MFCreateMediaType(&inputType);
        if (FAILED(hr)) return hr;

        hr = setVideoType(inputType.Get(), MFVideoFormat_NV12, width, height);
        if (FAILED(hr)) return hr;

        hr = inputType->SetUINT32(MF_MT_DEFAULT_STRIDE, static_cast<UINT32>(width));
        if (FAILED(hr)) return hr;

        hr = writer->SetInputMediaType(streamIndex, inputType.Get(), nullptr);
        if (FAILED(hr)) return hr;

        return writer->BeginWriting();
    }

    HRESULT writeSample(
        IMFSinkWriter* writer,
        DWORD streamIndex,
        const std::vector<uint8_t>& nv12,
        LONGLONG time,
        LONGLONG duration)
    {
        ComPtr<IMFMediaBuffer> buffer;
        HRESULT hr = MFCreateMemoryBuffer(static_cast<DWORD>(nv12.size()), &buffer);
        if (FAILED(hr)) return hr;

        BYTE* data = nullptr;
        hr = buffer->Lock(&data, nullptr, nullptr);
        if (FAILED(hr)) return hr;

        memcpy(data, nv12.data(), nv12.size());

        buffer->Unlock();

        hr = buffer->SetCurrentLength(static_cast<DWORD>(nv12.size()));
        if (FAILED(hr)) return hr;

        ComPtr<IMFSample> sample;
        hr = MFCreateSample(&sample);
        if (FAILED(hr)) return hr;

        hr = sample->AddBuffer(buffer.Get());
        if (FAILED(hr)) return hr;

        hr = sample->SetSampleTime(time);
        if (FAILED(hr)) return hr;

        hr = sample->SetSampleDuration(duration);
        if (FAILED(hr)) return hr;

        return writer->WriteSample(streamIndex, sample.Get());
    }

    void logHr(const char* what, HRESULT hr)
    {
        std::cout
            << "[Recorder] " << what << " hr=0x"
            << std::hex << static_cast<unsigned long>(hr) << std::dec
            << std::endl;
    }

    void workerMain(
        std::wstring path,
        int width,
        int height,
        std::promise<bool> ready)
    {
        State& s = state();

        const HRESULT comHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        const bool comInitialised = SUCCEEDED(comHr);

        bool mfStarted = false;
        ComPtr<IMFSinkWriter> writer;
        DWORD streamIndex = 0;

        HRESULT hr = MFStartup(MF_VERSION);

        if (SUCCEEDED(hr))
        {
            mfStarted = true;
            hr = createWriter(path, width, height, writer, streamIndex);
        }

        if (FAILED(hr))
        {
            logHr("cannot start", hr);

            writer.Reset();

            if (mfStarted) MFShutdown();
            if (comInitialised) CoUninitialize();

            s.finalizeOk = false;
            ready.set_value(false);
            return;
        }

        LARGE_INTEGER frequency;
        LARGE_INTEGER now;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&now);

        s.frequency = frequency.QuadPart;
        s.startCounter = now.QuadPart;

        std::cout << "[Recorder] recording started" << std::endl;

        ready.set_value(true);

        std::vector<uint8_t> frame;
        std::vector<uint8_t> nv12;
        std::vector<uint8_t> previous;

        LONGLONG previousTime = 0;
        LONGLONG baseTime = 0;

        bool havePrevious = false;
        bool haveBase = false;
        bool failed = false;

        unsigned written = 0;

        while (true)
        {
            LONGLONG time = 0;

            {
                std::unique_lock<std::mutex> lock(s.mutex);

                s.cv.wait(lock, [&] { return s.hasPending || s.stopRequested; });

                if (!s.hasPending)
                {
                    break;
                }

                frame.swap(s.pending);
                time = s.pendingTime;
                s.hasPending = false;
            }

            // After a write error keep draining, so Submit never backs up.
            if (failed)
            {
                continue;
            }

            if (!haveBase)
            {
                baseTime = time;
                haveBase = true;
            }

            time -= baseTime;

            rgbaToNv12(frame.data(), width, height, nv12);

            // A sample's duration is the gap to the next one, so each frame
            // is written when the following frame arrives.
            if (havePrevious)
            {
                const LONGLONG duration =
                    (std::max)(time - previousTime, kMinFrameDuration);

                hr = writeSample(
                    writer.Get(),
                    streamIndex,
                    previous,
                    previousTime,
                    duration);

                if (FAILED(hr))
                {
                    logHr("write failed", hr);
                    failed = true;
                    continue;
                }

                ++written;
            }

            previous.swap(nv12);
            previousTime = time;
            havePrevious = true;
        }

        if (!failed && havePrevious)
        {
            hr = writeSample(
                writer.Get(),
                streamIndex,
                previous,
                previousTime,
                kLastFrameDuration);

            if (SUCCEEDED(hr))
            {
                ++written;
            }
            else
            {
                logHr("write failed", hr);
                failed = true;
            }
        }

        hr = writer->Finalize();

        logHr("finalize", hr);

        std::cout
            << "[Recorder] finished, frames written: " << written
            << std::endl;

        s.finalizeOk = SUCCEEDED(hr) && !failed && written > 0;

        writer.Reset();

        MFShutdown();

        if (comInitialised)
        {
            CoUninitialize();
        }
    }
}

bool Recorder_Start(const wchar_t* path, int width, int height)
{
    State& s = state();

    std::lock_guard<std::mutex> control(s.control);

    if (s.recording.load() || s.worker.joinable())
    {
        return false;
    }

    // H.264 needs even dimensions.
    if (path == nullptr || width <= 0 || height <= 0 ||
        (width & 1) != 0 || (height & 1) != 0)
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s.mutex);

        s.width = width;
        s.height = height;
        s.stopRequested = false;
        s.hasPending = false;
        s.pending.clear();
    }

    std::promise<bool> ready;
    std::future<bool> readyResult = ready.get_future();

    s.worker = std::thread(
        workerMain,
        std::wstring(path),
        width,
        height,
        std::move(ready));

    const bool ok = readyResult.get();

    if (!ok)
    {
        s.worker.join();
        return false;
    }

    s.recording = true;

    return true;
}

bool Recorder_Stop()
{
    State& s = state();

    std::lock_guard<std::mutex> control(s.control);

    if (!s.worker.joinable())
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s.mutex);

        s.recording = false;
        s.stopRequested = true;
    }

    s.cv.notify_one();

    s.worker.join();

    return s.finalizeOk;
}

bool Recorder_IsRecording()
{
    return state().recording.load();
}

void Recorder_Submit(const uint8_t* rgba, int width, int height)
{
    State& s = state();

    if (!s.recording.load() || rgba == nullptr)
    {
        return;
    }

    if (width != s.width || height != s.height)
    {
        return;
    }

    const size_t bytes = static_cast<size_t>(width) * height * 4;

    std::lock_guard<std::mutex> lock(s.mutex);

    if (s.stopRequested)
    {
        return;
    }

    s.pending.assign(rgba, rgba + bytes);
    s.pendingTime = elapsedHns(s);
    s.hasPending = true;

    s.cv.notify_one();
}
