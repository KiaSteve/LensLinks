#pragma once

#include <windows.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <ks.h>
#include <ksproxy.h>

class LensLinksMediaStream;

// ============================================================
// LensLinks test interface
// ============================================================

// {A7D2D9B1-6C44-4D4F-9E6B-52C7B3D84110}
struct __declspec(uuid("A7D2D9B1-6C44-4D4F-9E6B-52C7B3D84110"))
ILensLinksMediaSourceTest : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetLensLinksStream(
        IMFMediaStream** ppStream) = 0;
};

// ============================================================
// LensLinksMediaSource
// ============================================================

class LensLinksMediaSource final :
    public IMFMediaSourceEx,
    public IMFGetService,
    public IKsControl,
    public IMFSampleAllocatorControl,
    public ILensLinksMediaSourceTest
{
public:

    LensLinksMediaSource();

    // ========================================================
    // IUnknown
    // ========================================================

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void** ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    // ========================================================
    // IMFMediaEventGenerator
    // ========================================================

    HRESULT STDMETHODCALLTYPE GetEvent(
        DWORD dwFlags,
        IMFMediaEvent** ppEvent) override;

    HRESULT STDMETHODCALLTYPE BeginGetEvent(
        IMFAsyncCallback* pCallback,
        IUnknown* punkState) override;

    HRESULT STDMETHODCALLTYPE EndGetEvent(
        IMFAsyncResult* pResult,
        IMFMediaEvent** ppEvent) override;

    HRESULT STDMETHODCALLTYPE QueueEvent(
        MediaEventType met,
        REFGUID guidExtendedType,
        HRESULT hrStatus,
        const PROPVARIANT* pvValue) override;

    // ========================================================
    // IMFMediaSource
    // ========================================================

    HRESULT STDMETHODCALLTYPE GetCharacteristics(
        DWORD* pdwCharacteristics) override;

    HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(
        IMFPresentationDescriptor** ppPresentationDescriptor) override;

    HRESULT STDMETHODCALLTYPE Start(
        IMFPresentationDescriptor* pPresentationDescriptor,
        const GUID* pguidTimeFormat,
        const PROPVARIANT* pvarStartPosition) override;

    HRESULT STDMETHODCALLTYPE Stop() override;

    HRESULT STDMETHODCALLTYPE Pause() override;

    HRESULT STDMETHODCALLTYPE Shutdown() override;

    // ========================================================
    // IMFMediaSourceEx
    // ========================================================

    HRESULT STDMETHODCALLTYPE GetSourceAttributes(
        IMFAttributes** ppAttributes) override;

    HRESULT STDMETHODCALLTYPE GetStreamAttributes(
        DWORD dwStreamIdentifier,
        IMFAttributes** ppAttributes) override;

    HRESULT STDMETHODCALLTYPE SetD3DManager(
        IUnknown* pManager) override;

    // ========================================================
    // IMFGetService
    // ========================================================

    HRESULT STDMETHODCALLTYPE GetService(
        REFGUID guidService,
        REFIID riid,
        LPVOID* ppvObject) override;


// ============================================================
// IKsControl
// ============================================================


// ============================================================
// IKsControl
// ============================================================

HRESULT STDMETHODCALLTYPE KsProperty(
    PKSPROPERTY pProperty,
    ULONG ulPropertyLength,
    LPVOID pPropertyData,
    ULONG ulDataLength,
    ULONG* pBytesReturned) override;

HRESULT STDMETHODCALLTYPE KsMethod(
    PKSMETHOD pMethod,
    ULONG ulMethodLength,
    LPVOID pMethodData,
    ULONG ulDataLength,
    ULONG* pBytesReturned) override;

HRESULT STDMETHODCALLTYPE KsEvent(
    PKSEVENT pEvent,
    ULONG ulEventLength,
    LPVOID pEventData,
    ULONG ulDataLength,
    ULONG* pBytesReturned) override;

    // ============================================================
// IMFSampleAllocatorControl
// ============================================================

HRESULT STDMETHODCALLTYPE SetDefaultAllocator(
    DWORD dwOutputStreamID,
    IUnknown* pAllocator) override;

HRESULT STDMETHODCALLTYPE GetAllocatorUsage(
    DWORD dwOutputStreamID,
    DWORD* pdwInputStreamID,
    MFSampleAllocatorUsage* peUsage) override;


// ============================================================
// Test interface
// ============================================================

    // ========================================================
    // Test interface
    // ========================================================

    HRESULT STDMETHODCALLTYPE GetLensLinksStream(
        IMFMediaStream** ppStream) override;

    // ========================================================
    // Helpers
    // ========================================================

    LensLinksMediaStream* GetStream() const
    {
        return stream_;
    }

private:

    ~LensLinksMediaSource();

    LONG refCount_ = 1;

    bool shutdown_ = false;
    bool started_ = false;

    LensLinksMediaStream* stream_ = nullptr;

    IMFPresentationDescriptor* presentationDescriptor_ =
        nullptr;

    IMFAttributes* sourceAttributes_ =
        nullptr;

    IMFMediaEventQueue* eventQueue_ =
        nullptr;
};