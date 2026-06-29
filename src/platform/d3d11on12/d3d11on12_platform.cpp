//
// D3D11On12 parasite backend for the Direct2D renderer.
//

#include "d3d11on12_platform.hpp"

#include <algorithm>

#include <d2d1_1helper.h>
#include <d2d1helper.h>
#include <WRL/client.h>

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

namespace Madokawaii::Platform::D3D11On12Backend {
    namespace {
        RenderState gState{};

        void CheckTearingSupport() {
            auto& windowState = Direct2D::GetState();

            Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
            HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory4));
            BOOL allowTearing = FALSE;
            if (SUCCEEDED(hr)) {
                Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
                hr = factory4.As(&factory5);
                if (SUCCEEDED(hr)) {
                    hr = factory5->CheckFeatureSupport(
                        DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                        &allowTearing,
                        sizeof(allowTearing));
                }
            }
            windowState.isTearingSupport = SUCCEEDED(hr) && allowTearing;
        }

        HRESULT CreateD3D12Device(
            IDXGIFactory2* factory,
            ID3D12Device** outDevice,
            IDXGIAdapter** outAdapter) {
            if (!factory || !outDevice) return E_INVALIDARG;
            *outDevice = nullptr;
            if (outAdapter) *outAdapter = nullptr;

            for (UINT adapterIndex = 0;; ++adapterIndex) {
                IDXGIAdapter1* adapter{};
                const HRESULT enumResult = factory->EnumAdapters1(adapterIndex, &adapter);
                if (enumResult == DXGI_ERROR_NOT_FOUND) break;
                if (FAILED(enumResult)) return enumResult;

                DXGI_ADAPTER_DESC1 desc{};
                if (SUCCEEDED(adapter->GetDesc1(&desc))
                    && (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0
                    && SUCCEEDED(D3D12CreateDevice(
                        adapter,
                        D3D_FEATURE_LEVEL_11_0,
                        IID_PPV_ARGS(outDevice)))) {
                    if (outAdapter) {
                        *outAdapter = adapter;
                    } else {
                        Direct2D::SafeRelease(adapter);
                    }
                    return S_OK;
                }

                Direct2D::SafeRelease(adapter);
            }

            IDXGIFactory4* factory4{};
            if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory4)))) {
                IDXGIAdapter* warpAdapter{};
                if (SUCCEEDED(factory4->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)))) {
                    const HRESULT hr = D3D12CreateDevice(
                        warpAdapter,
                        D3D_FEATURE_LEVEL_11_0,
                        IID_PPV_ARGS(outDevice));
                    if (SUCCEEDED(hr)) {
                        if (outAdapter) {
                            *outAdapter = warpAdapter;
                        } else {
                            Direct2D::SafeRelease(warpAdapter);
                        }
                        Direct2D::SafeRelease(factory4);
                        return S_OK;
                    }
                    Direct2D::SafeRelease(warpAdapter);
                }
                Direct2D::SafeRelease(factory4);
            }

            return E_FAIL;
        }

        void WaitForGpu(RenderState& state) {
            if (!state.d3d12CommandQueue || !state.fence || !state.fenceEvent) return;

            const auto fenceValue = ++state.fenceValue;
            if (FAILED(state.d3d12CommandQueue->Signal(state.fence, fenceValue))) return;

            if (state.fence->GetCompletedValue() < fenceValue
                && SUCCEEDED(state.fence->SetEventOnCompletion(fenceValue, state.fenceEvent))) {
                WaitForSingleObject(state.fenceEvent, INFINITE);
            }
        }

        void ReleaseWrappedBackBuffer(RenderState& state) {
            if (!state.wrappedBackBufferAcquired || !state.d3d11On12Device) return;

            auto* wrappedBackBuffer = state.backBufferIndex < Direct2D::SwapChainBufferCount
                ? state.wrappedBackBuffers[state.backBufferIndex]
                : nullptr;
            if (wrappedBackBuffer) {
                state.d3d11On12Device->ReleaseWrappedResources(&wrappedBackBuffer, 1);
            }
            state.wrappedBackBufferAcquired = false;
        }

        bool AcquireWrappedBackBuffer(Direct2D::PlatformState& windowState, RenderState& state) {
            if (!state.swapChain3 || !state.d3d11On12Device || !windowState.renderTarget) return false;

            state.backBufferIndex = state.swapChain3->GetCurrentBackBufferIndex();
            if (state.backBufferIndex >= Direct2D::SwapChainBufferCount
                || !state.wrappedBackBuffers[state.backBufferIndex]
                || !state.d2dBackBufferTargets[state.backBufferIndex]) {
                return false;
            }

            auto* wrappedBackBuffer = state.wrappedBackBuffers[state.backBufferIndex];
            state.d3d11On12Device->AcquireWrappedResources(&wrappedBackBuffer, 1);
            state.wrappedBackBufferAcquired = true;
            windowState.targetBitmap = state.d2dBackBufferTargets[state.backBufferIndex];
            windowState.renderTarget->SetTarget(windowState.targetBitmap);
            return true;
        }

        void ReleaseSwapChainTarget(Direct2D::PlatformState& windowState) {
            auto& state = GetState();
            ReleaseWrappedBackBuffer(state);

            if (windowState.renderTarget) {
                windowState.renderTarget->SetTarget(nullptr);
            }
            windowState.targetBitmap = nullptr;

            for (auto*& target : state.d2dBackBufferTargets) {
                Direct2D::SafeRelease(target);
            }

            if (windowState.d3dContext) {
                windowState.d3dContext->Flush();
            }
            WaitForGpu(state);

            for (auto*& wrappedBackBuffer : state.wrappedBackBuffers) {
                Direct2D::SafeRelease(wrappedBackBuffer);
            }
            for (auto*& backBuffer : state.d3d12BackBuffers) {
                Direct2D::SafeRelease(backBuffer);
            }
        }

        void ReleaseDeviceResources(Direct2D::PlatformState& windowState) {
            auto& state = GetState();
            ReleaseSwapChainTarget(windowState);
            Direct2D::SafeRelease(state.swapChain3);
            Direct2D::SafeRelease(windowState.swapChain);
            Direct2D::SafeRelease(windowState.renderTarget);
            Direct2D::SafeRelease(windowState.d2dDevice);
            Direct2D::SafeRelease(windowState.dxgiFactory);
            Direct2D::SafeRelease(state.d3d11On12Device);
            Direct2D::SafeRelease(windowState.d3dContext);
            Direct2D::SafeRelease(windowState.d3dDevice);
            Direct2D::SafeRelease(state.d3d12CommandQueue);
            Direct2D::SafeRelease(state.d3d12Device);
            Direct2D::SafeRelease(state.fence);
            if (state.fenceEvent) {
                CloseHandle(state.fenceEvent);
                state.fenceEvent = nullptr;
            }
            state.fenceValue = 0;
            state.backBufferIndex = 0;
            state.wrappedBackBufferAcquired = false;
            windowState.featureLevel = {};
            windowState.implementerInfo.clear();
        }

        bool EnsureDeviceResources(Direct2D::PlatformState& windowState) {
            auto& state = GetState();
            if (state.d3d12Device
                && state.d3d12CommandQueue
                && state.d3d11On12Device
                && windowState.d3dDevice
                && windowState.d3dContext
                && windowState.dxgiFactory
                && windowState.d2dDevice
                && windowState.renderTarget) {
                return true;
            }

            ReleaseDeviceResources(windowState);
            if (!Direct2D::EnsureFactories()) return false;
            CheckTearingSupport();

            IDXGIAdapter* adapter{};
            IDXGIDevice* dxgiDevice{};
            bool ok = false;

            if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&windowState.dxgiFactory)))
                && SUCCEEDED(CreateD3D12Device(windowState.dxgiFactory, &state.d3d12Device, &adapter))) {
                D3D12_COMMAND_QUEUE_DESC queueDesc{};
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

                if (SUCCEEDED(state.d3d12Device->CreateCommandQueue(
                        &queueDesc,
                        IID_PPV_ARGS(&state.d3d12CommandQueue)))) {
                    IUnknown* commandQueues[] = {state.d3d12CommandQueue};
                    ok = SUCCEEDED(D3D11On12CreateDevice(
                            state.d3d12Device,
                            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                            nullptr,
                            0,
                            commandQueues,
                            ARRAYSIZE(commandQueues),
                            0,
                            &windowState.d3dDevice,
                            &windowState.d3dContext,
                            &windowState.featureLevel))
                        && SUCCEEDED(windowState.d3dDevice->QueryInterface(IID_PPV_ARGS(&state.d3d11On12Device)))
                        && SUCCEEDED(state.d3d11On12Device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))
                        && SUCCEEDED(windowState.d2dFactory->CreateDevice(dxgiDevice, &windowState.d2dDevice))
                        && SUCCEEDED(windowState.d2dDevice->CreateDeviceContext(
                            D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                            &windowState.renderTarget))
                        && SUCCEEDED(state.d3d12Device->CreateFence(
                            0,
                            D3D12_FENCE_FLAG_NONE,
                            IID_PPV_ARGS(&state.fence)));
                }
            }

            if (ok) {
                state.fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
                ok = state.fenceEvent != nullptr;
            }

            if (!ok) {
                Direct2D::SafeRelease(dxgiDevice);
                Direct2D::SafeRelease(adapter);
                ReleaseDeviceResources(windowState);
                return false;
            }

            windowState.implementerInfo = Direct2D::FormatAdapterInfo(adapter, "D3D11On12");
            Direct2D::SafeRelease(dxgiDevice);
            Direct2D::SafeRelease(adapter);
            windowState.renderTarget->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
            return true;
        }

        bool EnsureSwapChain(Direct2D::PlatformState& windowState) {
            auto& state = GetState();
            if (windowState.swapChain) return true;
            if (!windowState.window || !EnsureDeviceResources(windowState)) return false;

            DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
            swapChainDesc.Width = static_cast<UINT>(std::max(1, windowState.screenWidth));
            swapChainDesc.Height = static_cast<UINT>(std::max(1, windowState.screenHeight));
            swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            swapChainDesc.Stereo = FALSE;
            swapChainDesc.SampleDesc.Count = 1;
            swapChainDesc.SampleDesc.Quality = 0;
            swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            swapChainDesc.BufferCount = Direct2D::SwapChainBufferCount;
            swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
            swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
            swapChainDesc.Flags = Direct2D::SwapChainFlags();

            HRESULT hr = windowState.dxgiFactory->CreateSwapChainForHwnd(
                state.d3d12CommandQueue,
                windowState.window,
                &swapChainDesc,
                nullptr,
                nullptr,
                &windowState.swapChain);

            if (FAILED(hr)) {
                Direct2D::SafeRelease(windowState.swapChain);
                swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
                hr = windowState.dxgiFactory->CreateSwapChainForHwnd(
                    state.d3d12CommandQueue,
                    windowState.window,
                    &swapChainDesc,
                    nullptr,
                    nullptr,
                    &windowState.swapChain);
            }

            if (FAILED(hr)) {
                Direct2D::SafeRelease(windowState.swapChain);
                return false;
            }

            if (FAILED(windowState.swapChain->QueryInterface(IID_PPV_ARGS(&state.swapChain3)))) {
                Direct2D::SafeRelease(windowState.swapChain);
                return false;
            }
            state.backBufferIndex = state.swapChain3->GetCurrentBackBufferIndex();
            windowState.dxgiFactory->MakeWindowAssociation(windowState.window, DXGI_MWA_NO_ALT_ENTER);
            return true;
        }

        bool EnsureSwapChainTarget(Direct2D::PlatformState& windowState) {
            auto& state = GetState();
            if (state.d2dBackBufferTargets[0]) return true;
            if (!EnsureSwapChain(windowState)) return false;

            const auto bitmapProperties = D2D1::BitmapProperties1(
                D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
                96.0f,
                96.0f);

            for (UINT i = 0; i < Direct2D::SwapChainBufferCount; ++i) {
                if (FAILED(windowState.swapChain->GetBuffer(i, IID_PPV_ARGS(&state.d3d12BackBuffers[i])))) {
                    ReleaseSwapChainTarget(windowState);
                    return false;
                }

                D3D11_RESOURCE_FLAGS d3d11Flags{};
                d3d11Flags.BindFlags = D3D11_BIND_RENDER_TARGET;
                if (FAILED(state.d3d11On12Device->CreateWrappedResource(
                        state.d3d12BackBuffers[i],
                        &d3d11Flags,
                        D3D12_RESOURCE_STATE_PRESENT,
                        D3D12_RESOURCE_STATE_PRESENT,
                        IID_PPV_ARGS(&state.wrappedBackBuffers[i])))) {
                    ReleaseSwapChainTarget(windowState);
                    return false;
                }

                IDXGISurface* surface{};
                const HRESULT hr = state.wrappedBackBuffers[i]->QueryInterface(IID_PPV_ARGS(&surface));
                if (FAILED(hr)
                    || FAILED(windowState.renderTarget->CreateBitmapFromDxgiSurface(
                        surface,
                        &bitmapProperties,
                        &state.d2dBackBufferTargets[i]))) {
                    Direct2D::SafeRelease(surface);
                    ReleaseSwapChainTarget(windowState);
                    return false;
                }
                Direct2D::SafeRelease(surface);
            }

            state.backBufferIndex = state.swapChain3->GetCurrentBackBufferIndex();
            windowState.targetBitmap = state.d2dBackBufferTargets[state.backBufferIndex];
            windowState.renderTarget->SetTarget(windowState.targetBitmap);
            windowState.renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            windowState.renderTarget->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            windowState.currentTransform = D2D1::Matrix3x2F::Identity();
            windowState.renderTarget->SetTransform(windowState.currentTransform);
            return true;
        }

        bool BeginFrame(Direct2D::PlatformState& windowState) {
            return AcquireWrappedBackBuffer(windowState, GetState());
        }

        void EndFrameBeforePresent(Direct2D::PlatformState& windowState) {
            auto& state = GetState();
            ReleaseWrappedBackBuffer(state);
            if (windowState.d3dContext) {
                windowState.d3dContext->Flush();
            }
        }

        void AfterPresent(Direct2D::PlatformState&, HRESULT) {
            auto& state = GetState();
            if (state.swapChain3) {
                state.backBufferIndex = state.swapChain3->GetCurrentBackBufferIndex();
            }
        }

        void ResizeRenderTarget(Direct2D::PlatformState& windowState, int width, int height) {
            windowState.screenWidth = std::max(1, width);
            windowState.screenHeight = std::max(1, height);

            if (!windowState.swapChain) {
                return;
            }

            ReleaseSwapChainTarget(windowState);
            const HRESULT hr = windowState.swapChain->ResizeBuffers(
                0,
                static_cast<UINT>(windowState.screenWidth),
                static_cast<UINT>(windowState.screenHeight),
                DXGI_FORMAT_UNKNOWN,
                Direct2D::SwapChainFlags());

            if (Direct2D::IsDeviceLost(hr)) {
                ReleaseDeviceResources(windowState);
            } else if (SUCCEEDED(hr)) {
                EnsureSwapChainTarget(windowState);
            }
        }

        const wchar_t* ImplementationDllName() {
            return L"d3d11on12.dll";
        }

        const char* ImplementationLabel() {
            return "D3D11On12 (DirectX 12)";
        }

        struct BackendRegistrar {
            BackendRegistrar() {
                Direct2D::SetRenderBackend(&RenderBackend());
            }
        };

        BackendRegistrar gRegistrar{};
    }

    RenderState& GetState() {
        return gState;
    }

    const Direct2D::RenderBackend& RenderBackend() {
        static const Direct2D::RenderBackend backend{
            EnsureSwapChainTarget,
            ReleaseSwapChainTarget,
            ReleaseDeviceResources,
            BeginFrame,
            EndFrameBeforePresent,
            AfterPresent,
            ResizeRenderTarget,
            ImplementationDllName,
            ImplementationLabel
        };
        return backend;
    }
}
