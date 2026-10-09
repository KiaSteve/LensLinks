#include "jpeg_decoder.h"

#include "../shared/frame.h"

#include <windows.h>
#include <wincodec.h>

#include <iostream>

#pragma comment(lib, "windowscodecs.lib")

JpegDecoder::JpegDecoder()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

JpegDecoder::~JpegDecoder()
{
    CoUninitialize();
}

bool JpegDecoder::decode(
    const std::vector<uint8_t>& jpeg,
    Frame& frame)
{
    frame = Frame{};

    HRESULT hr;

    IWICImagingFactory* factory = nullptr;
    IWICStream* stream = nullptr;
    IWICBitmapDecoder* decoder = nullptr;
    IWICBitmapFrameDecode* bitmapFrame = nullptr;
    IWICFormatConverter* converter = nullptr;

    hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory));

    if (FAILED(hr))
        goto cleanup;

    hr = factory->CreateStream(&stream);

    if (FAILED(hr))
        goto cleanup;

    hr = stream->InitializeFromMemory(
        const_cast<BYTE*>(jpeg.data()),
        static_cast<DWORD>(jpeg.size()));

    if (FAILED(hr))
        goto cleanup;

    hr = factory->CreateDecoderFromStream(
        stream,
        nullptr,
        WICDecodeMetadataCacheOnLoad,
        &decoder);

    if (FAILED(hr))
        goto cleanup;

    hr = decoder->GetFrame(0, &bitmapFrame);

    if (FAILED(hr))
        goto cleanup;

    UINT w = 0;
    UINT h = 0;

    hr = bitmapFrame->GetSize(&w, &h);

    if (FAILED(hr))
        goto cleanup;

    frame.width = static_cast<int>(w);
    frame.height = static_cast<int>(h);
    frame.stride = frame.width * 4;

    hr = factory->CreateFormatConverter(&converter);

    if (FAILED(hr))
        goto cleanup;

    hr = converter->Initialize(
        bitmapFrame,
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0,
        WICBitmapPaletteTypeCustom);

    if (FAILED(hr))
        goto cleanup;

    frame.pixels.resize(
        frame.stride *
        frame.height);

    hr = converter->CopyPixels(
        nullptr,
        frame.stride,
        static_cast<UINT>(frame.pixels.size()),
        frame.pixels.data());

    if (FAILED(hr))
        goto cleanup;

    if (converter) converter->Release();
    if (bitmapFrame) bitmapFrame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();

    return true;

cleanup:

    if (converter) converter->Release();
    if (bitmapFrame) bitmapFrame->Release();
    if (decoder) decoder->Release();
    if (stream) stream->Release();
    if (factory) factory->Release();

    frame = Frame{};

    std::cout << "JPEG decode failed." << std::endl;

    return false;
}