#include "D3DApp.h"

using namespace Microsoft::WRL;

LRESULT D3DApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SIZE) {
        D3DApp::OnResize();
        return 0;
    }
    return MyApp::HandleMessage(hwnd, msg, wParam, lParam);
}

void D3DApp::OnMouseUp(int xPos, int yPos) {
    MyApp::OnMouseUp(xPos, yPos);
}

void D3DApp::OnMouseDown(int xPos, int yPos) {
    MyApp::OnMouseDown(xPos, yPos);
}

void D3DApp::OnKeyDown() {
    if (IsKeyDown('S')) {
        MessageBox(GetWindow(), L"test", L"test caption", 0);
        OutputDebugString(L"S key is pressed\n");
        return;
    }
    MyApp::OnKeyDown();
}

void D3DApp::OnKeyUp() {
    MyApp::OnKeyUp();
}

void D3DApp::UpdateViewport() {
    auto w = static_cast<LONG>(GetClientWidth());
    auto h = static_cast<LONG>(GetClientHeight());
    viewport_.TopLeftX = 0;
    viewport_.TopLeftY = 0;
    viewport_.Width = w;
    viewport_.Height = h;
    viewport_.MinDepth = 0.0f;
    viewport_.MaxDepth = 1.0f;

    scissorRect_ = {0, 0, w, h};
}

void D3DApp::ExecuteCommandList() {
    ID3D12CommandList* cmdLists[] = {commandList_.Get()};
    commandQueue_->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void D3DApp::OnResize() {
    MyApp::OnResize();

    assert(device_);
    assert(swapChain_);
    assert(commandAllocator_);

    // Flush before changing any resources.
    FlushCommandQueue();

    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

    swapChain_->ResetAllBuffers();

    ThrowIfFailed(swapChain_->Resize(GetClientWidth(), GetClientHeight()));

    swapChain_->CreateRtvs(device_.Get());

    CreateDepthStencilBuffer();

    CreateDepthStencilView();

    ExecuteCommandList();

    FlushCommandQueue();

    UpdateViewport();
}

void D3DApp::Initialize(const wchar_t* windowName) {
    if (!InitializeWindow(windowName)) {
        throw std::runtime_error("Failed to initialize window");
    }

    if (!InitializeD3D()) {
        throw std::runtime_error("Failed to initialize D3D");
    }

    timer_.Reset();

    OnResize();
    OnInitialize();
}

bool D3DApp::InitializeD3D() {
#if defined(DEBUG) || defined(_DEBUG)
    {
        ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory_.ReleaseAndGetAddressOf())));

    // Try to create hardware device

    // Fallback to WARP device
    if (HRESULT hardwareResult = D3D12CreateDevice(nullptr,  // default adapter
                                                   D3D_FEATURE_LEVEL_11_0,
                                                   IID_PPV_ARGS(device_.ReleaseAndGetAddressOf()));
        FAILED(hardwareResult)) {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(dxgiFactory_->EnumWarpAdapter(IID_PPV_ARGS(warpAdapter.GetAddressOf())));

        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(),
                                        D3D_FEATURE_LEVEL_11_0,
                                        IID_PPV_ARGS(device_.ReleaseAndGetAddressOf())));
    }

    // Fence
    ThrowIfFailed(device_->CreateFence(0,
                                       D3D12_FENCE_FLAG_NONE,
                                       IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf())));

    // 4x MSAA quality support
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels{};
    qualityLevels.Format = backBufferFormat_;
    qualityLevels.SampleCount = 4;
    qualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    qualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                               &qualityLevels,
                                               sizeof(qualityLevels)));

    msaa4XQuality_ = qualityLevels.NumQualityLevels;
    assert(msaa4XQuality_ > 0 && "Unexpected MSAA quality level.");

    CreateCommandObjects();
    CreateSwapChain();
    CreateDsvHeaps();

    return true;
}

void D3DApp::Update() {
    timer_.Tick();

    if (!isPaused_) {
        CalculateFrameStats();
        OnUpdate();
    } else {
        Sleep(100);
    }
}

void D3DApp::SetViewportAndScissorRects() {
    commandList_->RSSetViewports(1, &viewport_);
    commandList_->RSSetScissorRects(1, &scissorRect_);
}

