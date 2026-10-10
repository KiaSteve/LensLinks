#include <iostream>
#include "lenslinks_media_source.h"
#include "lenslinks_media_stream.h"
#include <string>
static std::string ll_narrow(const wchar_t* w) { std::string s; for (; w && *w; ++w) s.push_back(static_cast<char>(*w)); return s; }

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>


LensLinksMediaSource::LensLinksMediaSource()
{
    std::cout
        << "[LensLinksMediaSource] created"
        << std::endl;

    HRESULT hr = MFCreateAttributes(
        &sourceAttributes_,
        8);

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "MFCreateAttributes failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    hr = MFCreateEventQueue(
        &eventQueue_);

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "MFCreateEventQueue failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    stream_ = new LensLinksMediaStream(this);

    IMFMediaType* mediaType = nullptr;

    hr = MFCreateMediaType(
        &mediaType);

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "MFCreateMediaType failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    hr = mediaType->SetGUID(
        MF_MT_MAJOR_TYPE,
        MFMediaType_Video);

    if (FAILED(hr))
    {
        mediaType->Release();
        return;
    }

    hr = mediaType->SetGUID(
        MF_MT_SUBTYPE,
        MFVideoFormat_RGB32);

    if (FAILED(hr))
    {
        mediaType->Release();
        return;
    }

    hr = MFSetAttributeSize(
        mediaType,
        MF_MT_FRAME_SIZE,
        1280,
        720);

    if (FAILED(hr))
    {
        mediaType->Release();
        return;
    }

    hr = MFSetAttributeRatio(
        mediaType,
        MF_MT_FRAME_RATE,
        30,
        1);

    if (FAILED(hr))
    {
        mediaType->Release();
        return;
    }

    hr = MFSetAttributeRatio(
        mediaType,
        MF_MT_PIXEL_ASPECT_RATIO,
        1,
        1);

    if (FAILED(hr))
    {
        mediaType->Release();
        return;
    }

    hr = mediaType->SetUINT32(
        MF_MT_INTERLACE_MODE,
        MFVideoInterlace_Progressive);

    if (FAILED(hr))
    {
        mediaType->Release();
        return;
    }

    mediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, 1280 * 4);
    mediaType->SetUINT32(MF_MT_SAMPLE_SIZE, 1280 * 720 * 4);
    mediaType->SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, 1);
    mediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, 1);

    IMFMediaType* mediaTypes[1] =
    {
        mediaType
    };

    IMFStreamDescriptor* streamDescriptor = nullptr;

    hr = MFCreateStreamDescriptor(
        0,
        1,
        mediaTypes,
        &streamDescriptor);

    mediaType->Release();

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "MFCreateStreamDescriptor failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    IMFMediaTypeHandler* mediaTypeHandler = nullptr;

    hr = streamDescriptor->GetMediaTypeHandler(
        &mediaTypeHandler);

    if (SUCCEEDED(hr))
    {
        IMFMediaType* currentType = nullptr;

        hr = mediaTypeHandler->GetMediaTypeByIndex(
            0,
            &currentType);

        if (SUCCEEDED(hr))
        {
            hr = mediaTypeHandler->SetCurrentMediaType(
                currentType);

            currentType->Release();
        }

        mediaTypeHandler->Release();
    }

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "media type handler setup failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        streamDescriptor->Release();
        return;
    }

    hr = stream_->setStreamDescriptor(
        streamDescriptor);

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "setStreamDescriptor failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        streamDescriptor->Release();
        return;
    }

    hr = MFCreatePresentationDescriptor(
        1,
        &streamDescriptor,
        &presentationDescriptor_);

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "MFCreatePresentationDescriptor failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        streamDescriptor->Release();
        return;
    }

    streamDescriptor->Release();

    hr = presentationDescriptor_->SelectStream(0);

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksMediaSource] "
            << "SelectStream failed: 0x"
            << std::hex
            << static_cast<unsigned long>(hr)
            << std::dec
            << std::endl;

        return;
    }

    std::cout
        << "[LensLinksMediaSource] "
        << "video stream initialized "
        << "1280x720 @ 30fps RGB32"
        << std::endl;
}

