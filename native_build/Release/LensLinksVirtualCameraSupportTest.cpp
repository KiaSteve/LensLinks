#include <windows.h>
#include <mfapi.h>
#include <mfvirtualcamera.h>

#include <iostream>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mfsensorgroup.lib")

static void PrintHR(
    const char* label,
    HRESULT hr)
{
    std::cout
        << label
        << " = 0x"
        << std::hex
        << std::uppercase
        << static_cast<unsigned long>(hr)
        << std::dec
        << std::endl;
}

int main()
{
    std::cout
        << "========================================\n"
        << " LensLinks Virtual Camera Support Test\n"
        << "========================================\n";

    HRESULT hr =
        CoInitializeEx(
            nullptr,
            COINIT_MULTITHREADED);

    PrintHR(
        "CoInitializeEx",
        hr);

    if (FAILED(hr))
        return 1;

    hr =
        MFStartup(
            MF_VERSION,
            MFSTARTUP_FULL);

    PrintHR(
        "MFStartup",
        hr);

    if (FAILED(hr))
    {
        CoUninitialize();
        return 2;
    }

    std::cout
        << "\n[1] MFIsVirtualCameraTypeSupported\n";

    BOOL supported = FALSE;

    hr =
        MFIsVirtualCameraTypeSupported(
            MFVirtualCameraType_SoftwareCameraSource,
            &supported);

    PrintHR(
        "MFIsVirtualCameraTypeSupported",
        hr);

    std::cout
        << "Software camera source supported = "
        << (supported ? "TRUE" : "FALSE")
        << std::endl;

    std::cout
        << "\n[2] Create virtual camera configuration\n";

    const wchar_t* sourceId =
        L"{8F8B9C41-4A6D-4C7B-912A-7E529D138461}";

    IMFVirtualCamera* camera = nullptr;

    hr =
        MFCreateVirtualCamera(
            MFVirtualCameraType_SoftwareCameraSource,
            MFVirtualCameraLifetime_Session,
            MFVirtualCameraAccess_CurrentUser,
            L"LensLinks Camera Test",
            sourceId,
            nullptr,
            0,
            &camera);

    PrintHR(
        "MFCreateVirtualCamera",
        hr);

    if (FAILED(hr))
    {
        MFShutdown();
        CoUninitialize();
        return 3;
    }

    std::cout
        << "IMFVirtualCamera object created."
        << std::endl;

    std::cout
        << "\n[3] IMFVirtualCamera::Start\n";

    hr =
        camera->Start(nullptr);

    PrintHR(
        "IMFVirtualCamera::Start",
        hr);

    if (SUCCEEDED(hr))
    {
        std::cout
            << "VIRTUAL CAMERA STARTED SUCCESSFULLY."
            << std::endl;

        std::cout
            << "\n[4] Read symbolic link\n";

        WCHAR symbolicLink[2048] = {};

        UINT32 length = 0;

        hr =
            camera->GetString(
                MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK,
                symbolicLink,
                2048,
                &length);

        PrintHR(
            "Get symbolic link",
            hr);

        if (SUCCEEDED(hr))
        {
            std::wcout
                << L"Symbolic link:\n"
                << symbolicLink
                << std::endl;
        }

        std::cout
            << "\nCamera started. Press ENTER to stop."
            << std::endl;

        std::cin.get();

        std::cout
            << "\n[5] Stop"
            << std::endl;

        hr =
            camera->Stop();

        PrintHR(
            "IMFVirtualCamera::Stop",
            hr);
    }
    else
    {
        std::cout
            << "Virtual camera Start FAILED."
            << std::endl;
    }

    std::cout
        << "\n[6] Shutdown"
        << std::endl;

    hr =
        camera->Shutdown();

    PrintHR(
        "IMFVirtualCamera::Shutdown",
        hr);

    camera->Release();

    MFShutdown();
    CoUninitialize();

    std::cout
        << "\n========================================\n"
        << " Test complete\n"
        << "========================================\n";

    return 0;
}