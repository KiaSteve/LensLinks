#include "lenslinks_media_stream.h"
#include <string>
static std::string ll_narrow(const wchar_t* w) { std::string s; for (; w && *w; ++w) s.push_back(static_cast<char>(*w)); return s; }

#include "lenslinks_media_source.h"

#include "../virtual_camera/virtual_camera.h"

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfobjects.h>

#include <cstring>
#include <iostream>
#include <limits>
#include <ks.h>
#include <ksmedia.h>
#include <vector>

namespace
{
    int clampInt(int value, int low, int high)
    {
        return value < low ? low : (value > high ? high : value);
    }

    // Converts RGBA bytes (R,G,B,A order) to NV12 using BT.709 with limited
    // range (Y 16..235, U/V 16..240). Chroma is averaged over each 2x2 block.
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
}

// ========================================================
// Constructor
// ========================================================

LensLinksMediaStream::LensLinksMediaStream(
    LensLinksMediaSource* source)
    : source_(source)
{
    if (source_ != nullptr)
    {
        source_->AddRef();
    }

    // Create stream attributes FIRST
    HRESULT hr =
        MFCreateAttributes(
            &streamAttributes_,
            8);

    if (FAILED(hr))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "MFCreateAttributes failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    // Identify this as a video capture stream
    // Identify this as a video capture stream
hr =
    streamAttributes_->SetGUID(
        MF_DEVICESTREAM_STREAM_CATEGORY,
        PINNAME_VIDEO_CAPTURE);

if (FAILED(hr))
{
    std::cout
        << "[LensLinksMediaStream] "
        << "Set stream category failed hr=0x"
        << std::hex
        << static_cast<unsigned long>(hr)
        << std::dec
        << std::endl;

    return;
}

    // Stream identifier
    hr =
        streamAttributes_->SetUINT32(
            MF_DEVICESTREAM_STREAM_ID,
            0);

    if (FAILED(hr))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "Set stream ID failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    // Mark this as a Frame Server shared stream
    hr =
        streamAttributes_->SetUINT32(
            MF_DEVICESTREAM_FRAMESERVER_SHARED,
            1);

    if (FAILED(hr))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "Set Frame Server shared failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    // This stream provides color frames
    hr =
        streamAttributes_->SetUINT32(
            MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES,
            MFFrameSourceTypes::MFFrameSourceTypes_Color);

    if (FAILED(hr))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "Set frame source types failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    // Then create the event queue
    hr =
        MFCreateEventQueue(
            &eventQueue_);

    if (FAILED(hr))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "MFCreateEventQueue failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    streamState_ =
        MF_STREAM_STATE_STOPPED;

    std::cout
        << "[LensLinksMediaStream] created"
        << std::endl;
}

// ========================================================
// Destructor
// ========================================================

// ========================================================
// Destructor
// ========================================================

LensLinksMediaStream::~LensLinksMediaStream()
{
    shutdown_ = true;

    if (eventQueue_ != nullptr)
    {
        eventQueue_->Shutdown();

        eventQueue_->Release();

        eventQueue_ = nullptr;
    }

    if (streamDescriptor_ != nullptr)
    {
        streamDescriptor_->Release();

        streamDescriptor_ = nullptr;
    }

    if (streamAttributes_ != nullptr)
    {
        streamAttributes_->Release();

        streamAttributes_ = nullptr;
    }

    if (source_ != nullptr)
    {
        source_->Release();

        source_ = nullptr;
    }

    std::cout
        << "[LensLinksMediaStream] destroyed"
        << std::endl;
}

