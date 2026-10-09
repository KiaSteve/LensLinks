#pragma once

#include <windows.h>
#include <unknwn.h>
#include <objbase.h>
#include <mfidl.h>

extern const CLSID CLSID_LensLinksMediaSource;


// ============================================================
// LensLinksClassFactory
// ============================================================

class LensLinksClassFactory : public IClassFactory
{
public:

    LensLinksClassFactory();

    // --------------------------------------------------------
    // IUnknown
    // --------------------------------------------------------

    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        void** ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;


    // --------------------------------------------------------
    // IClassFactory
    // --------------------------------------------------------

    HRESULT STDMETHODCALLTYPE CreateInstance(
        IUnknown* pUnkOuter,
        REFIID riid,
        void** ppvObject) override;

    HRESULT STDMETHODCALLTYPE LockServer(
        BOOL fLock) override;


private:

    ~LensLinksClassFactory();

    LONG refCount_ = 1;
};


// ============================================================
// Standard COM DLL entry points
// ============================================================

HRESULT STDMETHODCALLTYPE DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID* ppv);

HRESULT STDMETHODCALLTYPE DllCanUnloadNow();