#pragma once

#include <windows.h>
#include <mfidl.h>

// {A7D2D9B1-6C44-4D4F-9E6B-52C7B3D84110}
struct __declspec(uuid("A7D2D9B1-6C44-4D4F-9E6B-52C7B3D84110"))
ILensLinksMediaSourceTest : public IUnknown
{
    virtual HRESULT STDMETHODCALLTYPE GetLensLinksStream(
        IMFMediaStream** ppStream) = 0;
};