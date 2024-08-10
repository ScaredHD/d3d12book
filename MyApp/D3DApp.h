#pragma once

#include <map>

#include "App.h"
#include "Timer.h"

#include "Common/d3dUtil.h"

#include "DescriptorHeap.h"
#include "SwapChain.h"

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp : public MyApp {
  public:
    D3DApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : MyApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

    void Initialize(const wchar_t* windowName = L"D3D App");

    virtual bool InitializeD3D();

    void Update();

    virtual void Draw() = 0;

  protected:
    virtual void OnInitialize() = 0;
    virtual void OnUpdate() = 0;

    void DefaultDraw();

    void OnResize() override;

    void UpdateViewport();

    void SetViewportAndScissorRects();

    void ExecuteCommandList();

    void CreateCommandObjects();

    void CreateSwapChain();

    void CreateDsvHeaps();

    void FlushCommandQueue();

    void CreateDepthStencilBuffer();

    void CreateDepthStencilView();

    void Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;

    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::unique_ptr<SwapChain<2>> swapChain_;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_;
    DXGI_FORMAT depthStencilFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
    std::unique_ptr<DescriptorHeap> dsvHeap_;

    bool isPaused_ = false;

    UINT64 nextFenceValue_ = 0;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;

    UINT msaa4XQuality_ = {};
    bool msaa4XEnabled_ = false;

    D3D12_VIEWPORT viewport_ = {};
    RECT scissorRect_ = {};

    // Game stats
    void CalculateFrameStats();

    Timer timer_;
    int frameCount_ = 0;
    float timeElapsed_ = 0.0f;
};
