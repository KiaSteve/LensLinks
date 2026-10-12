// lenslinks_register.cpp
//
// Self-registration so that "regsvr32 LensLinksNative.dll" writes the COM
// keys the Windows virtual camera (Frame Server) needs. Must run elevated,
// because the keys go under HKLM: Frame Server runs as LocalService and
// cannot see per-user (HKCU) registrations.

#include <windows.h>

#include <string>

namespace
{
    constexpr wchar_t kClsid[] = L"{8F8B9C41-4A6D-4C7B-912A-7E529D138461}";

    std::wstring clsidKeyPath()
    {
        return std::wstring(L"SOFTWARE\\Classes\\CLSID\\") + kClsid;
    }

    // Full path of the DLL this code lives in.
    HRESULT getModulePath(std::wstring& path)
    {
        HMODULE module = nullptr;

        if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&getModulePath),
                &module))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        wchar_t buffer[MAX_PATH * 2];

        const DWORD length = GetModuleFileNameW(
            module,
            buffer,
            static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])));

        if (length == 0)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (length >= sizeof(buffer) / sizeof(buffer[0]))
        {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        path.assign(buffer, length);

        return S_OK;
    }

    HRESULT setString(HKEY key, const wchar_t* name, const std::wstring& value)
    {
        const LONG result = RegSetValueExW(
            key,
            name,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));

        return HRESULT_FROM_WIN32(result);
    }
}

STDAPI DllRegisterServer()
{
    std::wstring path;

    HRESULT hr = getModulePath(path);

    if (FAILED(hr))
    {
        return hr;
    }

    HKEY clsidKey = nullptr;

    LONG result = RegCreateKeyExW(
        HKEY_LOCAL_MACHINE,
        clsidKeyPath().c_str(),
        0,
        nullptr,
        0,
        KEY_WRITE | KEY_WOW64_64KEY,
        nullptr,
        &clsidKey,
        nullptr);

    if (result != ERROR_SUCCESS)
    {
        // ERROR_ACCESS_DENIED (5) means this is not an elevated prompt.
        return HRESULT_FROM_WIN32(result);
    }

    hr = setString(clsidKey, nullptr, L"LensLinks Media Source");

    if (SUCCEEDED(hr))
    {
        HKEY serverKey = nullptr;

        result = RegCreateKeyExW(
            clsidKey,
            L"InprocServer32",
            0,
            nullptr,
            0,
            KEY_WRITE | KEY_WOW64_64KEY,
            nullptr,
            &serverKey,
            nullptr);

        if (result != ERROR_SUCCESS)
        {
            hr = HRESULT_FROM_WIN32(result);
        }
        else
        {
            hr = setString(serverKey, nullptr, path);

            if (SUCCEEDED(hr))
            {
                hr = setString(serverKey, L"ThreadingModel", L"Both");
            }

            RegCloseKey(serverKey);
        }
    }

    RegCloseKey(clsidKey);

    return hr;
}

STDAPI DllUnregisterServer()
{
    const LONG result = RegDeleteTreeW(
        HKEY_LOCAL_MACHINE,
        clsidKeyPath().c_str());

    if (result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND)
    {
        return S_OK;
    }

    return HRESULT_FROM_WIN32(result);
}
