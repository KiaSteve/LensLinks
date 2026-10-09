#include "lenslinks_media_source_activate.h"

#include "lenslinks_media_source.h"

#include <iostream>
#include <new>

// ============================================================
// Constructor
// ============================================================

LensLinksMediaSourceActivate::LensLinksMediaSourceActivate()
{
    std::cout
        << "[LensLinksActivate] created"
        << std::endl;

    HRESULT hr = MFCreateAttributes(
        &attributes_,
        8);

    if (FAILED(hr))
    {
        std::cerr
            << "[LensLinksActivate] MFCreateAttributes failed: 0x"
            << std::hex
            << hr
            << std::dec
            << std::endl;

        attributes_ = nullptr;
    }
}

// ============================================================
// Destructor
// ============================================================

LensLinksMediaSourceActivate::~LensLinksMediaSourceActivate()
{
    if (source_ != nullptr)
    {
        source_->Release();
        source_ = nullptr;
    }

    if (attributes_ != nullptr)
    {
        attributes_->Release();
        attributes_ = nullptr;
    }

    std::cout
        << "[LensLinksActivate] destroyed"
        << std::endl;
}

// ============================================================
// IUnknown
// ============================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::QueryInterface(
    REFIID riid,
    void** ppvObject)
{
    if (ppvObject == nullptr)
        return E_POINTER;

    *ppvObject = nullptr;

    if (riid == IID_IUnknown ||
        riid == IID_IMFActivate ||
        riid == IID_IMFAttributes)
    {
        *ppvObject =
            static_cast<IMFActivate*>(this);

        AddRef();

        std::cout
            << "[LensLinksActivate] QueryInterface SUCCESS"
            << std::endl;

        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
LensLinksMediaSourceActivate::AddRef()
{
    return static_cast<ULONG>(
        InterlockedIncrement(&refCount_));
}

ULONG STDMETHODCALLTYPE
LensLinksMediaSourceActivate::Release()
{
    ULONG count =
        static_cast<ULONG>(
            InterlockedDecrement(&refCount_));

    if (count == 0)
        delete this;

    return count;
}

// ============================================================
// IMFAttributes
//
// All IMFAttributes operations are delegated to attributes_.
// ============================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetItem(
    REFGUID guidKey,
    PROPVARIANT* pValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetItem(
        guidKey,
        pValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetItemType(
    REFGUID guidKey,
    MF_ATTRIBUTE_TYPE* pType)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetItemType(
        guidKey,
        pType);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::CompareItem(
    REFGUID guidKey,
    REFPROPVARIANT Value,
    BOOL* pbResult)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->CompareItem(
        guidKey,
        Value,
        pbResult);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::Compare(
    IMFAttributes* pTheirs,
    MF_ATTRIBUTES_MATCH_TYPE MatchType,
    BOOL* pbResult)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->Compare(
        pTheirs,
        MatchType,
        pbResult);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetUINT32(
    REFGUID guidKey,
    UINT32* punValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetUINT32(
        guidKey,
        punValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetUINT64(
    REFGUID guidKey,
    UINT64* punValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetUINT64(
        guidKey,
        punValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetDouble(
    REFGUID guidKey,
    double* pfValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetDouble(
        guidKey,
        pfValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetGUID(
    REFGUID guidKey,
    GUID* pguidValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetGUID(
        guidKey,
        pguidValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetStringLength(
    REFGUID guidKey,
    UINT32* pcchLength)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetStringLength(
        guidKey,
        pcchLength);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetString(
    REFGUID guidKey,
    LPWSTR pwszValue,
    UINT32 cchBufSize,
    UINT32* pcchLength)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetString(
        guidKey,
        pwszValue,
        cchBufSize,
        pcchLength);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetAllocatedString(
    REFGUID guidKey,
    LPWSTR* ppszValue,
    UINT32* pcchLength)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetAllocatedString(
        guidKey,
        ppszValue,
        pcchLength);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetBlobSize(
    REFGUID guidKey,
    UINT32* pcbBlobSize)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetBlobSize(
        guidKey,
        pcbBlobSize);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetBlob(
    REFGUID guidKey,
    UINT8* pBuf,
    UINT32 cbBufSize,
    UINT32* pcbBlobSize)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetBlob(
        guidKey,
        pBuf,
        cbBufSize,
        pcbBlobSize);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetAllocatedBlob(
    REFGUID guidKey,
    UINT8** ppBuf,
    UINT32* pcbSize)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetAllocatedBlob(
        guidKey,
        ppBuf,
        pcbSize);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetUnknown(
    REFGUID guidKey,
    REFIID riid,
    LPVOID* ppv)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetUnknown(
        guidKey,
        riid,
        ppv);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetItem(
    REFGUID guidKey,
    REFPROPVARIANT Value)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetItem(
        guidKey,
        Value);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::DeleteItem(
    REFGUID guidKey)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->DeleteItem(
        guidKey);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::DeleteAllItems()
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->DeleteAllItems();
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetUINT32(
    REFGUID guidKey,
    UINT32 unValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetUINT32(
        guidKey,
        unValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetUINT64(
    REFGUID guidKey,
    UINT64 unValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetUINT64(
        guidKey,
        unValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetDouble(
    REFGUID guidKey,
    double fValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetDouble(
        guidKey,
        fValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetGUID(
    REFGUID guidKey,
    REFGUID guidValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetGUID(
        guidKey,
        guidValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetString(
    REFGUID guidKey,
    LPCWSTR wszValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetString(
        guidKey,
        wszValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetBlob(
    REFGUID guidKey,
    const UINT8* pBuf,
    UINT32 cbBufSize)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetBlob(
        guidKey,
        pBuf,
        cbBufSize);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::SetUnknown(
    REFGUID guidKey,
    IUnknown* pUnknown)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->SetUnknown(
        guidKey,
        pUnknown);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::LockStore()
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->LockStore();
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::UnlockStore()
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->UnlockStore();
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetCount(
    UINT32* pcItems)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetCount(
        pcItems);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::GetItemByIndex(
    UINT32 unIndex,
    GUID* pguidKey,
    PROPVARIANT* pValue)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->GetItemByIndex(
        unIndex,
        pguidKey,
        pValue);
}

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::CopyAllItems(
    IMFAttributes* pDest)
{
    if (attributes_ == nullptr)
        return E_UNEXPECTED;

    return attributes_->CopyAllItems(
        pDest);
}

// ============================================================
// IMFActivate
// ============================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::ActivateObject(
    REFIID riid,
    void** ppv)
{
    if (ppv == nullptr)
        return E_POINTER;

    *ppv = nullptr;

    if (shutdown_)
        return MF_E_SHUTDOWN;

    std::cout
        << "[LensLinksActivate] ActivateObject"
        << std::endl;

    // --------------------------------------------------------
    // If the source was already activated, return it.
    // --------------------------------------------------------

    if (source_ != nullptr)
    {
        HRESULT hr =
            source_->QueryInterface(
                riid,
                ppv);

        std::cout
            << "[LensLinksActivate] ActivateObject existing "
            << "source hr=0x"
            << std::hex
            << hr
            << std::dec
            << std::endl;

        return hr;
    }

    // --------------------------------------------------------
    // Create the actual Media Foundation media source.
    // --------------------------------------------------------

    LensLinksMediaSource* newSource =
        new (std::nothrow)
        LensLinksMediaSource();

    if (newSource == nullptr)
        return E_OUTOFMEMORY;

        // --------------------------------------------------------
// Copy all attributes supplied through IMFActivate into
// the media source's source attribute store.
//
// Frame Server requires this when IMFActivate and
// IMFMediaSource are separate COM objects.
// --------------------------------------------------------

IMFAttributes* sourceAttributes = nullptr;

HRESULT hr =
    newSource->GetSourceAttributes(
        &sourceAttributes);

if (FAILED(hr))
{
    newSource->Release();
return hr;
}

hr =
    attributes_->CopyAllItems(
        sourceAttributes);

sourceAttributes->Release();

if (FAILED(hr))
{
    newSource->Release();
    return hr;
}

    // --------------------------------------------------------
    // Keep the source's initial COM reference.
    //
    // newSource starts with refCount = 1.
    // source_ becomes the owner of that reference.
    // --------------------------------------------------------

    source_ = newSource;

    // --------------------------------------------------------
    // Give the caller its own COM reference.
    // QueryInterface() performs AddRef().
    // --------------------------------------------------------

    hr =
    source_->QueryInterface(
        riid,
        ppv);

    // --------------------------------------------------------
    // If QueryInterface failed, release our internal reference.
    // --------------------------------------------------------

    if (FAILED(hr))
    {
        source_->Release();
        source_ = nullptr;
    }

    std::cout
        << "[LensLinksActivate] ActivateObject hr=0x"
        << std::hex
        << hr
        << std::dec
        << std::endl;

    return hr;
}

// ============================================================
// ShutdownObject
// ============================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::ShutdownObject()
{
    return E_NOTIMPL;
}
// ============================================================
// DetachObject
// ============================================================

HRESULT STDMETHODCALLTYPE
LensLinksMediaSourceActivate::DetachObject()
{
    return E_NOTIMPL;
}