// ========================================================
// IUnknown
// ========================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::QueryInterface(
    REFIID riid,
    void** ppvObject)
{
    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }

    *ppvObject = nullptr;

    if (riid == IID_IUnknown)
    {
        *ppvObject =
            static_cast<IUnknown*>(
                static_cast<IMFMediaStream*>(this));
    }
    else if (riid == IID_IMFMediaEventGenerator)
    {
        *ppvObject =
            static_cast<IMFMediaEventGenerator*>(
                static_cast<IMFMediaStream*>(this));
    }
    else if (riid == IID_IMFMediaStream)
    {
        *ppvObject =
            static_cast<IMFMediaStream*>(this);
    }
    else if (riid == IID_IMFMediaStream2)
    {
        *ppvObject =
            static_cast<IMFMediaStream2*>(this);
    }
    else
    {
        {
        static LONG refusedCount = 0;
        if (InterlockedIncrement(&refusedCount) <= 20)
        {
            LPOLESTR refused = nullptr;
            if (SUCCEEDED(StringFromIID(riid, &refused)))
            {
                std::cout << "[LensLinks] QueryInterface refused (first 20): "
                          << ll_narrow(refused) << std::endl;
                CoTaskMemFree(refused);
            }
        }
    }

        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

ULONG STDMETHODCALLTYPE
LensLinksMediaStream::AddRef()
{
    return static_cast<ULONG>(
        InterlockedIncrement(
            &refCount_));
}

ULONG STDMETHODCALLTYPE
LensLinksMediaStream::Release()
{
    ULONG count =
        static_cast<ULONG>(
            InterlockedDecrement(
                &refCount_));

    if (count == 0)
    {
        delete this;
    }

    return count;
}

// ========================================================
// IMFMediaEventGenerator
// ========================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::GetEvent(
    DWORD dwFlags,
    IMFMediaEvent** ppEvent)
{
    if (ppEvent == nullptr)
    {
        return E_POINTER;
    }

    *ppEvent = nullptr;

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (eventQueue_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return eventQueue_->GetEvent(
        dwFlags,
        ppEvent);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::BeginGetEvent(
    IMFAsyncCallback* pCallback,
    IUnknown* punkState)
{
    if (pCallback == nullptr)
    {
        return E_POINTER;
    }

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (eventQueue_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return eventQueue_->BeginGetEvent(
        pCallback,
        punkState);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::EndGetEvent(
    IMFAsyncResult* pResult,
    IMFMediaEvent** ppEvent)
{
    if (pResult == nullptr)
    {
        return E_POINTER;
    }

    if (ppEvent == nullptr)
    {
        return E_POINTER;
    }

    *ppEvent = nullptr;

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (eventQueue_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return eventQueue_->EndGetEvent(
        pResult,
        ppEvent);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::QueueEvent(
    MediaEventType met,
    REFGUID guidExtendedType,
    HRESULT hrStatus,
    const PROPVARIANT* pvValue)
{
    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (eventQueue_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return eventQueue_->QueueEventParamVar(
        met,
        guidExtendedType,
        hrStatus,
        pvValue);
}

// ========================================================
// IMFMediaStream
// ========================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::GetMediaSource(
    IMFMediaSource** ppMediaSource)
{
    if (ppMediaSource == nullptr)
    {
        return E_POINTER;
    }

    *ppMediaSource = nullptr;

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (source_ == nullptr)
    {
        return E_UNEXPECTED;
    }

    *ppMediaSource = source_;

    source_->AddRef();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::GetStreamDescriptor(
    IMFStreamDescriptor** ppStreamDescriptor)
{
    if (ppStreamDescriptor == nullptr)
    {
        return E_POINTER;
    }

    *ppStreamDescriptor = nullptr;

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (streamDescriptor_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    *ppStreamDescriptor =
        streamDescriptor_;

    streamDescriptor_->AddRef();

    return S_OK;
}

// ========================================================
// IMFMediaStream2
// ========================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::SetStreamState(
    MF_STREAM_STATE state)
{
    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    switch (state)
    {
        case MF_STREAM_STATE_STOPPED:
        case MF_STREAM_STATE_RUNNING:
        case MF_STREAM_STATE_PAUSED:
            break;

        default:
            return E_INVALIDARG;
    }

    std::cout
        << "[LensLinksMediaStream] "
        << "SetStreamState "
        << static_cast<int>(state)
        << std::endl;

    const MF_STREAM_STATE previousState = streamState_;
    streamState_ = state;

    if (eventQueue_ != nullptr)
    {
        if (state == MF_STREAM_STATE_RUNNING)
        {
            const HRESULT qhr = eventQueue_->QueueEventParamVar(
                MEStreamStarted, GUID_NULL, S_OK, nullptr);

            std::cout << "[LensLinksMediaStream] MEStreamStarted queued hr=0x"
                      << std::hex << static_cast<unsigned long>(qhr)
                      << std::dec << std::endl;
        }
        else if (state == MF_STREAM_STATE_STOPPED &&
                 previousState == MF_STREAM_STATE_RUNNING)
        {
            eventQueue_->QueueEventParamVar(
                MEStreamStopped, GUID_NULL, S_OK, nullptr);
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::GetStreamState(
    MF_STREAM_STATE* pState)
{
    if (pState == nullptr)
    {
        return E_POINTER;
    }

    *pState =
        MF_STREAM_STATE_STOPPED;

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    *pState =
        streamState_;

    return S_OK;
}

// ========================================================
// RequestSample
// ========================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaStream::RequestSample(
    IUnknown* pToken)
{
    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (eventQueue_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    if (streamDescriptor_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    // ----------------------------------------------------
    // Make sure stream is running
    // ----------------------------------------------------

    if (streamState_ != MF_STREAM_STATE_RUNNING)
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample ignored: stream not running"
            << std::endl;

        return MF_E_NOT_INITIALIZED;
    }

    // ----------------------------------------------------
    // Obtain camera frame
    // ----------------------------------------------------

    // Pace to about 30 fps. Frame Server requests the next sample as soon
    // as it receives one, so without this we would spin the CPU.
    {
        static ULONGLONG nextTick = 0;
        const ULONGLONG now = GetTickCount64();

        if (nextTick > now)
        {
            Sleep(static_cast<DWORD>(nextTick - now));
        }

        nextTick = (nextTick > now ? nextTick : now) + 33;
    }

    CameraFrame frame;

    if (!VirtualCamera::instance().getFrame(frame))
    {
        // No producer frame (the app is running but no phone is streaming,
        // or the producer stopped). Show one dark-gray "no signal" frame,
        // built once and reused.
        static std::vector<uint8_t> noSignal;

        if (noSignal.empty())
        {
            noSignal.assign(1280u * 720u * 4u, 0x28);

            for (size_t i = 3; i < noSignal.size(); i += 4)
            {
                noSignal[i] = 255;
            }
        }

        frame.width = 1280;
        frame.height = 720;
        frame.pixels = noSignal;
    }

    // ----------------------------------------------------
    // Validate dimensions
    // ----------------------------------------------------

    if (frame.width <= 0 ||
        frame.height <= 0)
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample: invalid dimensions "
            << frame.width
            << "x"
            << frame.height
            << std::endl;

        return MF_E_INVALIDMEDIATYPE;
    }

    if (frame.pixels.empty())
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample: empty frame"
            << std::endl;

        return MF_E_NOT_INITIALIZED;
    }

    // ----------------------------------------------------
    // RGB32
    // ----------------------------------------------------

    constexpr size_t BYTES_PER_PIXEL = 4;

    const size_t width =
        static_cast<size_t>(
            frame.width);

    const size_t height =
        static_cast<size_t>(
            frame.height);

    if (height != 0 &&
        width >
            (std::numeric_limits<size_t>::max)() /
                height)
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample: dimension overflow"
            << std::endl;

        return E_INVALIDARG;
    }

    const size_t pixelCount =
        width * height;

    if (pixelCount >
        (std::numeric_limits<size_t>::max)() /
            BYTES_PER_PIXEL)
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample: pixel buffer overflow"
            << std::endl;

        return E_INVALIDARG;
    }

    const size_t expectedSize =
        pixelCount *
        BYTES_PER_PIXEL;

    if (frame.pixels.size() != expectedSize)
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample: invalid RGB32 buffer size "
            << "expected="
            << expectedSize
            << " actual="
            << frame.pixels.size()
            << std::endl;

        return MF_E_INVALIDMEDIATYPE;
    }

    if (expectedSize >
        static_cast<size_t>(
            (std::numeric_limits<DWORD>::max)()))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample: buffer too large"
            << std::endl;

        return E_INVALIDARG;
    }

    // ----------------------------------------------------
    // Convert to NV12 (BT.709, limited range)
    // ----------------------------------------------------
    //
    // The stream advertises NV12, which every application decodes the
    // same way, instead of RGB32, whose byte order applications disagree on.

    if ((frame.width & 1) != 0 ||
        (frame.height & 1) != 0)
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "RequestSample: NV12 needs even dimensions, got "
            << frame.width
            << "x"
            << frame.height
            << std::endl;

        return MF_E_INVALIDMEDIATYPE;
    }

    static thread_local std::vector<uint8_t> nv12;

    rgbaToNv12(
        frame.pixels.data(),
        frame.width,
        frame.height,
        nv12);

    const DWORD bufferSize =
        static_cast<DWORD>(
            nv12.size());

    // ----------------------------------------------------
    // Create sample
    // ----------------------------------------------------

    IMFSample* sample = nullptr;

    HRESULT hr =
        MFCreateSample(
            &sample);

    if (FAILED(hr))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "MFCreateSample failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return hr;
    }

    // ----------------------------------------------------
    // Create media buffer
    // ----------------------------------------------------

    IMFMediaBuffer* buffer = nullptr;

    hr =
        MFCreateMemoryBuffer(
            bufferSize,
            &buffer);

    if (FAILED(hr))
    {
        sample->Release();

        std::cout
            << "[LensLinksMediaStream] "
            << "MFCreateMemoryBuffer failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return hr;
    }

    // ----------------------------------------------------
    // Copy frame into buffer
    // ----------------------------------------------------

    BYTE* destination = nullptr;
    DWORD maxLength = 0;
    DWORD currentLength = 0;

    hr =
        buffer->Lock(
            &destination,
            &maxLength,
            &currentLength);

    if (SUCCEEDED(hr))
    {
        if (destination == nullptr)
        {
            hr = E_POINTER;
        }
        else if (maxLength < bufferSize)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            std::memcpy(
                destination,
                nv12.data(),
                bufferSize);

            hr =
                buffer->SetCurrentLength(
                    bufferSize);
        }

        buffer->Unlock();
    }

    if (FAILED(hr))
    {
        buffer->Release();
        sample->Release();

        std::cout
            << "[LensLinksMediaStream] "
            << "buffer copy failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return hr;
    }

    // ----------------------------------------------------
    // Attach buffer to sample
    // ----------------------------------------------------

    hr =
        sample->AddBuffer(
            buffer);

    buffer->Release();

    if (FAILED(hr))
    {
        sample->Release();

        std::cout
            << "[LensLinksMediaStream] "
            << "AddBuffer failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return hr;
    }

    // ----------------------------------------------------
    // Preserve request token
    // ----------------------------------------------------

    if (pToken != nullptr)
    {
        hr =
            sample->SetUnknown(
                MFSampleExtension_Token,
                pToken);

        if (FAILED(hr))
        {
            std::cout
                << "[LensLinksMediaStream] "
                << "SetUnknown token failed hr=0x"
                << std::hex
                << static_cast<unsigned long>(hr)
                << std::dec
                << std::endl;

            sample->Release();

            return hr;
        }
    }

    // ----------------------------------------------------
    // Frame timing
    // ----------------------------------------------------

    static LONGLONG frameNumber = 0;

    constexpr LONGLONG FRAME_RATE = 30;

    constexpr LONGLONG FRAME_DURATION =
        10'000'000LL / FRAME_RATE;

    const LONGLONG sampleTime = MFGetSystemTime();

    ++frameNumber;

    hr =
        sample->SetSampleTime(
            sampleTime);

    if (FAILED(hr))
    {
        sample->Release();

        return hr;
    }

    hr =
        sample->SetSampleDuration(
            FRAME_DURATION);

    if (FAILED(hr))
    {
        sample->Release();

        return hr;
    }

    // ----------------------------------------------------
    // Queue MEMediaSample
    // ----------------------------------------------------

    PROPVARIANT value;

    PropVariantInit(
        &value);

    value.vt =
        VT_UNKNOWN;

    value.punkVal =
        sample;

    sample->AddRef();

    hr =
        eventQueue_->QueueEventParamVar(
            MEMediaSample,
            GUID_NULL,
            S_OK,
            &value);

    PropVariantClear(
        &value);

    // Release our original sample reference.
    sample->Release();

    if (FAILED(hr))
    {
        std::cout
            << "[LensLinksMediaStream] "
            << "Queue MEMediaSample failed hr=0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return hr;
    }

    { static unsigned logCount = 0; if ((logCount++ % 150) == 0) { std::cout << "[LensLinksMediaStream] MEMediaSample queued (every 150th) " << frame.width << "x" << frame.height << std::endl; } }

    return S_OK;
}

// ========================================================
// Stream descriptor
// ========================================================

HRESULT
LensLinksMediaStream::setStreamDescriptor(
    IMFStreamDescriptor* descriptor)
{
    if (descriptor == nullptr)
    {
        return E_POINTER;
    }

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    // Release the previous descriptor
    if (streamDescriptor_ != nullptr)
    {
        streamDescriptor_->Release();
        streamDescriptor_ = nullptr;
    }

    // Keep our own reference
    descriptor->AddRef();
    streamDescriptor_ = descriptor;

    IMFMediaTypeHandler* typeHandler = nullptr;

    HRESULT hr =
        streamDescriptor_->GetMediaTypeHandler(
            &typeHandler);

    if (FAILED(hr))
    {
        return hr;
    }

    IMFMediaType* mediaType = nullptr;

    hr =
        typeHandler->GetMediaTypeByIndex(
            0,
            &mediaType);

    if (FAILED(hr))
    {
        typeHandler->Release();
        return hr;
    }

    // Set the current media type
    hr =
        typeHandler->SetCurrentMediaType(
            mediaType);

    mediaType->Release();
    typeHandler->Release();

    if (FAILED(hr))
    {
        return hr;
    }

    // Identify this as a video capture stream
    hr =
        streamDescriptor_->SetGUID(
            MF_DEVICESTREAM_STREAM_CATEGORY,
            PINNAME_VIDEO_CAPTURE);

    if (FAILED(hr))
    {
        return hr;
    }

    // Stream identifier
    hr =
        streamDescriptor_->SetUINT32(
            MF_DEVICESTREAM_STREAM_ID,
            0);

    if (FAILED(hr))
    {
        return hr;
    }

    // Frame Server shared stream
    hr =
        streamDescriptor_->SetUINT32(
            MF_DEVICESTREAM_FRAMESERVER_SHARED,
            1);

    if (FAILED(hr))
    {
        return hr;
    }

    // This stream provides color frames
    hr =
        streamDescriptor_->SetUINT32(
            MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES,
            MFFrameSourceTypes::MFFrameSourceTypes_Color);

    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}