LensLinksMediaSource::~LensLinksMediaSource()
{
    if (presentationDescriptor_ != nullptr)
    {
        presentationDescriptor_->Release();
        presentationDescriptor_ = nullptr;
    }

    if (stream_ != nullptr)
    {
        stream_->Release();
        stream_ = nullptr;
    }

    if (eventQueue_ != nullptr)
    {
        eventQueue_->Release();
        eventQueue_ = nullptr;
    }

    if (sourceAttributes_ != nullptr)
    {
        sourceAttributes_->Release();
        sourceAttributes_ = nullptr;
    }

    std::cout
        << "[LensLinksMediaSource] destroyed"
        << std::endl;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::QueryInterface(
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
                static_cast<IMFMediaSource*>(this));
    }
    else if (riid == IID_IMFMediaEventGenerator)
    {
        *ppvObject =
            static_cast<IMFMediaEventGenerator*>(
                static_cast<IMFMediaSource*>(this));
    }
    else if (riid == IID_IMFMediaSource)
    {
        *ppvObject =
            static_cast<IMFMediaSource*>(this);
    }
    else if (riid == IID_IMFMediaSourceEx)
    {
        *ppvObject =
            static_cast<IMFMediaSourceEx*>(this);
    }
    else if (riid == IID_IMFGetService)
    {
        *ppvObject =
            static_cast<IMFGetService*>(this);
    }
    else if (riid == __uuidof(IKsControl))
    {
        *ppvObject = static_cast<IKsControl*>(this);
    }
    else if (riid == __uuidof(ILensLinksMediaSourceTest))
    {
        *ppvObject =
            static_cast<ILensLinksMediaSourceTest*>(this);
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
LensLinksMediaSource::AddRef()
{
    return static_cast<ULONG>(
        InterlockedIncrement(&refCount_));
}

ULONG STDMETHODCALLTYPE
LensLinksMediaSource::Release()
{
    ULONG count =
        static_cast<ULONG>(
            InterlockedDecrement(&refCount_));

    if (count == 0)
    {
        delete this;
    }

    return count;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::GetEvent(
    DWORD dwFlags,
    IMFMediaEvent** ppEvent)
{
    if (eventQueue_ == nullptr)
    {
        return MF_E_SHUTDOWN;
    }

    return eventQueue_->GetEvent(
        dwFlags,
        ppEvent);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::BeginGetEvent(
    IMFAsyncCallback* pCallback,
    IUnknown* punkState)
{
    if (eventQueue_ == nullptr)
    {
        return MF_E_SHUTDOWN;
    }

    return eventQueue_->BeginGetEvent(
        pCallback,
        punkState);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::EndGetEvent(
    IMFAsyncResult* pResult,
    IMFMediaEvent** ppEvent)
{
    if (eventQueue_ == nullptr)
    {
        return MF_E_SHUTDOWN;
    }

    return eventQueue_->EndGetEvent(
        pResult,
        ppEvent);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::QueueEvent(
    MediaEventType met,
    REFGUID guidExtendedType,
    HRESULT hrStatus,
    const PROPVARIANT* pvValue)
{
    if (eventQueue_ == nullptr)
    {
        return MF_E_SHUTDOWN;
    }

    return eventQueue_->QueueEventParamVar(
        met,
        guidExtendedType,
        hrStatus,
        pvValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::GetCharacteristics(
    DWORD* pdwCharacteristics)
{
    if (pdwCharacteristics == nullptr)
    {
        return E_POINTER;
    }

    *pdwCharacteristics =
        MFMEDIASOURCE_IS_LIVE;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::CreatePresentationDescriptor(
    IMFPresentationDescriptor** ppPresentationDescriptor)
{
    if (ppPresentationDescriptor == nullptr)
    {
        return E_POINTER;
    }

    *ppPresentationDescriptor = nullptr;

    if (presentationDescriptor_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return presentationDescriptor_->Clone(
        ppPresentationDescriptor);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::Start(
    IMFPresentationDescriptor* pPresentationDescriptor,
    const GUID* pguidTimeFormat,
    const PROPVARIANT* pvarStartPosition)
{
    UNREFERENCED_PARAMETER(pguidTimeFormat);
    UNREFERENCED_PARAMETER(pvarStartPosition);

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (pPresentationDescriptor == nullptr)
    {
        return E_POINTER;
    }

    started_ = true;

    if (stream_ != nullptr)
    {
        stream_->SetStreamState(
            MF_STREAM_STATE_RUNNING);
    }

    HRESULT hr =
        eventQueue_->QueueEventParamUnk(
            MENewStream,
            GUID_NULL,
            S_OK,
            stream_);

    if (FAILED(hr))
    {
        return hr;
    }

    return eventQueue_->QueueEventParamVar(
        MESourceStarted,
        GUID_NULL,
        S_OK,
        nullptr);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::Stop()
{
    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    started_ = false;

    if (stream_ != nullptr)
    {
        stream_->SetStreamState(
            MF_STREAM_STATE_STOPPED);
    }

    return eventQueue_->QueueEventParamVar(
        MESourceStopped,
        GUID_NULL,
        S_OK,
        nullptr);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::Pause()
{
    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    return MF_E_INVALID_STATE_TRANSITION;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::Shutdown()
{
    if (shutdown_)
    {
        return S_OK;
    }

    shutdown_ = true;
    started_ = false;

    if (stream_ != nullptr)
    {
        stream_->SetStreamState(
            MF_STREAM_STATE_STOPPED);
    }

    if (eventQueue_ != nullptr)
    {
        eventQueue_->Shutdown();
        eventQueue_->Release();
        eventQueue_ = nullptr;
    }

    std::cout
        << "[LensLinksMediaSource] Shutdown"
        << std::endl;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::GetSourceAttributes(
    IMFAttributes** ppAttributes)
{
    if (ppAttributes == nullptr)
    {
        return E_POINTER;
    }

    *ppAttributes = nullptr;

    if (sourceAttributes_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    sourceAttributes_->AddRef();

    *ppAttributes = sourceAttributes_;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::GetStreamAttributes(
    DWORD dwStreamIdentifier,
    IMFAttributes** ppAttributes)
{
    if (ppAttributes == nullptr)
    {
        return E_POINTER;
    }

    *ppAttributes = nullptr;

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (stream_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    DWORD streamId = 0;

    IMFStreamDescriptor* descriptor = nullptr;

    HRESULT hr =
        stream_->GetStreamDescriptor(
            &descriptor);

    if (FAILED(hr))
    {
        return hr;
    }

    hr =
        descriptor->GetStreamIdentifier(
            &streamId);

    descriptor->Release();

    if (FAILED(hr))
    {
        return hr;
    }

    if (dwStreamIdentifier != streamId)
    {
        return MF_E_NOT_FOUND;
    }

    IMFAttributes* attributes =
        stream_->getStreamAttributes();

    if (attributes == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    attributes->AddRef();

    *ppAttributes = attributes;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::SetD3DManager(
    IUnknown* pManager)
{
    UNREFERENCED_PARAMETER(pManager);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::GetService(
    REFGUID guidService,
    REFIID riid,
    LPVOID* ppvObject)
{
    UNREFERENCED_PARAMETER(guidService);
    UNREFERENCED_PARAMETER(riid);

    if (ppvObject == nullptr)
    {
        return E_POINTER;
    }

    *ppvObject = nullptr;

    return MF_E_UNSUPPORTED_SERVICE;
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::GetLensLinksStream(
    IMFMediaStream** ppStream)
{
    if (ppStream == nullptr)
    {
        return E_POINTER;
    }

    *ppStream = nullptr;

    if (stream_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    return stream_->QueryInterface(
        IID_PPV_ARGS(ppStream));
}

// ============================================================
// IKsControl
// ============================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::KsProperty(
    PKSPROPERTY pProperty,
    ULONG ulPropertyLength,
    LPVOID pPropertyData,
    ULONG ulDataLength,
    ULONG* pBytesReturned)
{
    UNREFERENCED_PARAMETER(pProperty);
    UNREFERENCED_PARAMETER(ulPropertyLength);
    UNREFERENCED_PARAMETER(pPropertyData);
    UNREFERENCED_PARAMETER(ulDataLength);
    UNREFERENCED_PARAMETER(pBytesReturned);

    // LensLinks currently exposes no camera controls.
    return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
}


HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::KsMethod(
    PKSMETHOD pMethod,
    ULONG ulMethodLength,
    LPVOID pMethodData,
    ULONG ulDataLength,
    ULONG* pBytesReturned)
{
    UNREFERENCED_PARAMETER(pMethod);
    UNREFERENCED_PARAMETER(ulMethodLength);
    UNREFERENCED_PARAMETER(pMethodData);
    UNREFERENCED_PARAMETER(ulDataLength);
    UNREFERENCED_PARAMETER(pBytesReturned);

    // LensLinks currently exposes no KS methods.
    return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
}


HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::KsEvent(
    PKSEVENT pEvent,
    ULONG ulEventLength,
    LPVOID pEventData,
    ULONG ulDataLength,
    ULONG* pBytesReturned)
{
    UNREFERENCED_PARAMETER(pEvent);
    UNREFERENCED_PARAMETER(ulEventLength);
    UNREFERENCED_PARAMETER(pEventData);
    UNREFERENCED_PARAMETER(ulDataLength);
    UNREFERENCED_PARAMETER(pBytesReturned);

    // LensLinks currently exposes no KS events.
    return HRESULT_FROM_WIN32(ERROR_SET_NOT_FOUND);
}

// ============================================================
// IMFSampleAllocatorControl
// ============================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::SetDefaultAllocator(
    DWORD dwOutputStreamID,
    IUnknown* pAllocator)
{
    UNREFERENCED_PARAMETER(pAllocator);

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (stream_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    IMFStreamDescriptor* descriptor = nullptr;

    HRESULT hr =
        stream_->GetStreamDescriptor(
            &descriptor);

    if (FAILED(hr))
    {
        return hr;
    }

    DWORD streamId = 0;

    hr =
        descriptor->GetStreamIdentifier(
            &streamId);

    descriptor->Release();

    if (FAILED(hr))
    {
        return hr;
    }

    if (dwOutputStreamID != streamId)
    {
        return MF_E_NOT_FOUND;
    }

    // LensLinks currently manages its own sample allocation.
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
LensLinksMediaSource::GetAllocatorUsage(
    DWORD dwOutputStreamID,
    DWORD* pdwInputStreamID,
    MFSampleAllocatorUsage* peUsage)
{
    if (pdwInputStreamID == nullptr ||
        peUsage == nullptr)
    {
        return E_POINTER;
    }

    *pdwInputStreamID = 0;
    *peUsage =
        MFSampleAllocatorUsage_UsesProvidedAllocator;

    if (shutdown_)
    {
        return MF_E_SHUTDOWN;
    }

    if (stream_ == nullptr)
    {
        return MF_E_NOT_INITIALIZED;
    }

    IMFStreamDescriptor* descriptor = nullptr;

    HRESULT hr =
        stream_->GetStreamDescriptor(
            &descriptor);

    if (FAILED(hr))
    {
        return hr;
    }

    DWORD streamId = 0;

    hr =
        descriptor->GetStreamIdentifier(
            &streamId);

    descriptor->Release();

    if (FAILED(hr))
    {
        return hr;
    }

    if (dwOutputStreamID != streamId)
    {
        return MF_E_NOT_FOUND;
    }

    *pdwInputStreamID = streamId;

    return S_OK;
}

// ============================================================
// IMFSampleAllocatorControl
// ============================================================


