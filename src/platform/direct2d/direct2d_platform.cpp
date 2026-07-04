//
// Direct2D backend shared state and helpers.
//

#include "direct2d_platform.hpp"

#include <algorithm>
#include <format>
#include <thread>

#include <d2d1_1helper.h>
#include <d2d1helper.h>
#include <windowsx.h>
#include <WRL/client.h>

// The required symbols are in dxgi1_5.h. Developers can define those symbols if they are missing in their SDK.

#if defined(__has_include)
#if __has_include(<dxgi1_5.h>)
#include <dxgi1_5.h>
#endif
#endif

#ifndef DXGI_PRESENT_ALLOW_TEARING
#define DXGI_PRESENT_ALLOW_TEARING          0x00000200UL
#define DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING  2048

typedef
enum DXGI_FEATURE
{
    DXGI_FEATURE_PRESENT_ALLOW_TEARING = 0
} DXGI_FEATURE;

MIDL_INTERFACE("7632e1f5-ee65-4dca-87fd-84cd75f8838d")
IDXGIFactory5 : public IDXGIFactory4
{
    public:
    virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
                       DXGI_FEATURE Feature,
                      _Inout_updates_bytes_(FeatureSupportDataSize) void *pFeatureSupportData,
                       UINT FeatureSupportDataSize) = 0;
};
#endif


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

        std::uint8_t EffectiveAlpha(Graphics::Color color) {
            return color.a;
        }

        void CheckTearingSupport() {
            PlatformState& state = GetState();

            Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
            HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));
            BOOL allowTearing = FALSE;
            if (SUCCEEDED(hr))
            {
                Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
                hr = factory4.As(&factory5);
                if (SUCCEEDED(hr))
                {
                    hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
                }
            }
            state.isTearingSupport = SUCCEEDED(hr) && allowTearing;
        }

        UINT CurrentSwapChainFlags() {
            return GetState().isTearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
        }

        UINT CurrentPresentFlags() {
            const auto& state = GetState();
            return (state.isTearingSupport && !state.fullscreen) ? DXGI_PRESENT_ALLOW_TEARING : 0;
        }

        bool IsDeviceLostResult(HRESULT hr) {
            return hr == DXGI_ERROR_DEVICE_REMOVED
                || hr == DXGI_ERROR_DEVICE_RESET
                || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR;
        }

        std::string DescribeAdapter(IDXGIAdapter* adapter, const char* backendName) {
            if (!adapter) {
                return std::format("{} (DXGI adapter unavailable)", backendName);
            }

            DXGI_ADAPTER_DESC desc{};
            const auto hr = adapter->GetDesc(&desc);
            if (FAILED(hr)) {
                return std::format("{} (DXGI adapter info unavailable)", backendName);
            }

            const auto description = WideToUtf8(desc.Description);
            if (description.empty()) {
                return std::format("{} (unnamed DXGI adapter)", backendName);
            }

            return std::format(
                "{} on {} (vendor 0x{:04X}, device 0x{:04X})",
                backendName,
                description,
                desc.VendorId,
                desc.DeviceId);
        }

        HRESULT CreateD3DDevice(
            ID3D11Device** outDevice,
            D3D_FEATURE_LEVEL* outFeatureLevel,
            ID3D11DeviceContext** outContext) {
            const D3D_FEATURE_LEVEL featureLevels[] = {
                D3D_FEATURE_LEVEL_11_1,
                D3D_FEATURE_LEVEL_11_0,
                D3D_FEATURE_LEVEL_10_1,
                D3D_FEATURE_LEVEL_10_0,
                D3D_FEATURE_LEVEL_9_3,
                D3D_FEATURE_LEVEL_9_2,
                D3D_FEATURE_LEVEL_9_1,
            };

            constexpr UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

            HRESULT hr = D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                deviceFlags,
                featureLevels,
                ARRAYSIZE(featureLevels),
                D3D11_SDK_VERSION,
                outDevice,
                outFeatureLevel,
                outContext);

            if (hr == E_INVALIDARG) {
                hr = D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_HARDWARE,
                    nullptr,
                    deviceFlags,
                    featureLevels + 1,
                    ARRAYSIZE(featureLevels) - 1,
                    D3D11_SDK_VERSION,
                    outDevice,
                    outFeatureLevel,
                    outContext);
            }

            if (FAILED(hr)) {
                SafeRelease(*outContext);
                SafeRelease(*outDevice);

                hr = D3D11CreateDevice(
                    nullptr,
                    D3D_DRIVER_TYPE_WARP,
                    nullptr,
                    deviceFlags,
                    featureLevels + 1,
                    ARRAYSIZE(featureLevels) - 1,
                    D3D11_SDK_VERSION,
                    outDevice,
                    outFeatureLevel,
                    outContext);
            }

            CheckTearingSupport();
            return hr;
        }

        void ReleaseSwapChainTarget(PlatformState& state) {
            if (state.renderTarget) {
                state.renderTarget->SetTarget(nullptr);
            }
            SafeRelease(state.targetBitmap);
        }

        void ReleaseDeviceResources(PlatformState& state) {
            ReleaseSwapChainTarget(state);
            SafeRelease(state.swapChain);
            SafeRelease(state.renderTarget);
            SafeRelease(state.d2dDevice);
            SafeRelease(state.dxgiFactory);
            SafeRelease(state.d3dContext);
            SafeRelease(state.d3dDevice);
            state.featureLevel = {};
            state.implementerInfo.clear();
        }

        bool EnsureDeviceResources() {
            auto& state = GetState();
            if (state.d3dDevice
                && state.d3dContext
                && state.dxgiFactory
                && state.d2dDevice
                && state.renderTarget) {
                return true;
            }

            ReleaseDeviceResources(state);
            if (!EnsureFactories()) return false;

            if (FAILED(CreateD3DDevice(&state.d3dDevice, &state.featureLevel, &state.d3dContext))) {
                ReleaseDeviceResources(state);
                return false;
            }

            IDXGIDevice* dxgiDevice{};
            IDXGIAdapter* adapter{};
            if (FAILED(state.d3dDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))
                || FAILED(dxgiDevice->GetAdapter(&adapter))
                || FAILED(adapter->GetParent(IID_PPV_ARGS(&state.dxgiFactory)))
                || FAILED(state.d2dFactory->CreateDevice(dxgiDevice, &state.d2dDevice))
                || FAILED(state.d2dDevice->CreateDeviceContext(
                    D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                    &state.renderTarget))) {
                SafeRelease(adapter);
                SafeRelease(dxgiDevice);
                ReleaseDeviceResources(state);
                return false;
            }

            std::string implementer;
            if (!dxgiDevice) implementer = "Direct2D (DXGI device unavailable)";
            else {
                implementer = DescribeAdapter(adapter, "Direct2D");
            }

            state.implementerInfo = implementer;
            SafeRelease(adapter);
            SafeRelease(dxgiDevice);
            state.renderTarget->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
            return true;
        }

        bool EnsureSwapChain() {
            auto& state = GetState();
            if (state.swapChain) return true;
            if (!state.window || !EnsureDeviceResources()) return false;

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
            swapChainDesc.Width = static_cast<UINT>(std::max(1, state.screenWidth));
            swapChainDesc.Height = static_cast<UINT>(std::max(1, state.screenHeight));
            swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = SwapChainBufferCount;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = CurrentSwapChainFlags();

            HRESULT hr = state.dxgiFactory->CreateSwapChainForHwnd(
                state.d3dDevice,
                state.window,
                &swapChainDesc,
                nullptr,
                nullptr,
                &state.swapChain);

            if (FAILED(hr)) {
                SafeRelease(state.swapChain);
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                hr = state.dxgiFactory->CreateSwapChainForHwnd(
                    state.d3dDevice,
                    state.window,
                    &swapChainDesc,
                    nullptr,
                    nullptr,
                    &state.swapChain);
            }

            if (FAILED(hr)) {
                SafeRelease(state.swapChain);
                return false;
            }

            state.dxgiFactory->MakeWindowAssociation(state.window, DXGI_MWA_NO_ALT_ENTER);
            return true;
        }

        bool EnsureSwapChainTarget() {
            auto& state = GetState();
            if (state.targetBitmap) return true;
            if (!EnsureSwapChain()) return false;

            IDXGISurface* backBuffer{};
            if (FAILED(state.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)))) {
                return false;
            }

            const auto bitmapProperties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
                96.0f,
                96.0f);

            const HRESULT hr = state.renderTarget->CreateBitmapFromDxgiSurface(
                backBuffer,
                &bitmapProperties,
                &state.targetBitmap);
            SafeRelease(backBuffer);
            if (FAILED(hr)) return false;

            state.renderTarget->SetTarget(state.targetBitmap);
            state.renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            state.renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            state.currentTransform = D2D1::Matrix3x2F::Identity();
            state.renderTarget->SetTransform(state.currentTransform);
            return true;
        }

        bool BeginNativeFrame(PlatformState&) {
            return true;
        }

        bool EnsureNativeRenderTarget(PlatformState&) {
            return EnsureSwapChainTarget();
        }

        void EndNativeFrameBeforePresent(PlatformState&) {
        }

        void AfterNativePresent(PlatformState&, HRESULT) {
        }

        void ResizeNativeRenderTarget(PlatformState& state, int width, int height) {
            state.screenWidth = std::max(1, width);
            state.screenHeight = std::max(1, height);

            if (!state.swapChain) {
                return;
            }

            ReleaseSwapChainTarget(state);
            const HRESULT hr = state.swapChain->ResizeBuffers(
                0,
                static_cast<UINT>(state.screenWidth),
                static_cast<UINT>(state.screenHeight),
                DXGI_FORMAT_UNKNOWN,
                CurrentSwapChainFlags());

            if (IsDeviceLostResult(hr)) {
                ReleaseDeviceResources(state);
            } else if (SUCCEEDED(hr)) {
                EnsureSwapChainTarget();
            }
        }

        const wchar_t* NativeImplementationDllName() {
            return L"d2d1.dll";
        }

        const char* NativeImplementationLabel() {
            return "Direct2D (DirectX 11)";
        }

        const RenderBackend& NativeRenderBackend() {
            static const RenderBackend backend{
                EnsureNativeRenderTarget,
                ReleaseSwapChainTarget,
                ReleaseDeviceResources,
                BeginNativeFrame,
                EndNativeFrameBeforePresent,
                AfterNativePresent,
                ResizeNativeRenderTarget,
                NativeImplementationDllName,
                NativeImplementationLabel
            };
            return backend;
        }

        const RenderBackend*& ActiveRenderBackendSlot() {
            static const RenderBackend* backend = &NativeRenderBackend();
            return backend;
        }

        const RenderBackend& ActiveRenderBackend() {
            return *ActiveRenderBackendSlot();
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

    void SetRenderBackend(const RenderBackend* backend) {
        ActiveRenderBackendSlot() = backend ? backend : &NativeRenderBackend();
    }

    std::string FormatAdapterInfo(IDXGIAdapter* adapter, const char* backendName) {
        return DescribeAdapter(adapter, backendName);
    }

    bool IsDeviceLost(HRESULT hr) {
        return IsDeviceLostResult(hr);
    }

    UINT SwapChainFlags() {
        return CurrentSwapChainFlags();
    }

    const wchar_t* ImplementationDllName() {
        return ActiveRenderBackend().implementationDllName();
    }

    const char* ImplementationLabel() {
        return ActiveRenderBackend().implementationLabel();
    }

    bool EnsureFactories() {
        auto& state = GetState();

        if (!state.d2dFactory) {
            if (FAILED(D2D1CreateFactory(
                    D2D1_FACTORY_TYPE_SINGLE_THREADED,
                    __uuidof(ID2D1Factory1),
                    nullptr,
                    reinterpret_cast<void**>(&state.d2dFactory)))) {
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
        if (!state.window) return false;

        RECT rc{};
        GetClientRect(state.window, &rc);
        state.screenWidth = static_cast<int>(std::max<LONG>(1, rc.right - rc.left));
        state.screenHeight = static_cast<int>(std::max<LONG>(1, rc.bottom - rc.top));

        return ActiveRenderBackend().ensureRenderTarget(state);
    }

    ID2D1DeviceContext* RenderTarget() {
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

        return true;
    }

    void ShutdownWindow() {
        auto& state = GetState();

        ActiveRenderBackend().releaseDeviceResources(state);
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
        if (!ActiveRenderBackend().beginFrame(state)) return;

        state.drawing = true;
        state.transformStack.clear();
        state.currentTransform = D2D1::Matrix3x2F::Identity();
        state.renderTarget->SetTransform(state.currentTransform);
        state.renderTarget->BeginDraw();
    }

    void EndFrame() {
        auto& state = GetState();
        if (!state.renderTarget || !state.swapChain || !state.drawing) {
            ResetTransientInput();
            return;
        }

        const auto beforePresent = std::chrono::steady_clock::now();
        const HRESULT drawResult = state.renderTarget->EndDraw();
        state.drawing = false;

        ActiveRenderBackend().endFrameBeforePresent(state);

        HRESULT presentResult = S_OK;
        if (SUCCEEDED(drawResult)) {
            presentResult = state.swapChain->Present(0, CurrentPresentFlags());
        }

        if (drawResult == D2DERR_RECREATE_TARGET) {
            ActiveRenderBackend().releaseSwapChainTarget(state);
        } else if (IsDeviceLostResult(drawResult) || IsDeviceLostResult(presentResult)) {
            ActiveRenderBackend().releaseDeviceResources(state);
        } else {
            ActiveRenderBackend().afterPresent(state, presentResult);
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
        ActiveRenderBackend().resizeRenderTarget(state, width, height);
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
