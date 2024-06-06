#pragma once

#include "MyApp.h"
#include "MyTimer.h"

#include "Common/d3dUtil.h"

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class MyD3DApp : public MyApp {
  public:
    MyD3DApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
        : MyApp(hInstance, hPrevInstance, pCmdLine, nCmdShow) {}

    void Initialize();

    virtual bool InitializeD3D();

    virtual void Update();

    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

  protected:
    virtual void OnInitialize() {}

    void OnMouseDown(int xPos, int yPos) override;

    void OnMouseUp(int xPos, int yPos) override;

    void OnKeyDown() override;

    void OnKeyUp() override;

    void OnResize() override;

    void CreateCommandObjects();

    void CreateSwapChain();

    void CreateDescriptorHeaps();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetStartSwapChainBufferView() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    void CreateBackBufferViews();

    void CreateDepthStencilView();

    void SetMsaa4x(bool state);

    void CalculateFrameStats();

  private:
    bool isPaused = false;

    Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> d3dDevice;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT rtvDescriptorSize;
    UINT dsvDescriptorSize;
    UINT cbvSrvDescriptorSize;

    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    UINT msaa4xQuality;
    bool msaa4xEnabled = false;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
    static constexpr int s_swapChainBufferCount = 2;
    int currBackBuffer = 0;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
    DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> swapChainBuffers[s_swapChainBufferCount];

    // Game stats
    MyTimer timer;
    int frameCount = 0;
    float timeElapsed = 0.0f;
};
