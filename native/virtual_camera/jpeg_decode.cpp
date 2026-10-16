#include "jpeg_decode.h"

#define NOMINMAX
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>

using Microsoft::WRL::ComPtr;

namespace
{
    std::atomic<int> g_rotation(0);

    bool decodeInternal(
        const uint8_t* jpeg,
        size_t size,
        int outW,
        int outH,
        std::vector<uint8_t>& rgba)
    {
        ComPtr<IWICImagingFactory> factory;

        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory));

        if (FAILED(hr)) return false;

        ComPtr<IWICStream> stream;
        hr = factory->CreateStream(&stream);
        if (FAILED(hr)) return false;

        hr = stream->InitializeFromMemory(
            const_cast<BYTE*>(jpeg),
            static_cast<DWORD>(size));
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapDecoder> decoder;
        hr = factory->CreateDecoderFromStream(
            stream.Get(),
            nullptr,
            WICDecodeMetadataCacheOnDemand,
            &decoder);
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapFrameDecode> frame;
        hr = decoder->GetFrame(0, &frame);
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapSource> source;
        hr = frame.As(&source);
        if (FAILED(hr)) return false;

        // Optional rotation, applied before measuring and scaling.
        const int rotation = g_rotation.load();

        if (rotation != 0)
        {
            WICBitmapTransformOptions options = WICBitmapTransformRotate90;

            if (rotation == 180) options = WICBitmapTransformRotate180;
            else if (rotation == 270) options = WICBitmapTransformRotate270;

            ComPtr<IWICBitmapFlipRotator> rotator;
            hr = factory->CreateBitmapFlipRotator(&rotator);
            if (FAILED(hr)) return false;

            hr = rotator->Initialize(source.Get(), options);
            if (FAILED(hr)) return false;

            source.Reset();
            hr = rotator.As(&source);
            if (FAILED(hr)) return false;
        }

        UINT srcW = 0;
        UINT srcH = 0;
        hr = source->GetSize(&srcW, &srcH);
        if (FAILED(hr) || srcW == 0 || srcH == 0) return false;

        const double scale = (std::min)(
            static_cast<double>(outW) / srcW,
            static_cast<double>(outH) / srcH);

        UINT dstW = static_cast<UINT>(srcW * scale + 0.5);
        UINT dstH = static_cast<UINT>(srcH * scale + 0.5);
        dstW = (std::max)(static_cast<UINT>(1), (std::min)(dstW, static_cast<UINT>(outW)));
        dstH = (std::max)(static_cast<UINT>(1), (std::min)(dstH, static_cast<UINT>(outH)));

        if (dstW != srcW || dstH != srcH)
        {
            ComPtr<IWICBitmapScaler> scaler;
            hr = factory->CreateBitmapScaler(&scaler);
            if (FAILED(hr)) return false;

            hr = scaler->Initialize(
                source.Get(),
                dstW,
                dstH,
                WICBitmapInterpolationModeFant);
            if (FAILED(hr)) return false;

            source.Reset();
            hr = scaler.As(&source);
            if (FAILED(hr)) return false;
        }

        ComPtr<IWICFormatConverter> converter;
        hr = factory->CreateFormatConverter(&converter);
        if (FAILED(hr)) return false;

        hr = converter->Initialize(
            source.Get(),
            GUID_WICPixelFormat32bppBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom);
        if (FAILED(hr)) return false;

        std::vector<uint8_t> bgra(static_cast<size_t>(dstW) * dstH * 4);

        hr = converter->CopyPixels(
            nullptr,
            dstW * 4,
            static_cast<UINT>(bgra.size()),
            bgra.data());
        if (FAILED(hr)) return false;

        // Black canvas with opaque alpha.
        const size_t total = static_cast<size_t>(outW) * outH * 4;
        rgba.assign(total, 0);
        for (size_t i = 3; i < total; i += 4)
        {
            rgba[i] = 255;
        }

        const int x0 = (outW - static_cast<int>(dstW)) / 2;
        const int y0 = (outH - static_cast<int>(dstH)) / 2;

        for (UINT row = 0; row < dstH; ++row)
        {
            const uint8_t* s = &bgra[static_cast<size_t>(row) * dstW * 4];
            uint8_t* d = &rgba[
                (static_cast<size_t>(y0 + row) * outW + x0) * 4];

            for (UINT col = 0; col < dstW; ++col)
            {
                d[0] = s[2]; // R
                d[1] = s[1]; // G
                d[2] = s[0]; // B
                d[3] = 255;
                s += 4;
                d += 4;
            }
        }

        return true;
    }
}

void SetJpegRotationDegrees(int degrees)
{
    int normalized = (degrees / 90) * 90;
    normalized %= 360;
    if (normalized < 0) normalized += 360;

    g_rotation.store(normalized);
}

bool DecodeJpegLetterboxRgba(
    const uint8_t* jpeg,
    size_t size,
    int outW,
    int outH,
    std::vector<uint8_t>& rgba)
{
    if (jpeg == nullptr || size == 0 || outW <= 0 || outH <= 0)
    {
        return false;
    }

    // The calling thread may already have COM initialised in another
    // apartment mode (Flutter's UI thread does). That is fine for WIC.
    const HRESULT initHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (FAILED(initHr) && initHr != RPC_E_CHANGED_MODE)
    {
        return false;
    }

    const bool ok = decodeInternal(jpeg, size, outW, outH, rgba);

    if (SUCCEEDED(initHr))
    {
        CoUninitialize();
    }

    return ok;
}
