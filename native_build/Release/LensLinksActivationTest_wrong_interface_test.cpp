#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <iostream>
#include <iomanip>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")

const CLSID CLSID_LensLinksMediaSource =
{
    0x8F8B9C41,
    0x4A6D,
    0x4C7B,
    {0x91, 0x2A, 0x7E, 0x52, 0x9D, 0x13, 0x84, 0x61}
};

static void PrintHR(const char* label, HRESULT hr)
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
    std::cout << "========================================\n";
    std::cout << " LensLinks COM Activation Test\n";
    std::cout << "========================================\n";

    HRESULT hr = CoInitializeEx(
        nullptr,
        COINIT_MULTITHREADED);

    PrintHR("CoInitializeEx", hr);

    if (FAILED(hr))
        return 1;

    hr = MFStartup(
        MF_VERSION,
        MFSTARTUP_FULL);

    PrintHR("MFStartup", hr);

    if (FAILED(hr))
    {
        CoUninitialize();
        return 1;
    }

    IUnknown* unknown = nullptr;

    std::cout << "\n[1] CoCreateInstance\n";

    hr = CoCreateInstance(
        CLSID_LensLinksMediaSource,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IUnknown,
        reinterpret_cast<void**>(&unknown));

    PrintHR("CoCreateInstance", hr);

    if (FAILED(hr))
    {
        std::cout << "\nCOM activation FAILED.\n";

        MFShutdown();
        CoUninitialize();

        return 2;
    }

    std::cout << "COM activation SUCCESS.\n";

    IMFMediaSource* source = nullptr;

    std::cout << "\n[2] QueryInterface(IMFMediaSource)\n";

    hr = unknown->QueryInterface(
        IID_IMFMediaSource,
        reinterpret_cast<void**>(&source));

    PrintHR("QueryInterface IMFMediaSource", hr);

    unknown->Release();
    unknown = nullptr;

    if (FAILED(hr))
    {
        MFShutdown();
        CoUninitialize();
        return 3;
    }

    std::cout << "IMFMediaSource SUCCESS.\n";

    DWORD characteristics = 0;

    std::cout << "\n[3] GetCharacteristics\n";

    hr = source->GetCharacteristics(
        &characteristics);

    PrintHR("GetCharacteristics", hr);

    if (SUCCEEDED(hr))
    {
        std::cout
            << "Characteristics = 0x"
            << std::hex
            << characteristics
            << std::dec
            << std::endl;
    }

    IMFPresentationDescriptor* descriptor = nullptr;

    std::cout << "\n[4] CreatePresentationDescriptor\n";

    hr = source->CreatePresentationDescriptor(
        &descriptor);

    PrintHR(
        "CreatePresentationDescriptor",
        hr);

    if (SUCCEEDED(hr) && descriptor != nullptr)
    {
        DWORD streamCount = 0;

        hr = descriptor->GetStreamDescriptorCount(
            &streamCount);

        PrintHR(
            "GetStreamDescriptorCount",
            hr);

        std::cout
            << "Stream count = "
            << streamCount
            << std::endl;

        if (streamCount > 0)
        {
            BOOL selected = FALSE;

            IMFStreamDescriptor* streamDescriptor =
                nullptr;

            hr = descriptor->GetStreamDescriptorByIndex(
                0,
                &selected,
                &streamDescriptor);

            PrintHR(
                "GetStreamDescriptorByIndex",
                hr);

            if (SUCCEEDED(hr) &&
                streamDescriptor != nullptr)
            {
                std::cout
                    << "Stream 0 selected = "
                    << (selected ? "TRUE" : "FALSE")
                    << std::endl;

                IMFMediaTypeHandler* handler =
                    nullptr;

                hr = streamDescriptor->GetMediaTypeHandler(
                    &handler);

                PrintHR(
                    "GetMediaTypeHandler",
                    hr);

                if (SUCCEEDED(hr) &&
                    handler != nullptr)
                {
                    DWORD typeCount = 0;

                    hr = handler->GetMediaTypeCount(
                        &typeCount);

                    PrintHR(
                        "GetMediaTypeCount",
                        hr);

                    std::cout
                        << "Media type count = "
                        << typeCount
                        << std::endl;

                    if (typeCount > 0)
                    {
                        IMFMediaType* type = nullptr;

                        hr = handler->GetMediaTypeByIndex(
                            0,
                            &type);

                        PrintHR(
                            "GetMediaTypeByIndex(0)",
                            hr);

                        if (SUCCEEDED(hr) &&
                            type != nullptr)
                        {
                            UINT32 width = 0;
                            UINT32 height = 0;

                            MFGetAttributeSize(
                                type,
                                MF_MT_FRAME_SIZE,
                                &width,
                                &height);

                            std::cout
                                << "Media type size = "
                                << width
                                << "x"
                                << height
                                << std::endl;

                            type->Release();
                        }

                        handler->Release();
                    }
                }

                streamDescriptor->Release();
            }
        }

        std::cout
            << "\n[5] Start\n";

        hr = source->Start(
            descriptor,
            nullptr,
            nullptr);

        PrintHR("IMFMediaSource::Start", hr);

        descriptor->Release();
        descriptor = nullptr;
    }

    std::cout << "\n[6] Stop\n";

    hr = source->Stop();

    PrintHR("IMFMediaSource::Stop", hr);

    std::cout << "\n[7] Shutdown\n";

    hr = source->Shutdown();

    PrintHR("IMFMediaSource::Shutdown", hr);

    source->Release();
    source = nullptr;

    MFShutdown();
    CoUninitialize();

    std::cout << "\n========================================\n";
    std::cout << " Test complete\n";
    std::cout << "========================================\n";

    return 0;
}
