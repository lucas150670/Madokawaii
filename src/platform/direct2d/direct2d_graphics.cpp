//
// Direct2D backend graphics implementation.
//

#include "direct2d_platform.hpp"

#include "Madokawaii/platform/graphics.hpp"

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
        std::string QuerySystemDllVersion(const wchar_t* dllName, const char* label) {
            wchar_t systemDirectory[MAX_PATH]{};
            const auto length = GetSystemDirectoryW(systemDirectory, ARRAYSIZE(systemDirectory));
            if (length == 0 || length >= ARRAYSIZE(systemDirectory)) {
                return std::format("{} version unavailable", label);
            }

            const std::wstring dllPath = std::wstring(systemDirectory) + L"\\" + dllName;
            DWORD handle{};
            const auto infoSize = GetFileVersionInfoSizeW(dllPath.c_str(), &handle);
            if (infoSize == 0) {
                return std::format("{} version unavailable", label);
            }

            std::vector<std::uint8_t> versionInfo(infoSize);
            if (!GetFileVersionInfoW(dllPath.c_str(), handle, infoSize, versionInfo.data())) {
                return std::format("{} version unavailable", label);
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
                return std::format("{} version unavailable", label);
            }

            return std::format(
                "{} {}.{}.{}.{}",
                label,
                HIWORD(fixedFileInfo->dwFileVersionMS),
                LOWORD(fixedFileInfo->dwFileVersionMS),
                HIWORD(fixedFileInfo->dwFileVersionLS),
                LOWORD(fixedFileInfo->dwFileVersionLS));
        }

        std::string QueryBackendDllVersion() {
            return QuerySystemDllVersion(
                Direct2D::ImplementationDllName(),
                Direct2D::ImplementationLabel());
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
        Direct2D::PlatformState& state = Direct2D::GetState();
        return state.implementerInfo;
    }

    std::string GetImplementationInfo() {
        static const auto implementationInfo = QueryBackendDllVersion();
        return implementationInfo;
    }

    float GetFPS() {
        return Direct2D::GetState().fps;
    }

    float GetFrameTime() {
        return Direct2D::GetState().frameTime;
    }

    float GetOnePercentLowFPS()
    {
        const auto& state = Direct2D::GetState();
        return Common::FrameStats::GetOnePercentLowFPS(state.frameTimeSamples, state.frameTimeSampleCount);
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
