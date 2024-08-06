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

    void Initialize();

    virtual bool InitializeD3D();

    void Update();

    virtual void Draw();

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

  protected:
    virtual void OnInitialize() {}

    virtual void OnUpdate() {}

    void OnMouseDown(int xPos, int yPos) override;

    void OnMouseUp(int xPos, int yPos) override;

    void OnKeyDown() override;

    void OnKeyUp() override;

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

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> device;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::unique_ptr<SwapChain<2>> swapChain_;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_;
    DXGI_FORMAT depthStencilFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
    std::unique_ptr<DescriptorHeap> dsvHeap_;

    bool isPaused = false;

    UINT64 nextFence = 0;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;

    bool Msaa4xEnabled() const { return msaa4xEnabled; }

    void SetMsaa4x(bool state);

    UINT GetMsaa4xQuality() const { return msaa4xQuality; }

    UINT msaa4xQuality = {};
    bool msaa4xEnabled = false;

    D3D12_VIEWPORT viewport = {};
    RECT scissorRect = {};

    // Game stats
    void CalculateFrameStats();

    MyTimer timer;
    int frameCount = 0;
    float timeElapsed = 0.0f;
};
