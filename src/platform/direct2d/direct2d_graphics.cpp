//
// Direct2D backend graphics implementation.
//

#include "direct2d_platform.h"

#include "Madokawaii/platform/graphics.h"

#include <algorithm>
#include <format>
#include <vector>

#include <d2d1_3.h>
#include <d2d1helper.h>
#include <d3d11.h>
#include <dxgi.h>
#include <winver.h>

namespace Madokawaii::Platform::Graphics {
    namespace {
        constexpr UINT D3DDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        std::string WideToUtf8(const wchar_t* text) {
            return Direct2D::WideToUtf8(text ? text : L"");
        }

        ID3D11Device* CreateD3DDevice(D3D_FEATURE_LEVEL* outFeatureLevel) {
            const D3D_FEATURE_LEVEL featureLevels[] = {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0,
                D3D_FEATURE_LEVEL_9_3,
                D3D_FEATURE_LEVEL_9_2,
                D3D_FEATURE_LEVEL_9_1,
            };

            ID3D11Device* device{};
            HRESULT hr = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                D3DDeviceFlags,
                featureLevels,
                ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION,
                &device,
                outFeatureLevel,
                nullptr);

            if (hr == E_INVALIDARG) {
                hr = D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    D3DDeviceFlags,
                    featureLevels + 1,
                    ARRAYSIZE(featureLevels) - 1,
                    D3D11_SDK_VERSION,
                    &device,
                    outFeatureLevel,
                    nullptr);
            }

