#include "lenslinks_com.h"

#include "lenslinks_media_source.h"
#include "lenslinks_media_source_activate.h"

#include <iostream>
#include <new>

// ------------------------------------------------------------
// LensLinks Media Source CLSID
// ------------------------------------------------------------

const CLSID CLSID_LensLinksMediaSource =
{
    0x8F8B9C41,
    0x4A6D,
    0x4C7B,
    {0x91, 0x2A, 0x7E, 0x52, 0x9D, 0x13, 0x84, 0x61}
};


// ============================================================
// LensLinksClassFactory
// ============================================================

LensLinksClassFactory::LensLinksClassFactory()
{
    std::cout
        << "[LensLinksCOM] ClassFactory created"
        << std::endl;
}

LensLinksClassFactory::~LensLinksClassFactory()
{
    std::cout
        << "[LensLinksCOM] ClassFactory destroyed"
        << std::endl;
}


// ------------------------------------------------------------
// IUnknown
// ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
LensLinksClassFactory::QueryInterface(
    REFIID riid,
    void** ppvObject)
{
    if (ppvObject == nullptr)
        return E_POINTER;

    *ppvObject = nullptr;

    if (riid == IID_IUnknown ||
        riid == IID_IClassFactory)
    {
        *ppvObject =
            static_cast<IClassFactory*>(this);

        AddRef();

        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
LensLinksClassFactory::AddRef()
{
    return static_cast<ULONG>(
        InterlockedIncrement(&refCount_));
}

ULONG STDMETHODCALLTYPE
LensLinksClassFactory::Release()
{
    ULONG count =
        static_cast<ULONG>(
            InterlockedDecrement(&refCount_));

    if (count == 0)
        delete this;

    return count;
}


// ------------------------------------------------------------
// IClassFactory
// ------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
LensLinksClassFactory::CreateInstance(
    IUnknown* pUnkOuter,
    REFIID riid,
    void** ppvObject)
{
    if (ppvObject == nullptr)
        return E_POINTER;

    *ppvObject = nullptr;

    if (pUnkOuter != nullptr)
        return CLASS_E_NOAGGREGATION;

    std::cout
        << "[LensLinksCOM] CreateInstance"
        << std::endl;

    /*
     * IMPORTANT:
     *
     * The virtual-camera pipeline expects the COM object
     * identified by the source CLSID to behave as a proper
     * IMFActivate object.
     *
     * LensLinksMediaSourceActivate already provides:
     *
     *   - IMFActivate
     *   - IMFAttributes
     *   - ActivateObject
     *   - ShutdownObject
     *   - DetachObject
     *
     * Therefore we use it directly instead of the older
     * LensLinksMediaActivator implementation whose
     * IMFAttributes methods returned E_NOTIMPL.
     */

    LensLinksMediaSourceActivate* activator =
        new (std::nothrow)
        LensLinksMediaSourceActivate();

    if (activator == nullptr)
        return E_OUTOFMEMORY;

    HRESULT hr =
        activator->QueryInterface(
            riid,
            ppvObject);

    activator->Release();

    std::cout
        << "[LensLinksCOM] CreateInstance hr=0x"
        << std::hex
        << hr
        << std::dec
        << std::endl;

    return hr;
}

HRESULT STDMETHODCALLTYPE
LensLinksClassFactory::LockServer(
    BOOL)
{
    return S_OK;
}


// ============================================================
// Standard COM DLL entry points
// ============================================================

HRESULT STDMETHODCALLTYPE
DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv)
{
    if (ppv == nullptr)
        return E_POINTER;

    *ppv = nullptr;

    if (rclsid != CLSID_LensLinksMediaSource)
        return CLASS_E_CLASSNOTAVAILABLE;

    std::cout
        << "[LensLinksCOM] DllGetClassObject"
        << std::endl;

    LensLinksClassFactory* factory =
        new (std::nothrow)
        LensLinksClassFactory();

    if (factory == nullptr)
        return E_OUTOFMEMORY;

    HRESULT hr =
        factory->QueryInterface(
            riid,
            ppv);

    factory->Release();

    return hr;
}

HRESULT STDMETHODCALLTYPE
DllCanUnloadNow()
{
    /*
     * Keep the DLL loaded while the virtual camera is using it.
     *
     * We can make this more sophisticated later with a global
     * COM object count, but S_FALSE is safe for this stage.
     */

    return S_FALSE;
}