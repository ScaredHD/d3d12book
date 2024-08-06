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

    DXGI_FORMAT GetDepthStencilFormat() const { return depthStencilFormat; }

    bool Msaa4xEnabled() const { return msaa4xEnabled; }

    void SetMsaa4x(bool state);

    UINT GetMsaa4xQuality() const { return msaa4xQuality; }

    // Game stats
    void CalculateFrameStats();

    MyTimer timer;
    int frameCount = 0;
    float timeElapsed = 0.0f;

    ID3D12Device* GetDevice() const { return d3dDevice.Get(); }

    ID3D12GraphicsCommandList* GetCommandList() const { return commandList.Get(); }

    ID3D12CommandAllocator* GetCommandAllocator() const { return commandAllocator.Get(); }

    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue.Get(); }

    ID3D12Fence* GetFence() const { return fence.Get(); }

    UINT64 GetFenceValue() const { return fence->GetCompletedValue(); }

    UINT64 nextFence = 0;

    SwapChain<2>* GetSwapChain() const { return swapChain_.get(); }

    ID3D12Resource* GetDepthStencilBuffer() const { return depthStencilBuffer_.Get(); }

    bool isPaused = false;

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;

    DXGI_FORMAT backBufferFormat_ = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::unique_ptr<SwapChain<2>> swapChain_;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer_;
    DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    std::unique_ptr<DescriptorHeap> dsvHeap_;

    UINT msaa4xQuality = {};
    bool msaa4xEnabled = false;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    D3D12_VIEWPORT viewport = {};
    RECT scissorRect = {};
};
