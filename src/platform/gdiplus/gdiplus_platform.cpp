//
// 你，你不会真的要用这个东西吧？
//

#include "gdiplus_platform.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

namespace Madokawaii::Platform::GdiPlusBackend {
    namespace {
        constexpr float Pi = 3.14159265358979323846f;

        RenderState gState{};

        void ReleaseBackBuffer(RenderState& state) {
            state.graphics.reset();
            if (state.backBufferDc && state.previousBitmap) {
                SelectObject(state.backBufferDc, state.previousBitmap);
                state.previousBitmap = nullptr;
            }
            if (state.backBufferBitmap) {
                DeleteObject(state.backBufferBitmap);
                state.backBufferBitmap = nullptr;
            }
            if (state.backBufferDc) {
                DeleteDC(state.backBufferDc);
                state.backBufferDc = nullptr;
            }
            state.backBufferWidth = 0;
            state.backBufferHeight = 0;
        }

        bool EnsureBackBuffer(RenderState& state) {
            auto& windowState = Direct2D::GetState();
            if (!windowState.window) return false;

            RECT rc{};
            GetClientRect(windowState.window, &rc);
            const auto width = std::max<LONG>(1, rc.right - rc.left);
            const auto height = std::max<LONG>(1, rc.bottom - rc.top);
            windowState.screenWidth = static_cast<int>(width);
            windowState.screenHeight = static_cast<int>(height);

            if (state.backBufferDc
                && state.backBufferBitmap
                && state.backBufferWidth == width
                && state.backBufferHeight == height) {
                return true;
            }

            ReleaseBackBuffer(state);

            auto* windowDc = GetDC(windowState.window);
            if (!windowDc) return false;

            state.backBufferDc = CreateCompatibleDC(windowDc);
            if (!state.backBufferDc) {
                ReleaseDC(windowState.window, windowDc);
                return false;
            }

            BITMAPINFO bitmapInfo{};
            bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bitmapInfo.bmiHeader.biWidth = width;
            bitmapInfo.bmiHeader.biHeight = -height;
            bitmapInfo.bmiHeader.biPlanes = 1;
            bitmapInfo.bmiHeader.biBitCount = 32;
            bitmapInfo.bmiHeader.biCompression = BI_RGB;

            void* ignoredBits{};
            state.backBufferBitmap = CreateDIBSection(
                windowDc,
                &bitmapInfo,
                DIB_RGB_COLORS,
                &ignoredBits,
                nullptr,
                0);
            ReleaseDC(windowState.window, windowDc);

            if (!state.backBufferBitmap) {
                ReleaseBackBuffer(state);
                return false;
            }

            state.previousBitmap = SelectObject(state.backBufferDc, state.backBufferBitmap);
            state.backBufferWidth = static_cast<int>(width);
            state.backBufferHeight = static_cast<int>(height);
            return state.previousBitmap != nullptr;
        }

        void UpdateFrameStats(RenderState& state) {
            const auto now = std::chrono::steady_clock::now();
            state.frameBegin = now;

            if (state.lastFrameTime.time_since_epoch().count() == 0) {
                state.frameTime = 1.0f / 60.0f;
                state.lastFrameTime = now;
                state.fpsWindowStart = now;
                state.fps = 60.0f;
                return;
            }

            state.frameTime = std::chrono::duration<float>(now - state.lastFrameTime).count();
            state.lastFrameTime = now;
            Common::FrameStats::RecordFrameTime(
                state.frameTimeSamples,
                state.frameTimeSampleIndex,
                state.frameTimeSampleCount,
                state.frameTime);

            state.fpsFrameCounter++;
            const auto fpsWindowSeconds = std::chrono::duration<float>(now - state.fpsWindowStart).count();
            if (fpsWindowSeconds >= 0.5f) {
                state.fps = static_cast<float>(state.fpsFrameCounter) / fpsWindowSeconds;
                state.fpsFrameCounter = 0;
                state.fpsWindowStart = now;
            }
        }

        std::uint8_t ClampByte(std::uint32_t value) {
            return static_cast<std::uint8_t>(std::min<std::uint32_t>(value, 255u));
        }
    }

    RenderState& GetState() {
        return gState;
    }

    bool Startup() {
        auto& state = GetState();
        if (state.gdiplusStarted) return true;

        Gdiplus::GdiplusStartupInput input{};
        if (Gdiplus::GdiplusStartup(&state.gdiplusToken, &input, nullptr) != Gdiplus::Ok) {
            return false;
        }

        state.gdiplusStarted = true;
        state.currentTransform = {};
        return true;
    }

    void Shutdown() {
        auto& state = GetState();
        ReleaseBackBuffer(state);
        state.transformStack.clear();
        if (state.gdiplusStarted) {
            Gdiplus::GdiplusShutdown(state.gdiplusToken);
            state.gdiplusToken = 0;
            state.gdiplusStarted = false;
        }
    }

