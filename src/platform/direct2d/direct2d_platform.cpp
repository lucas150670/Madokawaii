//
// Direct2D backend shared state and helpers.
//

#include "direct2d_platform.hpp"

#include <algorithm>
#include <cstdio>
#include <thread>

#include <d2d1helper.h>
#include <windowsx.h>

namespace Madokawaii::Platform::Direct2D {
    namespace {
        constexpr wchar_t WindowClassName[] = L"MadokawaiiDirect2DWindow";

        PlatformState gState{};

        LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
            auto& state = GetState();

            switch (message) {
            case WM_CLOSE:
                state.shouldClose = true;
                DestroyWindow(hwnd);
                return 0;
            case WM_DESTROY:
                state.shouldClose = true;
                PostQuitMessage(0);
                return 0;
            case WM_SIZE:
                if (wParam != SIZE_MINIMIZED) {
                    const auto width = static_cast<int>(LOWORD(lParam));
                    const auto height = static_cast<int>(HIWORD(lParam));
                    ResizeRenderTarget(width, height);
                }
                return 0;
            case WM_MOUSEMOVE:
                state.mousePosition.x = GET_X_LPARAM(lParam);
                state.mousePosition.y = GET_Y_LPARAM(lParam);
                return 0;
            case WM_LBUTTONDOWN:
                state.mouseLeftDown = true;
                state.mouseLeftPressed = true;
                state.mousePosition.x = GET_X_LPARAM(lParam);
                state.mousePosition.y = GET_Y_LPARAM(lParam);
                SetCapture(hwnd);
                return 0;
            case WM_LBUTTONUP:
                state.mouseLeftDown = false;
                state.mouseLeftReleased = true;
                state.mousePosition.x = GET_X_LPARAM(lParam);
                state.mousePosition.y = GET_Y_LPARAM(lParam);
                ReleaseCapture();
                return 0;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                state.anyKeyPressed = true;
                if (wParam == VK_BACK) state.backspacePressed = true;
                if (wParam == VK_RETURN) state.enterPressed = true;
                return 0;
            case WM_CHAR:
                if (wParam >= 32 && wParam != 127) {
                    state.typedText.push_back(static_cast<wchar_t>(wParam));
                }
                return 0;
            default:
                return DefWindowProcW(hwnd, message, wParam, lParam);
            }
        }

        void RegisterWindowClass() {
            auto& state = GetState();
            if (state.windowClassRegistered) return;

            WNDCLASSEXW wc{};
            wc.cbSize = sizeof(wc);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WindowProc;
            wc.hInstance = state.instance;
            wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
            wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
            wc.lpszClassName = WindowClassName;

            RegisterClassExW(&wc);
            state.windowClassRegistered = true;
        }

        void UpdateFrameStats() {
            auto& state = GetState();
            const auto now = std::chrono::steady_clock::now();

            if (state.lastFrameTime.time_since_epoch().count() == 0) {
                state.frameTime = 1.0f / 60.0f;
                state.lastFrameTime = now;
                state.fpsWindowStart = now;
                state.fps = 60.0f;
                return;
            }

            state.frameTime = std::chrono::duration<float>(now - state.lastFrameTime).count();
            state.lastFrameTime = now;

            state.fpsFrameCounter++;
            const auto fpsWindowSeconds = std::chrono::duration<float>(now - state.fpsWindowStart).count();
            if (fpsWindowSeconds >= 0.5f) {
                state.fps = static_cast<float>(state.fpsFrameCounter) / fpsWindowSeconds;
                state.fpsFrameCounter = 0;
                state.fpsWindowStart = now;
            }
        }

