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
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
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
    D3D12_COMMAND_QUEUE_DESC desc;
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

    DXGI_SWAP_CHAIN_DESC desc;
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
    desc.BufferCount = swapChainBufferCount;

    desc.OutputWindow = GetWindowHandle();
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    ThrowIfFailed(
        dxgiFactory->CreateSwapChain(commandQueue.Get(), &desc, swapChain.GetAddressOf()));
}

void MyD3DApp::CreateDescriptorHeaps() {}

void MyD3DApp::SetMsaa4x(bool state) {
    if (state != msaa4xEnabled) {
        msaa4xEnabled = state;

        CreateSwapChain();
        OnResize();
    }
}