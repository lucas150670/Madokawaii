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
#include <d2d1_1.h>
#include <d3d11.h>
#include <dwrite.h>
#include <dxgi1_4.h>
#include <wincodec.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "Madokawaii/platform/graphics.hpp"
#include "Madokawaii/platform/shape.hpp"
#include "../common/frame_stats.hpp"

namespace Madokawaii::Platform::Direct2D {

    inline constexpr UINT SwapChainBufferCount = 2;

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

        ID2D1Factory1* d2dFactory{};
        IDWriteFactory* writeFactory{};
        IWICImagingFactory* wicFactory{};
        ID3D11Device* d3dDevice{};
        ID3D11DeviceContext* d3dContext{};
        IDXGIFactory2* dxgiFactory{};
        IDXGISwapChain1* swapChain{};
        ID2D1Device* d2dDevice{};
        ID2D1DeviceContext* renderTarget{};
        ID2D1Bitmap1* targetBitmap{};
        D3D_FEATURE_LEVEL featureLevel{};

        D2D1_MATRIX_3X2_F currentTransform{};
        std::vector<D2D1_MATRIX_3X2_F> transformStack;

        float targetFrameSeconds{};
        float frameTime{};
        float fps{};
        std::array<float, Common::FrameStats::SampleCount> frameTimeSamples{};
        std::size_t frameTimeSampleIndex{};
        std::size_t frameTimeSampleCount{};
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

        std::string implementerInfo{};
        bool isTearingSupport{};
    };

    struct RenderBackend {
        bool (*ensureRenderTarget)(PlatformState& state);
        void (*releaseSwapChainTarget)(PlatformState& state);
        void (*releaseDeviceResources)(PlatformState& state);
        bool (*beginFrame)(PlatformState& state);
        void (*endFrameBeforePresent)(PlatformState& state);
        void (*afterPresent)(PlatformState& state, HRESULT presentResult);
        void (*resizeRenderTarget)(PlatformState& state, int width, int height);
        const wchar_t* (*implementationDllName)();
        const char* (*implementationLabel)();
    };

    PlatformState& GetState();
    void SetRenderBackend(const RenderBackend* backend);

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
    ID2D1DeviceContext* RenderTarget();
    IDWriteFactory* WriteFactory();
    IWICImagingFactory* WicFactory();

    ID2D1SolidColorBrush* CreateBrush(Graphics::Color color);
    D2D1_COLOR_F ToD2DColor(Graphics::Color color);
    std::uint32_t ColorKey(Graphics::Color color);
    std::string FormatAdapterInfo(IDXGIAdapter* adapter, const char* backendName);
    bool IsDeviceLost(HRESULT hr);
    UINT SwapChainFlags();
    const wchar_t* ImplementationDllName();
    const char* ImplementationLabel();

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
