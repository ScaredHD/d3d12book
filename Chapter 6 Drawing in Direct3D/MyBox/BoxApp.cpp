#include "BoxApp.h"

#include "Common/MathHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void BoxApp::OnInitialize() {
    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), nullptr));

    BuildCbvHeap();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPso();

    // Execute the initialization commands
    ThrowIfFailed(commandList_->Close());
    ExecuteCommandList();
    FlushCommandQueue();
}

void BoxApp::OnResize() {
    D3DApp::OnResize();

    XMMATRIX m = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&matProjection_, m);
}

void BoxApp::OnUpdate() {
    if (IsMouseDown()) {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(GetMouseX() - lastMousePosX_));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(GetMouseY() - lastMousePosY_));
        // Update angles based on input to orbit camera around box.
        // d3dbook has += instead. Feels unnatural
        cameraPose_.theta -= dx;
        cameraPose_.phi -= dy;
        // Restrict the angle mPhi.
        cameraPose_.phi = MathHelper::Clamp(cameraPose_.phi, 0.1f, MathHelper::Pi - 0.1f);

        // std::string s = std::to_string(IsMouseDown()) + "\t" + std::to_string(dx);
        // s += "\t" + std::to_string(dy) + "\n";
        // ::OutputDebugStringA(s.c_str());

        lastMousePosX_ = GetMouseX();
        lastMousePosY_ = GetMouseY();
    }

    float x = cameraPose_.radius * sinf(cameraPose_.phi) * cosf(cameraPose_.theta);
    float y = cameraPose_.radius * cosf(cameraPose_.phi);
    float z = cameraPose_.radius * sinf(cameraPose_.phi) * sinf(cameraPose_.theta);

    XMMATRIX model = XMLoadFloat4x4(&matModel_);

    XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
    XMVECTOR target = XMVectorZero();
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

    XMMATRIX proj = XMLoadFloat4x4(&matProjection_);

    XMMATRIX modelViewProj = model * view * proj;

    ConstantBufferObject obj;
    XMStoreFloat4x4(&obj.modelViewProj, XMMatrixTranspose(modelViewProj));
    cbuffer_->Load(0, obj);
}

void BoxApp::OnMouseDown(int xPos, int yPos) {
    lastMousePosX_ = GetMouseX();
    lastMousePosY_ = GetMouseY();
    SetCapture(GetWindow());
}

void BoxApp::OnMouseUp(int xPos, int yPos) {
    ReleaseCapture();
}

void BoxApp::Draw() {
    ThrowIfFailed(commandAllocator_->Reset());

    ThrowIfFailed(commandList_->Reset(commandAllocator_.Get(), pso_.Get()));

    SetViewportAndScissorRects();

    // Transition of back buffer to render target state
    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_->GetCurrentBackBuffer(),
                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList_->ResourceBarrier(1, &transition);

    auto rtv = swapChain_->GetCurrentBackBufferView();
    commandList_->ClearRenderTargetView(rtv, Colors::LightSteelBlue, 0, nullptr);

    auto dsv = dsvHeap_->GetDescriptorHandleCpu(0);
    commandList_->ClearDepthStencilView(dsv,
                                        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                        1.0f,
                                        0,
                                        0,
                                        nullptr);

    commandList_->OMSetRenderTargets(1, &rtv, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap_->Get()};
    commandList_->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList_->SetGraphicsRootSignature(rootSignature_.Get());

    auto vbv = vbuffer_->GetView();
    commandList_->IASetVertexBuffers(0, 1, &vbv);

    auto ibv = ibuffer_->GetView();
    commandList_->IASetIndexBuffer(&ibv);

    commandList_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Bind actual cbvHeap (argument) to root signature (parameter)
    commandList_->SetGraphicsRootDescriptorTable(
        0,
        cbvHeap_->Get()->GetGPUDescriptorHandleForHeapStart());

    commandList_->DrawIndexedInstanced(boxGeo_->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

    transition = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_->GetCurrentBackBuffer(),
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      D3D12_RESOURCE_STATE_PRESENT);
    commandList_->ResourceBarrier(1, &transition);

    ThrowIfFailed(commandList_->Close());

    ExecuteCommandList();

    // Swap back and front buffers
    ThrowIfFailed(swapChain_->Present());
    swapChain_->SwapBuffers();

    FlushCommandQueue();
}

LRESULT BoxApp::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_LBUTTONDOWN) {
        BoxApp::OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    if (msg == WM_LBUTTONUP) {
        BoxApp::OnMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }

    return D3DApp::HandleMessage(hwnd, msg, wParam, lParam);
}

void BoxApp::BuildCbvHeap() {
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;

    cbvHeap_ = std::make_unique<DescriptorHeap>(device_.Get(), cbvHeapDesc);
}

