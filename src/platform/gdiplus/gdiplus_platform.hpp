//
// 你，你不会真的要用这个东西吧？
//

#ifndef MADOKAWAII_GDIPLUS_PLATFORM_H
#define MADOKAWAII_GDIPLUS_PLATFORM_H

#include "../direct2d/direct2d_platform.hpp"

#include <gdiplus.h>
#include <objidl.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Madokawaii::Platform::GdiPlusBackend {

    struct ImageData {
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<std::uint8_t> pixelsBgra;
    };

    struct TextureData {
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<std::uint8_t> pixelsBgra;
        std::unique_ptr<Gdiplus::Bitmap> bitmap;
        std::unordered_map<std::uint32_t, std::unique_ptr<Gdiplus::Bitmap>> tintedBitmaps;
    };

    struct FontData {
        std::unique_ptr<Gdiplus::PrivateFontCollection> collection;
        std::wstring familyName;
        bool valid{};
    };

    struct TransformState {
        float x{};
        float y{};
        float rotation{};
        float scaleX{1.0f};
        float scaleY{1.0f};
    };

    struct RenderState {
        ULONG_PTR gdiplusToken{};
        bool gdiplusStarted{};

        HDC backBufferDc{};
        HBITMAP backBufferBitmap{};
        HGDIOBJ previousBitmap{};
        int backBufferWidth{};
        int backBufferHeight{};

        std::unique_ptr<Gdiplus::Graphics> graphics;
        TransformState currentTransform{};
        std::vector<TransformState> transformStack;

        float targetFrameSeconds{};
        float frameTime{};
        float fps{};
        std::array<float, Common::FrameStats::SampleCount> frameTimeSamples{};
        std::size_t frameTimeSampleIndex{};
        std::size_t frameTimeSampleCount{};
        std::chrono::steady_clock::time_point lastFrameTime{};
        std::chrono::steady_clock::time_point fpsWindowStart{};
        std::chrono::steady_clock::time_point frameBegin{};
        int fpsFrameCounter{};
        bool drawing{};
    };

    RenderState& GetState();

    bool Startup();
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void SetTargetFrameRate(int targetFps);
    Gdiplus::Graphics* ActiveGraphics();

    Gdiplus::PointF ApplyTransform(TransformState transform, float x, float y);
    TransformState ComposeTransform(TransformState parent, TransformState local);

    Gdiplus::Color ToGdiColor(Graphics::Color color);
    std::uint32_t ColorKey(Graphics::Color color);
    std::unique_ptr<Gdiplus::Bitmap> CreateBitmapFromPixels(
        const std::uint8_t* pixelsBgra,
        std::uint32_t width,
        std::uint32_t height,
        Graphics::Color tint);
}

#endif // MADOKAWAII_GDIPLUS_PLATFORM_H
