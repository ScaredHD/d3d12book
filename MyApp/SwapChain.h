#pragma once

#include <queue>

#include "Common/d3dUtil.h"

#include "DescriptorHeap.h"

template <int bufferCount>
class SwapChain {
  public:
    static int BufferCount() { return bufferCount; }

    explicit SwapChain(ID3D12Device* device,
                       IDXGIFactory* dxgiFactory,
                       ID3D12CommandQueue* commandQueue,
                       DXGI_SWAP_CHAIN_DESC desc) {
        ThrowIfFailed(
            dxgiFactory->CreateSwapChain(commandQueue, &desc, dxgiSwapChain_.GetAddressOf()));

        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = bufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        rtvHeapDesc.NodeMask = 0;
        rtvHeap_ = std::make_unique<DescriptorHeap>(device, rtvHeapDesc);

        CreateRtvs(device);
    }

    void CreateRtvs(ID3D12Device* device) {
        for (int i = 0; i < bufferCount; ++i) {
            ThrowIfFailed(
                dxgiSwapChain_->GetBuffer(i, IID_PPV_ARGS(buffers_[i].ReleaseAndGetAddressOf())));

            device->CreateRenderTargetView(buffers_[i].Get(), nullptr, rtvHeap_->GetDescriptorHandleCpu(i));
        }
    }

    void ResetDxgiSwapChain() { dxgiSwapChain_.Reset(); }

    void ResetAllBuffers() {
        for (auto& buf : buffers_) {
            buf.Reset();
        }
    }

    HRESULT Present() const { return dxgiSwapChain_->Present(0, 0); }

    DXGI_FORMAT GetFormat() const { return backBufferFormat_; }

    void SwapBuffers() { currentBackBuffer_ = (currentBackBuffer_ + 1) % bufferCount; }

    ID3D12Resource* GetCurrentBackBuffer() const { return buffers_[currentBackBuffer_].Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const {
        return rtvHeap_->GetDescriptorHandleCpu(currentBackBuffer_);
    }

    HRESULT Resize(int clientWidth, int clientHeight) {
        return dxgiSwapChain_->ResizeBuffers(bufferCount,
                                             clientWidth,
                                             clientHeight,
                                             backBufferFormat_,
                                             DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
    }

  private:
    Microsoft::WRL::ComPtr<IDXGISwapChain> dxgiSwapChain_;

    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, bufferCount> buffers_;

    std::unique_ptr<DescriptorHeap> rtvHeap_;

    int currentBackBuffer_ = 0;
    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
};