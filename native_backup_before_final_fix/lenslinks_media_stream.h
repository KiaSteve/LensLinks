#pragma once

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>

class LensLinksMediaSource;

class LensLinksMediaStream final :
    public IMFMediaStream2
{
public:
    explicit LensLinksMediaStream(
        LensLinksMediaSource* source);

    HRESULT setStreamDescriptor(
        IMFStreamDescriptor* descriptor);

    HRESULT getAttributes(
        IMFAttributes** ppAttributes);

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void** ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

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

    HRESULT STDMETHODCALLTYPE GetMediaSource(
        IMFMediaSource** ppMediaSource) override;

    HRESULT STDMETHODCALLTYPE GetStreamDescriptor(
        IMFStreamDescriptor** ppStreamDescriptor) override;

    HRESULT STDMETHODCALLTYPE RequestSample(
        IUnknown* pToken) override;

    HRESULT STDMETHODCALLTYPE SetStreamState(
        MF_STREAM_STATE state) override;

    HRESULT STDMETHODCALLTYPE GetStreamState(
        MF_STREAM_STATE* pState) override;

private:
    ~LensLinksMediaStream();

    LensLinksMediaStream(
        const LensLinksMediaStream&) = delete;

    LensLinksMediaStream& operator=(
        const LensLinksMediaStream&) = delete;

    LONG refCount_ = 1;

    LensLinksMediaSource* source_ = nullptr;

    IMFStreamDescriptor* streamDescriptor_ = nullptr;

    IMFAttributes* streamAttributes_ = nullptr;

    IMFMediaEventQueue* eventQueue_ = nullptr;

    MF_STREAM_STATE streamState_ =
        MF_STREAM_STATE_STOPPED;

    bool shutdown_ = false;
};