            if (FAILED(hr)) {
                hr = D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP,
                    nullptr,
                    D3DDeviceFlags,
                    featureLevels + 1,
                    ARRAYSIZE(featureLevels) - 1,
                    D3D11_SDK_VERSION,
                    &device,
                    outFeatureLevel,
                    nullptr);
            }

            return SUCCEEDED(hr) ? device : nullptr;
        }

        IDXGIDevice* GetDxgiDevice() {
            D3D_FEATURE_LEVEL featureLevel{};
            auto* d3dDevice = CreateD3DDevice(&featureLevel);
            if (!d3dDevice) return nullptr;

            IDXGIDevice* inputDxgiDevice{};
            if (FAILED(d3dDevice->QueryInterface(IID_PPV_ARGS(&inputDxgiDevice)))) {
                Direct2D::SafeRelease(d3dDevice);
                return nullptr;
            }

            ID2D1Factory3* factory{};
            if (FAILED(D2D1CreateFactory(
                    D2D1_FACTORY_TYPE_SINGLE_THREADED,
                    IID_PPV_ARGS(&factory)))) {
                Direct2D::SafeRelease(inputDxgiDevice);
                Direct2D::SafeRelease(d3dDevice);
                return nullptr;
            }

            ID2D1Device2* d2dDevice{};
            if (FAILED(factory->CreateDevice(inputDxgiDevice, &d2dDevice))) {
                Direct2D::SafeRelease(factory);
                Direct2D::SafeRelease(inputDxgiDevice);
                Direct2D::SafeRelease(d3dDevice);
                return nullptr;
            }

            IDXGIDevice* outputDxgiDevice{};
            const auto hr = d2dDevice->GetDxgiDevice(&outputDxgiDevice);

            Direct2D::SafeRelease(d2dDevice);
            Direct2D::SafeRelease(factory);
            Direct2D::SafeRelease(inputDxgiDevice);
            Direct2D::SafeRelease(d3dDevice);
            if (FAILED(hr)) {
                Direct2D::SafeRelease(outputDxgiDevice);
                return nullptr;
            }
            return outputDxgiDevice;
        }

        std::string QueryDxgiDeviceInfo() {
            auto* dxgiDevice = GetDxgiDevice();
            if (!dxgiDevice) return "Direct2D (DXGI device unavailable)";

            IDXGIAdapter* adapter{};
            if (FAILED(dxgiDevice->GetAdapter(&adapter))) {
                Direct2D::SafeRelease(dxgiDevice);
                return "Direct2D (DXGI adapter unavailable)";
            }

            DXGI_ADAPTER_DESC desc{};
            const auto hr = adapter->GetDesc(&desc);
            Direct2D::SafeRelease(adapter);
            Direct2D::SafeRelease(dxgiDevice);

            if (FAILED(hr)) return "Direct2D (DXGI adapter info unavailable)";

            const auto description = WideToUtf8(desc.Description);
            if (description.empty()) return "Direct2D (unnamed DXGI adapter)";

            return std::format(
                "{} (vendor 0x{:04X}, device 0x{:04X})",
                description,
                desc.VendorId,
                desc.DeviceId);
        }

        std::string QueryDirect2DDllVersion() {
            wchar_t systemDirectory[MAX_PATH]{};
            const auto length = GetSystemDirectoryW(systemDirectory, ARRAYSIZE(systemDirectory));
            if (length == 0 || length >= ARRAYSIZE(systemDirectory)) {
                return "Direct2D d2d1.dll version unavailable";
            }

            const std::wstring dllPath = std::wstring(systemDirectory) + L"\\d2d1.dll";
            DWORD handle{};
            const auto infoSize = GetFileVersionInfoSizeW(dllPath.c_str(), &handle);
            if (infoSize == 0) {
                return "Direct2D d2d1.dll version unavailable";
            }

            std::vector<std::uint8_t> versionInfo(infoSize);
            if (!GetFileVersionInfoW(dllPath.c_str(), handle, infoSize, versionInfo.data())) {
                return "Direct2D d2d1.dll version unavailable";
            }

            VS_FIXEDFILEINFO* fixedFileInfo{};
            UINT fixedFileInfoSize{};
            if (!VerQueryValueW(
                    versionInfo.data(),
                    L"\\",
                    reinterpret_cast<void**>(&fixedFileInfo),
                    &fixedFileInfoSize)
                || fixedFileInfoSize < sizeof(VS_FIXEDFILEINFO)
                || fixedFileInfo->dwSignature != 0xfeef04bd) {
                return "Direct2D d2d1.dll version unavailable";
            }

            return std::format(
                "Direct2D d2d1.dll {}.{}.{}.{}",
                HIWORD(fixedFileInfo->dwFileVersionMS),
                LOWORD(fixedFileInfo->dwFileVersionMS),
                HIWORD(fixedFileInfo->dwFileVersionLS),
                LOWORD(fixedFileInfo->dwFileVersionLS));
        }

        IDWriteTextFormat* CreateDefaultTextFormat(float fontSize) {
            auto* factory = Direct2D::WriteFactory();
            if (!factory) return nullptr;

            IDWriteTextFormat* format{};
            if (FAILED(factory->CreateTextFormat(
                    L"Segoe UI",
                    nullptr,
                    DWRITE_FONT_WEIGHT_REGULAR,
                    DWRITE_FONT_STYLE_NORMAL,
                    DWRITE_FONT_STRETCH_NORMAL,
                    fontSize,
                    L"",
                    &format))) {
                return nullptr;
            }
            return format;
        }
    }

    std::string GetImplementer() {
        static const auto implementer = QueryDxgiDeviceInfo();
        return implementer;
    }

    std::string GetImplementationInfo() {
        static const auto implementationInfo = QueryDirect2DDllVersion();
        return implementationInfo;
    }

    float GetFPS() {
        return Direct2D::GetState().fps;
    }

    float GetFrameTime() {
        return Direct2D::GetState().frameTime;
    }

    void SetTargetFPS(int target) {
        Direct2D::SetTargetFrameRate(target);
    }

    void BeginDrawing() {
        Direct2D::BeginFrame();
    }

    void EndDrawing() {
        Direct2D::EndFrame();
    }

    void ClearBackground(Color color) {
        if (auto* renderTarget = Direct2D::RenderTarget()) {
            renderTarget->Clear(Direct2D::ToD2DColor(color));
        }
    }

    void DrawText(const char* text, int x, int y, int fontSize, Color color) {
        auto* renderTarget = Direct2D::RenderTarget();
        if (!renderTarget || !text) return;

        auto wideText = Direct2D::Utf8ToWide(text);
        auto* format = CreateDefaultTextFormat(static_cast<float>(fontSize));
        auto* brush = Direct2D::CreateBrush(color);
        if (!format || !brush) {
            Direct2D::SafeRelease(format);
            Direct2D::SafeRelease(brush);
            return;
        }

        const D2D1_RECT_F bounds = D2D1::RectF(
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(x) + 8192.0f,
            static_cast<float>(y) + std::max(1, fontSize) * 2.0f);
        renderTarget->DrawText(
            wideText.c_str(),
            static_cast<UINT32>(wideText.size()),
            format,
            bounds,
            brush,
            D2D1_DRAW_TEXT_OPTIONS_CLIP,
            DWRITE_MEASURING_MODE_NATURAL);

        Direct2D::SafeRelease(brush);
        Direct2D::SafeRelease(format);
    }

    void DrawLineEx(Vector2 start, Vector2 end, float thick, Color color) {
        auto* renderTarget = Direct2D::RenderTarget();
        auto* brush = Direct2D::CreateBrush(color);
        if (!renderTarget || !brush) {
            Direct2D::SafeRelease(brush);
            return;
        }

        renderTarget->DrawLine(
            D2D1::Point2F(start.x, start.y),
            D2D1::Point2F(end.x, end.y),
            brush,
            std::max(1.0f, thick));

        Direct2D::SafeRelease(brush);
    }

    int MeasureText(const char* text, int fontSize) {
        auto* factory = Direct2D::WriteFactory();
        if (!factory || !text) return 0;

        auto wideText = Direct2D::Utf8ToWide(text);
        auto* format = CreateDefaultTextFormat(static_cast<float>(fontSize));
        if (!format) return 0;

        IDWriteTextLayout* layout{};
        if (FAILED(factory->CreateTextLayout(
                wideText.c_str(),
                static_cast<UINT32>(wideText.size()),
                format,
                8192.0f,
                static_cast<float>(std::max(1, fontSize) * 2),
                &layout))) {
            Direct2D::SafeRelease(format);
            return 0;
        }

        DWRITE_TEXT_METRICS metrics{};
        layout->GetMetrics(&metrics);
        Direct2D::SafeRelease(layout);
        Direct2D::SafeRelease(format);
        return static_cast<int>(metrics.widthIncludingTrailingWhitespace + 0.5f);
    }

    void DrawRectangle(int posX, int posY, int width, int height, Color color) {
        auto* renderTarget = Direct2D::RenderTarget();
        auto* brush = Direct2D::CreateBrush(color);
        if (!renderTarget || !brush) {
            Direct2D::SafeRelease(brush);
            return;
        }

        renderTarget->FillRectangle(
            D2D1::RectF(
                static_cast<float>(posX),
                static_cast<float>(posY),
                static_cast<float>(posX + width),
                static_cast<float>(posY + height)),
            brush);
        Direct2D::SafeRelease(brush);
    }

    void SetTransform(float x, float y, float rotate, float scaleX, float scaleY) {
        auto& state = Direct2D::GetState();
        auto* renderTarget = Direct2D::RenderTarget();
        if (!renderTarget) return;

        state.transformStack.push_back(state.currentTransform);
        const auto localTransform =
            D2D1::Matrix3x2F::Scale(scaleX, scaleY)
            * D2D1::Matrix3x2F::Rotation(rotate)
            * D2D1::Matrix3x2F::Translation(x, y);
        state.currentTransform = state.currentTransform * localTransform;
        renderTarget->SetTransform(state.currentTransform);
    }

    void PopTransform() {
        auto& state = Direct2D::GetState();
        auto* renderTarget = Direct2D::RenderTarget();
        if (!renderTarget || state.transformStack.empty()) return;

        state.currentTransform = state.transformStack.back();
        state.transformStack.pop_back();
        renderTarget->SetTransform(state.currentTransform);
    }
}
