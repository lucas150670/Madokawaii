//
// Direct2D backend shared state and helpers.
//

#ifndef MADOKAWAII_DIRECT2D_PLATFORM_H
#define MADOKAWAII_DIRECT2D_PLATFORM_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#ifdef DrawText
#undef DrawText
#endif
#ifdef DrawTextEx
#undef DrawTextEx
#endif
#ifdef LoadImage
#undef LoadImage
#endif
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Madokawaii/platform/graphics.h"
#include "Madokawaii/platform/shape.h"

namespace Madokawaii::Platform::Direct2D {

    template <typename T>
    void SafeRelease(T*& value) {
        if (value) {
            value->Release();
            value = nullptr;
        }
    }

    struct ImageData {
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<std::uint8_t> pixelsBgra;
    };

    struct TextureData {
        std::uint32_t width{};
        std::uint32_t height{};
        std::vector<std::uint8_t> pixelsBgra;
        ID2D1Bitmap* bitmap{};
        std::unordered_map<std::uint32_t, ID2D1Bitmap*> tintedBitmaps;

        ~TextureData();
    };

    struct FontData {
        IDWriteFontFace* face{};
        bool valid{};

        ~FontData();
    };

    struct FileDialogData {
        std::wstring initialPath;
    };

    struct PlatformState {
        HINSTANCE instance{};
        HWND window{};
        bool windowClassRegistered{};
        bool shouldClose{};
        bool fullscreen{};
        DWORD windowedStyle{};
        RECT windowedRect{};
        int screenWidth{};
        int screenHeight{};

        ID2D1Factory* d2dFactory{};
        IDWriteFactory* writeFactory{};
        IWICImagingFactory* wicFactory{};
        ID2D1HwndRenderTarget* renderTarget{};

        D2D1_MATRIX_3X2_F currentTransform{};
        std::vector<D2D1_MATRIX_3X2_F> transformStack;

        float targetFrameSeconds{};
        float frameTime{};
        float fps{};
        std::chrono::steady_clock::time_point lastFrameTime{};
        std::chrono::steady_clock::time_point fpsWindowStart{};
        int fpsFrameCounter{};
        bool drawing{};

        POINT mousePosition{};
        bool mouseLeftDown{};
        bool mouseLeftPressed{};
        bool mouseLeftReleased{};
        bool anyKeyPressed{};
        bool backspacePressed{};
        bool enterPressed{};
        std::wstring typedText;

        bool guiLocked{};
        int guiTextSize{16};
    };

    PlatformState& GetState();

    bool InitializeWindow(int width, int height, const char* title);
    void ShutdownWindow();
    void PumpMessages();
    void SetTargetFrameRate(int targetFps);
    void BeginFrame();
    void EndFrame();
    void ResizeRenderTarget(int width, int height);
    void ToggleFullscreenWindow();

    bool EnsureFactories();
    bool EnsureRenderTarget();
    ID2D1HwndRenderTarget* RenderTarget();
    IDWriteFactory* WriteFactory();
    IWICImagingFactory* WicFactory();

    ID2D1SolidColorBrush* CreateBrush(Graphics::Color color);
    D2D1_COLOR_F ToD2DColor(Graphics::Color color);
    std::uint32_t ColorKey(Graphics::Color color);

    std::wstring Utf8ToWide(const char* text);
    std::wstring Utf8ToWide(const std::string& text);
    std::string WideToUtf8(const std::wstring& text);

    bool IsMouseOver(Shape::Rectangle bounds);
    bool IsLeftMousePressed();
    bool IsLeftMouseReleased();
    bool IsLeftMouseDown();
    void ResetTransientInput();

    HRESULT CreateBitmapFromPixels(
        const std::uint8_t* pixelsBgra,
        std::uint32_t width,
        std::uint32_t height,
        Graphics::Color tint,
        ID2D1Bitmap** outBitmap);

}

#endif // MADOKAWAII_DIRECT2D_PLATFORM_H
