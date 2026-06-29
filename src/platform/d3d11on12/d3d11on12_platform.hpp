#ifndef MADOKAWAII_D3D11ON12_PLATFORM_H
#define MADOKAWAII_D3D11ON12_PLATFORM_H

#include "../direct2d/direct2d_platform.hpp"

#include <d3d11on12.h>
#include <d3d12.h>

#include <cstdint>

namespace Madokawaii::Platform::D3D11On12Backend {

    struct RenderState {
        ID3D12Device* d3d12Device{};
        ID3D12CommandQueue* d3d12CommandQueue{};
        ID3D11On12Device* d3d11On12Device{};
        IDXGISwapChain3* swapChain3{};
        ID3D12Resource* d3d12BackBuffers[Direct2D::SwapChainBufferCount]{};
        ID3D11Resource* wrappedBackBuffers[Direct2D::SwapChainBufferCount]{};
        ID2D1Bitmap1* d2dBackBufferTargets[Direct2D::SwapChainBufferCount]{};
        ID3D12Fence* fence{};
        HANDLE fenceEvent{};
        std::uint64_t fenceValue{};
        UINT backBufferIndex{};
        bool wrappedBackBufferAcquired{};
    };

    RenderState& GetState();
    const Direct2D::RenderBackend& RenderBackend();

}

#endif // MADOKAWAII_D3D11ON12_PLATFORM_H