void BoxApp::BuildConstantBuffers() {
    cbuffer_ = std::make_unique<ConstantBuffer<ConstantBufferObject>>(device_.Get(), 1);
    CreateConstantBufferViewOnHeap(device_.Get(), cbuffer_.get(), 0, cbvHeap_.get(), 0);
}

void BoxApp::BuildRootSignature() {
    CD3DX12_ROOT_PARAMETER slotRootParameter[1];

    // Create a single descriptor table of CBVs
    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV,  // table type
                  1,                                // number of descriptors in the table
                  0  // base shader register arguments are bound to for this root parameter
    );
    slotRootParameter[0].InitAsDescriptorTable(1,         // number of ranges
                                               &cbvTable  // pointer to array of ranges
    );

    // a root signature is an array of root parameters
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
        1,
        slotRootParameter,
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a
    // single constant buffer
    ComPtr<ID3DBlob> serializedRootSig{};
    ComPtr<ID3DBlob> errorBlob{};
    auto result = D3D12SerializeRootSignature(&rootSigDesc,
                                              D3D_ROOT_SIGNATURE_VERSION_1,
                                              serializedRootSig.GetAddressOf(),
                                              errorBlob.GetAddressOf());

    if (errorBlob) {
        ::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
    }
    ThrowIfFailed(result);

    ThrowIfFailed(
        device_->CreateRootSignature(0,
                                     serializedRootSig->GetBufferPointer(),
                                     serializedRootSig->GetBufferSize(),
                                     IID_PPV_ARGS(rootSignature_.ReleaseAndGetAddressOf())));
}

void BoxApp::BuildShadersAndInputLayout() {
    vertexShaderByteCode_ = d3dUtil::CompileShader(L"shaders.hlsl", nullptr, "VS", "vs_5_0");
    pixelShaderByteCode_ = d3dUtil::CompileShader(L"shaders.hlsl", nullptr, "PS", "ps_5_0");

    inputLayout_ = {{"POSITION",
                     0,
                     DXGI_FORMAT_R32G32B32_FLOAT,
                     0,
                     0,
                     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                     0},
                    {"COLOR",
                     0,
                     DXGI_FORMAT_R32G32B32A32_FLOAT,
                     0,
                     12,
                     D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                     0}};
}

void BoxApp::BuildBoxGeometry() {
    std::array<Vertex, 8> vertices = {
        Vertex{XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)},
        Vertex{XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)},
        Vertex{XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)},
        Vertex{XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)},
        Vertex{XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)},
        Vertex{XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)},
        Vertex{XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)},
        Vertex{XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)}};

    // clang-format off
    std::array<std::uint16_t, 36> indices = {
        // front face
        0, 1, 2,
        0, 2, 3,
        // back face
        4, 6, 5,
        4, 7, 6,
        // left face
        4, 5, 1,
        4, 1, 0,
        // right face
        3, 2, 6,
        3, 6, 7,
        // top face
        1, 5, 6,
        1, 6, 2,
        // bottom face
        4, 0, 3,
        4, 3, 7
    };
    // clang-format on

    boxGeo_ = std::make_unique<MeshGeometry>();
    
    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;
    boxGeo_->DrawArgs["box"] = submesh;

    UINT vbByteSize = vertices.size() * sizeof(Vertex);
    vbuffer_ = std::make_unique<VertexBuffer>(sizeof(Vertex), vbByteSize);
    vbuffer_->Load(device_.Get(), commandList_.Get(), vertices.data(), vbByteSize);

    UINT ibByteSize = indices.size() * sizeof(std::uint16_t);
    ibuffer_ = std::make_unique<IndexBuffer>(DXGI_FORMAT_R16_UINT, ibByteSize);
    ibuffer_->Load(device_.Get(), commandList_.Get(), indices.data(), ibByteSize);
}

void BoxApp::BuildPso() {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
    psoDesc.InputLayout = {inputLayout_.data(), (UINT)inputLayout_.size()};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.VS = {reinterpret_cast<BYTE*>(vertexShaderByteCode_->GetBufferPointer()),
                  vertexShaderByteCode_->GetBufferSize()};
    psoDesc.PS = {reinterpret_cast<BYTE*>(pixelShaderByteCode_->GetBufferPointer()),
                  pixelShaderByteCode_->GetBufferSize()};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = backBufferFormat_;
    psoDesc.SampleDesc.Count = msaa4XEnabled_ ? 4 : 1;
    psoDesc.SampleDesc.Quality = msaa4XEnabled_ ? msaa4XQuality_ - 1 : 0;
    psoDesc.DSVFormat = depthStencilFormat_;

    ThrowIfFailed(
        device_->CreateGraphicsPipelineState(&psoDesc,
                                             IID_PPV_ARGS(pso_.ReleaseAndGetAddressOf())));
}