    void BeginFrame() {
        auto& state = GetState();
        if (!Startup()) return;

        Direct2D::PumpMessages();
        UpdateFrameStats(state);
        if (!EnsureBackBuffer(state)) return;

        state.graphics = std::make_unique<Gdiplus::Graphics>(state.backBufferDc);
        state.graphics->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
        state.graphics->SetCompositingQuality(Gdiplus::CompositingQualityHighSpeed);
        state.graphics->SetInterpolationMode(Gdiplus::InterpolationModeLowQuality);
        state.graphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        state.graphics->SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);
        state.graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);

        state.transformStack.clear();
        state.currentTransform = {};
        state.graphics->ResetTransform();
        state.drawing = true;
    }

    void EndFrame() {
        auto& state = GetState();
        auto& windowState = Direct2D::GetState();
        if (!state.drawing || !state.graphics || !windowState.window) {
            state.drawing = false;
            state.graphics.reset();
            Direct2D::ResetTransientInput();
            return;
        }

        state.graphics->Flush(Gdiplus::FlushIntentionFlush);
        state.graphics.reset();

        auto* windowDc = GetDC(windowState.window);
        if (windowDc) {
            BitBlt(
                windowDc,
                0,
                0,
                state.backBufferWidth,
                state.backBufferHeight,
                state.backBufferDc,
                0,
                0,
                SRCCOPY);
            ReleaseDC(windowState.window, windowDc);
        }

        state.drawing = false;
        Direct2D::ResetTransientInput();

        if (state.targetFrameSeconds > 0.0f) {
            const auto elapsed = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - state.frameBegin).count();
            const auto sleepSeconds = state.targetFrameSeconds - elapsed;
            if (sleepSeconds > 0.001f) {
                std::this_thread::sleep_for(std::chrono::duration<float>(sleepSeconds));
            }
        }
    }

    void SetTargetFrameRate(int targetFps) {
        GetState().targetFrameSeconds = targetFps > 0 ? 1.0f / static_cast<float>(targetFps) : 0.0f;
    }

    Gdiplus::Graphics* ActiveGraphics() {
        return GetState().graphics.get();
    }

    Gdiplus::PointF ApplyTransform(TransformState transform, float x, float y) {
        const float radians = transform.rotation * Pi / 180.0f;
        const float sinAngle = std::sin(radians);
        const float cosAngle = std::cos(radians);
        const float scaledX = x * transform.scaleX;
        const float scaledY = y * transform.scaleY;

        return {
            transform.x + cosAngle * scaledX - sinAngle * scaledY,
            transform.y + sinAngle * scaledX + cosAngle * scaledY
        };
    }

    TransformState ComposeTransform(TransformState parent, TransformState local) {
        const auto origin = ApplyTransform(parent, local.x, local.y);
        return {
            origin.X,
            origin.Y,
            parent.rotation + local.rotation,
            parent.scaleX * local.scaleX,
            parent.scaleY * local.scaleY
        };
    }

    Gdiplus::Color ToGdiColor(Graphics::Color color) {
        return Gdiplus::Color(color.a, color.r, color.g, color.b);
    }

    std::uint32_t ColorKey(Graphics::Color color) {
        return (static_cast<std::uint32_t>(color.r) << 24)
            | (static_cast<std::uint32_t>(color.g) << 16)
            | (static_cast<std::uint32_t>(color.b) << 8)
            | static_cast<std::uint32_t>(color.a);
    }

    std::unique_ptr<Gdiplus::Bitmap> CreateBitmapFromPixels(
        const std::uint8_t* pixelsBgra,
        std::uint32_t width,
        std::uint32_t height,
        Graphics::Color tint) {
        if (!pixelsBgra || width == 0 || height == 0) return {};

        auto bitmap = std::make_unique<Gdiplus::Bitmap>(
            static_cast<INT>(width),
            static_cast<INT>(height),
            PixelFormat32bppPARGB);
        if (bitmap->GetLastStatus() != Gdiplus::Ok) return {};

        Gdiplus::Rect lockRect(0, 0, static_cast<INT>(width), static_cast<INT>(height));
        Gdiplus::BitmapData bitmapData{};
        if (bitmap->LockBits(
                &lockRect,
                Gdiplus::ImageLockModeWrite,
                PixelFormat32bppPARGB,
                &bitmapData) != Gdiplus::Ok) {
            return {};
        }

        auto* scan0 = static_cast<std::uint8_t*>(bitmapData.Scan0);
        const auto stride = bitmapData.Stride;
        for (std::uint32_t y = 0; y < height; ++y) {
            auto* row = stride >= 0
                ? scan0 + static_cast<std::ptrdiff_t>(y) * stride
                : scan0 + static_cast<std::ptrdiff_t>(height - 1 - y) * -stride;
            for (std::uint32_t x = 0; x < width; ++x) {
                const auto index = (static_cast<std::size_t>(y) * width + x) * 4;
                const auto srcB = static_cast<std::uint32_t>(pixelsBgra[index + 0]);
                const auto srcG = static_cast<std::uint32_t>(pixelsBgra[index + 1]);
                const auto srcR = static_cast<std::uint32_t>(pixelsBgra[index + 2]);
                const auto srcA = static_cast<std::uint32_t>(pixelsBgra[index + 3]);

                const auto outA = srcA * tint.a / 255u;
                const auto outR = srcR * tint.r / 255u;
                const auto outG = srcG * tint.g / 255u;
                const auto outB = srcB * tint.b / 255u;

                row[x * 4 + 0] = ClampByte(outB * outA / 255u);
                row[x * 4 + 1] = ClampByte(outG * outA / 255u);
                row[x * 4 + 2] = ClampByte(outR * outA / 255u);
                row[x * 4 + 3] = ClampByte(outA);
            }
        }

        bitmap->UnlockBits(&bitmapData);
        return bitmap;
    }
}