void D3DApp::Draw() {
    ThrowIfFailed(commandAllocator_->Reset());
    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

    Transition(swapChain_->GetCurrentBackBuffer(),
               D3D12_RESOURCE_STATE_PRESENT,
               D3D12_RESOURCE_STATE_RENDER_TARGET);

    SetViewportAndScissorRects();

    commandList_->ClearRenderTargetView(swapChain_->GetCurrentBackBufferView(),
                                        DirectX::Colors::LightSteelBlue,
                                        0,
                                        nullptr);
    commandList_->ClearDepthStencilView(dsvHeap_->GetDescriptor(0),
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                        1.0f,
                                        0,
                                        0,
                                        nullptr);

    auto rtv = swapChain_->GetCurrentBackBufferView();
    auto dsv = dsvHeap_->GetDescriptor(0);
    commandList_->OMSetRenderTargets(1, &rtv, true, &dsv);

    Transition(swapChain_->GetCurrentBackBuffer(),
               D3D12_RESOURCE_STATE_RENDER_TARGET,
               D3D12_RESOURCE_STATE_PRESENT);

    ThrowIfFailed(commandList_->Close());

    ExecuteCommandList();

    ThrowIfFailed(swapChain_->Present());
    swapChain_->SwapBuffers();

    FlushCommandQueue();
}

void D3DApp::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(
        device_->CreateCommandQueue(&queueDesc,
                                    IID_PPV_ARGS(commandQueue_.ReleaseAndGetAddressOf())));

    ThrowIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                  IID_PPV_ARGS(commandAllocator_.GetAddressOf())));

    ThrowIfFailed(device_->CreateCommandList(0,
                                             D3D12_COMMAND_LIST_TYPE_DIRECT,
                                             commandAllocator_.Get(),
                                             nullptr,
                                             IID_PPV_ARGS(commandList_.GetAddressOf())));
    ThrowIfFailed(commandList_->Close());
}

void D3DApp::CreateSwapChain() {
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferDesc.Width = GetClientWidth();
    desc.BufferDesc.Height = GetClientHeight();
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format = backBufferFormat_;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    desc.SampleDesc.Count = msaa4XEnabled_ ? 4 : 1;
    desc.SampleDesc.Quality = msaa4XEnabled_ ? (msaa4XQuality_ - 1) : 0;

    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = swapChain_->BufferCount();

    desc.OutputWindow = GetWindow();
    desc.Windowed = true;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    swapChain_ = std::make_unique<SwapChain<2>>(device_.Get(),
                                                dxgiFactory_.Get(),
                                                commandQueue_.Get(),
                                                desc);
}

void D3DApp::CreateDsvHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = 1;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NodeMask = 0;

    dsvHeap_ = std::make_unique<DescriptorHeap>(device_.Get(), desc);
}

void D3DApp::FlushCommandQueue() {
    ThrowIfFailed(commandQueue_->Signal(fence_.Get(), ++nextFence_));

    if (fence_->GetCompletedValue() < nextFence_) {
        auto eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(fence_->SetEventOnCompletion(nextFence_, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void D3DApp::CreateDepthStencilBuffer() {
    depthStencilBuffer_.Reset();

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = GetClientWidth();
    desc.Height = GetClientHeight();
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    // depthStencilDesc.Format = depthStencilFormat_;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.SampleDesc.Count = msaa4XEnabled_ ? 4 : 1;
    desc.SampleDesc.Quality = msaa4XEnabled_ ? (msaa4XQuality_ - 1) : 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device_->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        &optClear,
        IID_PPV_ARGS(depthStencilBuffer_.ReleaseAndGetAddressOf())));
}

void D3DApp::CreateDepthStencilView() {
    D3D12_DEPTH_STENCIL_VIEW_DESC desc{};
    desc.Flags = D3D12_DSV_FLAG_NONE;
    desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    desc.Texture2D.MipSlice = 0;

    device_->CreateDepthStencilView(depthStencilBuffer_.Get(), &desc, dsvHeap_->GetDescriptor(0));

    Transition(depthStencilBuffer_.Get(),
               D3D12_RESOURCE_STATE_COMMON,
               D3D12_RESOURCE_STATE_DEPTH_WRITE);

    ThrowIfFailed(commandList_->Close());
}

void D3DApp::Transition(ID3D12Resource* resource,
                        D3D12_RESOURCE_STATES from,
                        D3D12_RESOURCE_STATES to) {
    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(resource, from, to);
    commandList_->ResourceBarrier(1, &transition);
}

void D3DApp::CalculateFrameStats() {
    ++frameCount_;
    // OutputDebugString(std::to_wstring(timer.TotalTimeFromStart()).c_str());

    // Do calculations every one second
    if (auto diff = timer_.TotalTimeFromStart() - timeElapsed_; diff > 1.0f) {
        float fps = frameCount_;
        float frameTime = 1000.0f / fps;

        auto fpsStr = std::to_wstring(fps);
        auto frameTimeStr = std::to_wstring(frameTime);

        std::wstring windowText = GetWindowName();
        windowText += L"  fps: " + fpsStr;
        windowText += L"  frame time: " + frameTimeStr;
        SetWindowText(GetWindow(), windowText.c_str());

        frameCount_ = 0;
        timeElapsed_ += diff;
    }
}