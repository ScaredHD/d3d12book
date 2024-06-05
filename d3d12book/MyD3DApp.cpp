#include "MyD3DApp.h"

using namespace Microsoft::WRL;

LRESULT MyD3DApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    return MyApp::HandleMessage(hwnd, msg, wParam, lParam);
}

void MyD3DApp::OnMouseUp(int xPos, int yPos) {
    MyApp::OnMouseUp(xPos, yPos);
}

void MyD3DApp::OnMouseDown(int xPos, int yPos) {
    MyApp::OnMouseDown(xPos, yPos);
}

void MyD3DApp::OnKeyDown() {
    if (IsKeyDown('S')) {
        MessageBox(GetWindowHandle(), L"test", L"test caption", 0);
        OutputDebugString(L"S key is pressed\n");
    }
}

void MyD3DApp::OnKeyUp() {
    MyApp::OnKeyUp();
}

void MyD3DApp::OnResize() {
    CreateBackBufferViews();
    CreateDepthStencilView();
    MyApp::OnResize();
}

bool MyD3DApp::InitializeD3D() {
#if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.ReleaseAndGetAddressOf())));

    // Try to create hardware device

    HRESULT hardwareResult = D3D12CreateDevice(nullptr,  // default adapter
                                               D3D_FEATURE_LEVEL_11_0,
                                               IID_PPV_ARGS(d3dDevice.ReleaseAndGetAddressOf()));

    // Fallback to WARP device
    if (FAILED(hardwareResult)) {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(warpAdapter.GetAddressOf())));

        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(),
                                        D3D_FEATURE_LEVEL_11_0,
                                        IID_PPV_ARGS(d3dDevice.ReleaseAndGetAddressOf())));
    }

    // Fence
    ThrowIfFailed(d3dDevice->CreateFence(0,
                                         D3D12_FENCE_FLAG_NONE,
                                         IID_PPV_ARGS(fence.ReleaseAndGetAddressOf())));

    // Cache descriptor sizes
    rtvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    cbvSrvDescriptorSize = d3dDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 4x MSAA quality support
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels{};
    qualityLevels.Format = backBufferFormat;
    qualityLevels.SampleCount = 4;
    qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    qualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(d3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                                 &qualityLevels,
                                                 sizeof(qualityLevels)));

    msaa4xQuality = qualityLevels.NumQualityLevels;
    assert(msaa4xQuality > 0 && "Unexpected MSAA quality level.");

    CreateCommandObjects();
    CreateSwapChain();
    CreateDescriptorHeaps();

    return true;
}

void MyD3DApp::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(
        d3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(commandQueue.ReleaseAndGetAddressOf())));

    ThrowIfFailed(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                    IID_PPV_ARGS(commandAllocator.GetAddressOf())));

    ThrowIfFailed(d3dDevice->CreateCommandList(0,
                                               D3D12_COMMAND_LIST_TYPE_DIRECT,
                                               commandAllocator.Get(),
                                               nullptr,
                                               IID_PPV_ARGS(commandList.GetAddressOf())));
    commandList->Close();
}

void MyD3DApp::CreateSwapChain() {
    swapChain.Reset();

    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferDesc.Width = GetClientWidth();
    desc.BufferDesc.Height = GetClientHeight();
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = backBufferFormat;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    desc.SampleDesc.Count = msaa4xEnabled ? 4 : 1;
    desc.SampleDesc.Quality = msaa4xEnabled ? (msaa4xQuality - 1) : 0;

    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = s_swapChainBufferCount;

    desc.OutputWindow = GetWindowHandle();
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(
        dxgiFactory->CreateSwapChain(commandQueue.Get(), &desc, swapChain.GetAddressOf()));
}

void MyD3DApp::CreateDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = s_swapChainBufferCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;

    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&rtvHeapDesc,
                                                  IID_PPV_ARGS(rtvDescriptorHeap.GetAddressOf())));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;

    ThrowIfFailed(d3dDevice->CreateDescriptorHeap(&dsvHeapDesc,
                                                  IID_PPV_ARGS(dsvDescriptorHeap.GetAddressOf())));
}

D3D12_CPU_DESCRIPTOR_HANDLE MyD3DApp::GetCurrentBackBufferView() const {
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(GetStartSwapChainBufferView(),
                                         currBackBuffer,
                                         rtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE MyD3DApp::GetStartSwapChainBufferView() const {
    return rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE MyD3DApp::GetDepthStencilView() const {
    return dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void MyD3DApp::CreateBackBufferViews() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(GetStartSwapChainBufferView());

    for (UINT i = 0; i < s_swapChainBufferCount; ++i) {
        // Get the i-th buffer in the swap chain
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(swapChainBuffers[i].GetAddressOf())));

        // Create an RTV to it
        d3dDevice->CreateRenderTargetView(swapChainBuffers[i].Get(), nullptr, rtv);

        // Next entry in descriptor heap
        rtv.Offset(1, rtvDescriptorSize);
    }
}

void MyD3DApp::CreateDepthStencilView() {
    D3D12_RESOURCE_DESC dsvDesc{};
    dsvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    dsvDesc.Alignment = 0;
    dsvDesc.Width = GetClientWidth();
    dsvDesc.Height = GetClientHeight();
    dsvDesc.DepthOrArraySize = 1;
    dsvDesc.MipLevels = 1;
    dsvDesc.Format = depthStencilFormat;
    dsvDesc.SampleDesc.Count = msaa4xEnabled ? 4 : 1;
    dsvDesc.SampleDesc.Quality = msaa4xEnabled ? (msaa4xQuality - 1) : 0;
    dsvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    dsvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = depthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(
        d3dDevice->CreateCommittedResource(&heapProp,
                                           D3D12_HEAP_FLAG_NONE,
                                           &dsvDesc,
                                           D3D12_RESOURCE_STATE_COMMON,
                                           &optClear,
                                           IID_PPV_ARGS(depthStencilBuffer.GetAddressOf())));

    d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(), nullptr, GetDepthStencilView());

    // Transition the resource from init state to be used as a depth buffer
    commandList->ResourceBarrier(
        1,
        &CD3DX12_RESOURCE_BARRIER::Transition(depthStencilBuffer.Get(),
                                              D3D12_RESOURCE_STATE_COMMON,
                                              D3D12_RESOURCE_STATE_DEPTH_WRITE));
}

void MyD3DApp::SetMsaa4x(bool state) {
    if (state != msaa4xEnabled) {
        msaa4xEnabled = state;

        CreateSwapChain();
        OnResize();
    }
}