        std::uint8_t EffectiveAlpha(Graphics::Color color) {
            return color.a;
        }
    }

    TextureData::~TextureData() {
        SafeRelease(bitmap);
        for (auto& [_, tintedBitmap] : tintedBitmaps) {
            SafeRelease(tintedBitmap);
        }
        tintedBitmaps.clear();
    }

    FontData::~FontData() {
        SafeRelease(face);
    }

    PlatformState& GetState() {
        return gState;
    }

    bool EnsureFactories() {
        auto& state = GetState();

        if (!state.d2dFactory) {
            if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &state.d2dFactory))) {
                return false;
            }
        }

        if (!state.writeFactory) {
            if (FAILED(DWriteCreateFactory(
                    DWRITE_FACTORY_TYPE_SHARED,
                    __uuidof(IDWriteFactory),
                    reinterpret_cast<IUnknown**>(&state.writeFactory)))) {
                return false;
            }
        }

        if (!state.wicFactory) {
            if (FAILED(CoCreateInstance(
                    CLSID_WICImagingFactory,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    IID_PPV_ARGS(&state.wicFactory)))) {
                return false;
            }
        }

        return true;
    }

    bool EnsureRenderTarget() {
        auto& state = GetState();
        if (state.renderTarget) return true;
        if (!state.window || !EnsureFactories()) return false;

        RECT rc{};
        GetClientRect(state.window, &rc);
        const auto size = D2D1::SizeU(
            static_cast<UINT32>(std::max<LONG>(1, rc.right - rc.left)),
            static_cast<UINT32>(std::max<LONG>(1, rc.bottom - rc.top)));

        const auto renderTargetProperties = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
            0.0f,
            0.0f,
            D2D1_RENDER_TARGET_USAGE_NONE,
            D2D1_FEATURE_LEVEL_DEFAULT);

        const auto hwndProperties = D2D1::HwndRenderTargetProperties(
            state.window,
            size,
            D2D1_PRESENT_OPTIONS_IMMEDIATELY);

        if (FAILED(state.d2dFactory->CreateHwndRenderTarget(
                renderTargetProperties,
                hwndProperties,
                &state.renderTarget))) {
            return false;
        }

        state.renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        state.renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        state.currentTransform = D2D1::Matrix3x2F::Identity();
        return true;
    }

    ID2D1HwndRenderTarget* RenderTarget() {
        return EnsureRenderTarget() ? GetState().renderTarget : nullptr;
    }

    IDWriteFactory* WriteFactory() {
        return EnsureFactories() ? GetState().writeFactory : nullptr;
    }

    IWICImagingFactory* WicFactory() {
        return EnsureFactories() ? GetState().wicFactory : nullptr;
    }

    bool InitializeWindow(int width, int height, const char* title) {
        auto& state = GetState();
        state.instance = GetModuleHandleW(nullptr);
        state.screenWidth = width;
        state.screenHeight = height;
        state.shouldClose = false;
        state.currentTransform = D2D1::Matrix3x2F::Identity();

        if (!EnsureFactories()) return false;
        RegisterWindowClass();

        const DWORD style = WS_OVERLAPPEDWINDOW;
        RECT rect{0, 0, width, height};
        AdjustWindowRect(&rect, style, FALSE);

        const auto titleWide = Utf8ToWide(title ? title : "Madokawaii");
        state.window = CreateWindowExW(
            0,
            WindowClassName,
            titleWide.c_str(),
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            state.instance,
            nullptr);

        if (!state.window) return false;

        state.windowedStyle = style;
        GetWindowRect(state.window, &state.windowedRect);
        ShowWindow(state.window, SW_SHOW);
        UpdateWindow(state.window);

        return EnsureRenderTarget();
    }

    void ShutdownWindow() {
        auto& state = GetState();

        SafeRelease(state.renderTarget);
        SafeRelease(state.wicFactory);
        SafeRelease(state.writeFactory);
        SafeRelease(state.d2dFactory);

        if (state.window) {
            DestroyWindow(state.window);
            state.window = nullptr;
        }
    }

    void PumpMessages() {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    void SetTargetFrameRate(int targetFps) {
        auto& state = GetState();
        state.targetFrameSeconds = targetFps > 0 ? 1.0f / static_cast<float>(targetFps) : 0.0f;
    }

    void BeginFrame() {
        auto& state = GetState();
        PumpMessages();
        UpdateFrameStats();

        if (!EnsureRenderTarget()) return;

        state.drawing = true;
        state.transformStack.clear();
        state.currentTransform = D2D1::Matrix3x2F::Identity();
        state.renderTarget->SetTransform(state.currentTransform);
        state.renderTarget->BeginDraw();
    }

    void EndFrame() {
        auto& state = GetState();
        if (!state.renderTarget || !state.drawing) {
            ResetTransientInput();
            return;
        }

        const auto beforePresent = std::chrono::steady_clock::now();
        const HRESULT hr = state.renderTarget->EndDraw();
        state.drawing = false;

        if (hr == D2DERR_RECREATE_TARGET) {
            SafeRelease(state.renderTarget);
        }

        ResetTransientInput();

        if (state.targetFrameSeconds > 0.0f) {
            const auto elapsed = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - beforePresent).count();
            const auto sleepSeconds = state.targetFrameSeconds - elapsed;
            if (sleepSeconds > 0.001f) {
                std::this_thread::sleep_for(std::chrono::duration<float>(sleepSeconds));
            }
        }
    }

    void ResizeRenderTarget(int width, int height) {
        auto& state = GetState();
        state.screenWidth = std::max(1, width);
        state.screenHeight = std::max(1, height);
        if (state.renderTarget) {
            state.renderTarget->Resize(D2D1::SizeU(
                static_cast<UINT32>(state.screenWidth),
                static_cast<UINT32>(state.screenHeight)));
        }
    }

    void ToggleFullscreenWindow() {
        auto& state = GetState();
        if (!state.window) return;

        state.fullscreen = !state.fullscreen;
        if (state.fullscreen) {
            state.windowedStyle = GetWindowLongW(state.window, GWL_STYLE);
            GetWindowRect(state.window, &state.windowedRect);

            MONITORINFO monitorInfo{sizeof(monitorInfo)};
            GetMonitorInfoW(MonitorFromWindow(state.window, MONITOR_DEFAULTTONEAREST), &monitorInfo);
            SetWindowLongW(state.window, GWL_STYLE, state.windowedStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(
                state.window,
                HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        } else {
            SetWindowLongW(state.window, GWL_STYLE, state.windowedStyle);
            SetWindowPos(
                state.window,
                nullptr,
                state.windowedRect.left,
                state.windowedRect.top,
                state.windowedRect.right - state.windowedRect.left,
                state.windowedRect.bottom - state.windowedRect.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }

    ID2D1SolidColorBrush* CreateBrush(Graphics::Color color) {
        auto* renderTarget = RenderTarget();
        if (!renderTarget) return nullptr;

        ID2D1SolidColorBrush* brush{};
        if (FAILED(renderTarget->CreateSolidColorBrush(ToD2DColor(color), &brush))) {
            return nullptr;
        }
        return brush;
    }

    D2D1_COLOR_F ToD2DColor(Graphics::Color color) {
        const auto alpha = EffectiveAlpha(color);
        return D2D1::ColorF(
            color.r / 255.0f,
            color.g / 255.0f,
            color.b / 255.0f,
            alpha / 255.0f);
    }

    std::uint32_t ColorKey(Graphics::Color color) {
        const auto alpha = EffectiveAlpha(color);
        return (static_cast<std::uint32_t>(color.r) << 24)
            | (static_cast<std::uint32_t>(color.g) << 16)
            | (static_cast<std::uint32_t>(color.b) << 8)
            | static_cast<std::uint32_t>(alpha);
    }

    std::wstring Utf8ToWide(const char* text) {
        if (!text || text[0] == '\0') return {};

        const int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        if (required <= 0) return {};

        std::wstring wide(static_cast<std::size_t>(required - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), required);
        return wide;
    }

    std::wstring Utf8ToWide(const std::string& text) {
        return Utf8ToWide(text.c_str());
    }

    std::string WideToUtf8(const std::wstring& text) {
        if (text.empty()) return {};

        const int required = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (required <= 0) return {};

        std::string utf8(static_cast<std::size_t>(required - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8.data(), required, nullptr, nullptr);
        return utf8;
    }

    bool IsMouseOver(Shape::Rectangle bounds) {
        const auto& state = GetState();
        const auto x = static_cast<float>(state.mousePosition.x);
        const auto y = static_cast<float>(state.mousePosition.y);
        return x >= bounds.x
            && x <= bounds.x + bounds.width
            && y >= bounds.y
            && y <= bounds.y + bounds.height;
    }

    bool IsLeftMousePressed() {
        return GetState().mouseLeftPressed;
    }

    bool IsLeftMouseReleased() {
        return GetState().mouseLeftReleased;
    }

    bool IsLeftMouseDown() {
        return GetState().mouseLeftDown;
    }

    void ResetTransientInput() {
        auto& state = GetState();
        state.mouseLeftPressed = false;
        state.mouseLeftReleased = false;
        state.anyKeyPressed = false;
        state.backspacePressed = false;
        state.enterPressed = false;
        state.typedText.clear();
    }

    HRESULT CreateBitmapFromPixels(
        const std::uint8_t* pixelsBgra,
        std::uint32_t width,
        std::uint32_t height,
        Graphics::Color tint,
        ID2D1Bitmap** outBitmap) {
        if (!outBitmap) return E_POINTER;
        *outBitmap = nullptr;

        auto* renderTarget = RenderTarget();
        if (!renderTarget || !pixelsBgra || width == 0 || height == 0) return E_INVALIDARG;

        const auto tintAlpha = static_cast<std::uint32_t>(ColorKey(tint) & 0xffu);
        std::vector<std::uint8_t> premultiplied(static_cast<std::size_t>(width) * height * 4);
        for (std::size_t i = 0; i < static_cast<std::size_t>(width) * height; ++i) {
            const auto srcB = static_cast<std::uint32_t>(pixelsBgra[i * 4 + 0]);
            const auto srcG = static_cast<std::uint32_t>(pixelsBgra[i * 4 + 1]);
            const auto srcR = static_cast<std::uint32_t>(pixelsBgra[i * 4 + 2]);
            const auto srcA = static_cast<std::uint32_t>(pixelsBgra[i * 4 + 3]);

            const auto outA = srcA * tintAlpha / 255u;
            const auto outR = srcR * tint.r / 255u;
            const auto outG = srcG * tint.g / 255u;
            const auto outB = srcB * tint.b / 255u;

            premultiplied[i * 4 + 0] = static_cast<std::uint8_t>(outB * outA / 255u);
            premultiplied[i * 4 + 1] = static_cast<std::uint8_t>(outG * outA / 255u);
            premultiplied[i * 4 + 2] = static_cast<std::uint8_t>(outR * outA / 255u);
            premultiplied[i * 4 + 3] = static_cast<std::uint8_t>(outA);
        }

        const auto bitmapProperties = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

        return renderTarget->CreateBitmap(
            D2D1::SizeU(width, height),
            premultiplied.data(),
            width * 4,
            bitmapProperties,
            outBitmap);
    }
}
