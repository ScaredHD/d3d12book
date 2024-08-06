#include "BoxApp.h"

#include "Common/MathHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void BoxApp::OnInitialize() {
    D3DApp::OnInitialize();

    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));

    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildRootSignature();
    BuildShadersAndInputLayout();
    BuildBoxGeometry();
    BuildPso();

    // Execute the initialization commands
    ThrowIfFailed(commandList->Close());
    ExecuteCommandList();
    FlushCommandQueue();
}

void BoxApp::OnResize() {
    D3DApp::OnResize();

    XMMATRIX m = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
    XMStoreFloat4x4(&matProjection_, m);
}

void BoxApp::OnUpdate() {
    D3DApp::OnUpdate();

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
    uploader_->CopyData(0, obj);
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
    ThrowIfFailed(commandAllocator->Reset());

    ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pso_.Get()));

    SetViewportAndScissorRects();

    // Transition of back buffer to render target state
    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_->GetCurrentBackBuffer(),
                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &transition);

    auto rtv = swapChain_->GetCurrentBackBufferView();
    commandList->ClearRenderTargetView(rtv, Colors::LightSteelBlue, 0, nullptr);

    auto dsv = dsvHeap_->GetDescriptor(0);
    commandList->ClearDepthStencilView(dsv,
                                       D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                       1.0f,
                                       0,
                                       0,
                                       nullptr);

    commandList->OMSetRenderTargets(1, &rtv, true, &dsv);

    ID3D12DescriptorHeap* descriptorHeaps[] = {cbvHeap_.Get()};
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    auto vbv = boxGeo_->VertexBufferView();
    commandList->IASetVertexBuffers(0, 1, &vbv);

    auto ibv = boxGeo_->IndexBufferView();
    commandList->IASetIndexBuffer(&ibv);

    commandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Bind actual cbvHeap (argument) to root signature (parameter)
    commandList->SetGraphicsRootDescriptorTable(0, cbvHeap_->GetGPUDescriptorHandleForHeapStart());

    commandList->DrawIndexedInstanced(boxGeo_->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

    transition = CD3DX12_RESOURCE_BARRIER::Transition(swapChain_->GetCurrentBackBuffer(),
                                                      D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                      D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &transition);

    ThrowIfFailed(commandList->Close());

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

void BoxApp::BuildDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;

    ThrowIfFailed(
        device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(cbvHeap_.ReleaseAndGetAddressOf())));
}

void BoxApp::BuildConstantBuffers() {
    uploader_ = std::make_unique<UploadBuffer<ConstantBufferObject>>(device.Get(), 1, true);

    UINT bytesize = d3dUtil::CalcConstantBufferByteSize(sizeof(ConstantBufferObject));

    D3D12_GPU_VIRTUAL_ADDRESS uploadBufferAddressGPU = uploader_->Resource()->GetGPUVirtualAddress();
    int boxConstantBufferIndex = 0;
    uploadBufferAddressGPU += boxConstantBufferIndex * bytesize;

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    cbvDesc.BufferLocation = uploadBufferAddressGPU;
    cbvDesc.SizeInBytes = bytesize;

    device->CreateConstantBufferView(&cbvDesc, cbvHeap_->GetCPUDescriptorHandleForHeapStart());
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
        device->CreateRootSignature(0,
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

    const UINT vbByteSize = vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = indices.size() * sizeof(std::uint16_t);

    boxGeo_ = std::make_unique<MeshGeometry>();
    boxGeo_->Name = "boxGeo";

    // Create VB and IB on CPU
    ThrowIfFailed(D3DCreateBlob(vbByteSize, boxGeo_->VertexBufferCPU.ReleaseAndGetAddressOf()));
    CopyMemory(boxGeo_->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

    ThrowIfFailed(D3DCreateBlob(ibByteSize, boxGeo_->IndexBufferCPU.ReleaseAndGetAddressOf()));
    CopyMemory(boxGeo_->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

    // Create VB and IB on GPU
    boxGeo_->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
                                                           commandList.Get(),
                                                           vertices.data(),
                                                           vbByteSize,
                                                           boxGeo_->VertexBufferUploader);
    boxGeo_->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(device.Get(),
                                                          commandList.Get(),
                                                          indices.data(),
                                                          ibByteSize,
                                                          boxGeo_->IndexBufferUploader);

    boxGeo_->VertexByteStride = sizeof(Vertex);
    boxGeo_->VertexBufferByteSize = vbByteSize;
    boxGeo_->IndexFormat = DXGI_FORMAT_R16_UINT;
    boxGeo_->IndexBufferByteSize = ibByteSize;

    SubmeshGeometry submesh;
    submesh.IndexCount = (UINT)indices.size();
    submesh.StartIndexLocation = 0;
    submesh.BaseVertexLocation = 0;
    boxGeo_->DrawArgs["box"] = submesh;
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
    psoDesc.SampleDesc.Count = Msaa4xEnabled() ? 4 : 1;
    psoDesc.SampleDesc.Quality = Msaa4xEnabled() ? GetMsaa4xQuality() - 1 : 0;
    psoDesc.DSVFormat = depthStencilFormat_;

    ThrowIfFailed(
        device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pso_.ReleaseAndGetAddressOf())));
}